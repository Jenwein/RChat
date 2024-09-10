#include "MysqlDAO.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize) :m_url(url), m_user(user), m_pass(pass), m_schema(schema), m_poolSize(poolsize), m_b_stop(false)
{
	try
	{
		//������ʼ����
		for (int i = 0; i < m_poolSize; ++i)
		{
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			std::unique_ptr<sql::Connection>con(driver->connect(m_url, m_user, m_pass));
			con->setSchema(m_schema);
			//��ȡ��ǰʱ���
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			//��ʱ���ת��Ϊ��
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			m_pool.push(std::make_unique<SqlConnection>(std::move(con), timestamp));
		}

		//������⣬ÿ60s���һ������״̬
		m_check_thread = std::thread([this]() {
			while (!m_b_stop)
			{
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});
		m_check_thread.detach();

	}
	catch (const sql::SQLException& e)
	{
		// �����쳣
		std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
	}
}

MysqlPool::~MysqlPool()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	m_pool = std::queue<std::unique_ptr<SqlConnection>>();
}

void MysqlPool::checkConnection()
{
	std::lock_guard<std::mutex> guard(m_mtx);
	int poolsize = m_pool.size();
	//��ȡ��ǰʱ���
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

	//�������У���ÿ����������ȡ��������������
	for (int i = 0; i < poolsize; ++i)
	{
		auto con = std::move(m_pool.front());
		m_pool.pop();
		//�黹����
		Defer defer([this, &con] {
			m_pool.push(std::move(con));
			});
		//��ǰʱ�����ϴμ��ʱ��֮�䳬��5��������Ҫ����һ��������Ϣ��ά�����ӣ���������������һ������
		if (timestamp - con->m_last_oper_time < 5 * 60)
			continue;


		try
		{
			std::unique_ptr<sql::Statement> stmt(con->m_con->createStatement());
			stmt->executeQuery("SELECT 1");	//������Ϣ
			con->m_last_oper_time = timestamp;	//���¼��ʱ��

		}
		catch (const sql::SQLException& e)
		{
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			//�����µ����������Ǿɵ��쳣����
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			std::unique_ptr<sql::Connection> newcon(driver->connect(m_url, m_user, m_pass));
			newcon->setSchema(m_schema);
			con->m_con = std::move(newcon);
			con->m_last_oper_time = timestamp;
		}
	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mtx);	//�漰���в�������
	// �������Ϊ���������ȴ�
	m_cond.wait(lock, [this]() {
		return !m_pool.empty() || m_b_stop;
		}
	);
	//�����Ƿ�����ֹ����ֹ�򷵻�nullptr�˳�
	if (m_b_stop)
		return nullptr;

	std::unique_ptr<SqlConnection> con(std::move(m_pool.front()));
	m_pool.pop();
	return con;
}

void MysqlPool::returnConnection(std::unique_ptr<SqlConnection> con)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	if (m_b_stop)
		return;

	m_pool.push(std::move(con));
	m_cond.notify_one();
}

void MysqlPool::Close()
{
	m_b_stop = true;
	m_cond.notify_all();
}

MysqlDAO::MysqlDAO()
{
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	//m_pool.reset(new MysqlPool(host + ":" + port, user, pwd, schema, 5));
	m_pool = std::make_unique<MysqlPool>(host + ":" + port, user, pwd, schema, 5);
}

MysqlDAO::~MysqlDAO()
{
	m_pool->Close();
}

int MysqlDAO::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	//�����ӳ��л�ȡ����
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return false;
	}
	Defer defer([this, &con] {
		m_pool->returnConnection(std::move(con));
		});
	try
	{
		std::unique_ptr<sql::Statement> stmt(con->m_con->createStatement());//���� SQL ������
		con->m_con->setAutoCommit(false);	// �����Զ��ύ����ʼ����

		//����û����Ƿ��Ѵ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_con->prepareStatement("SELECT 1 FROM `user` WHERE `name` = ? "));
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet>res(pstmt->executeQuery());	//ִ�в�ѯ

		if (res->next())
		{
			//�û����Ѵ���
			con->m_con->commit();
			return 0;
		}

		//���email�Ƿ��Ѵ���
		pstmt.reset(con->m_con->prepareStatement("SELECT 1 FROM `user` WHERE `email` = ?"));
		pstmt->setString(1, email);
		res.reset(pstmt->executeQuery());

		if (res->next())
		{
			con->m_con->commit();
			return 0;	//email�Ѵ���
		}

		//����user_id��
		stmt->execute("UPDATE `user_id` SET `id` = `id` + 1");

		//��ȡ���º��id
		res.reset(stmt->executeQuery("SELECT `id` FROM `user_id`"));

		if (res->next())
		{
			int uid = res->getInt("id");

			//��user���в����¼�¼
			pstmt.reset(con->m_con->prepareStatement("INSERT INTO `user`(`uid`,`name`,`email`,`pwd`)VALUES(?,?,?,?)"));
			pstmt->setInt(1, uid);
			pstmt->setString(2, name);
			pstmt->setString(3, email);
			pstmt->setString(4, pwd);
			pstmt->execute();

			con->m_con->commit();
			return uid;
		}
		else
		{
			//δ�ܻ�ȡid
			con->m_con->rollback();
			throw std::runtime_error("Failed to retrieve updated id from user_id table.");
		}
	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(MySQL error code: " << e.getErrorCode();
		std::cerr << ",SQLState: " << e.getSQLState() << ")" << std::endl;
		return -1;
	}
}

std::shared_ptr<UserInfo> MysqlDAO::GetUser(int uid)
{
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return nullptr;
	}
	Defer defer([this, &con] {
		m_pool->returnConnection(std::move(con));
		});
	try
	{
		//׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_con->prepareStatement("SELECT * FROM `user` WHERE `uid` = ?"));
		pstmt->setInt(1, uid);
		//ִ�в�ѯ
		std::unique_ptr<sql::ResultSet>res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> userinfo = nullptr;
		//���������
		while (res->next())
		{
			userinfo.reset(new UserInfo);
			userinfo->email = res->getString("email");
			userinfo->name = res->getString("name");
			userinfo->pwd = res->getString("pwd");
			userinfo->uid = uid;
			break;
		}
		return userinfo;

	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << " ,SQLState: " << e.getSQLState() << ")" << std::endl;
		return nullptr;
	}

}

std::shared_ptr<UserInfo> MysqlDAO::GetUser(std::string name)
{
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return nullptr;
	}
	Defer defer([this, &con] {
		m_pool->returnConnection(std::move(con));
		});
	try
	{
		//׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_con->prepareStatement("SELECT * FROM `user` WHERE `name` = ?"));
		pstmt->setString(1, name);
		//ִ�в�ѯ
		std::unique_ptr<sql::ResultSet>res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> userinfo = nullptr;
		//���������
		while (res->next())
		{
			userinfo->email = res->getString("email");
			userinfo->name = name;
			userinfo->pwd = res->getString("pwd");
			userinfo->uid = res->getInt("uid");
			break;
		}
		return userinfo;

	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << " ,SQLState: " << e.getSQLState() << ")" << std::endl;
		return nullptr;
	}

}

bool MysqlDAO::CheckEmail(const std::string& name, const std::string& email)
{
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return false;
	}
	Defer defer([this, &con] {
		m_pool->returnConnection(std::move(con));
		});
	try
	{
		//׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_con->prepareStatement("SELECT `email` FROM `user` WHERE `name` = ?"));
		pstmt->setString(1, name);
		//ִ�в�ѯ
		std::unique_ptr<sql::ResultSet>res(pstmt->executeQuery());

		//���������
		while (res->next())
		{
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email"))
			{
				return false;
			}

			return true;
		}
		return false;
	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << " ,SQLState: " << e.getSQLState() << ")" << std::endl;
		return false;
	}
}

bool MysqlDAO::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return false;
	}
	Defer defer([this, &con] {m_pool->returnConnection(std::move(con));});
	try
	{
		//׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_con->prepareStatement("SELECT * FROM `user` WHERE `email` = ?"));
		pstmt->setString(1, email);
		//ִ�в�ѯ
		std::unique_ptr<sql::ResultSet>res(pstmt->executeQuery());
		std::string origin_pwd = "";
		//���������
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// �����ѯ��������
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}
		if (pwd!=origin_pwd)
		{
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (const sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << " ,SQLState: " << e.getSQLState() << ")" << std::endl;
		return false;
	}
}

bool MysqlDAO::UpdatePwd(const std::string& name, const std::string& newpwd)
{
	auto con = m_pool->getConnection();
	if (con == nullptr)
	{
		return false;
	}
	Defer defer([this, &con] {
		m_pool->returnConnection(std::move(con));
		});
	try
	{
		//׼����ѯ���
		std::unique_ptr<sql::PreparedStatement>pstmt(con->m_con->prepareStatement("UPDATE `user` SET `pwd` = ? WHERE `name` = ?"));
		pstmt->setString(1, newpwd);
		pstmt->setString(2, name);

		//ִ��
		int updateCount = pstmt->executeUpdate();

		std::cout << "Update rows: " << updateCount << std::endl;
		return true;
		
	}
	catch (const sql::SQLException& e)
	{

		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << " ,SQLState: " << e.getSQLState() << ")" << std::endl;
		return false;
	}
}
