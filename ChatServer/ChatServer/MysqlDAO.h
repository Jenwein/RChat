#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>
#include "ConfigMgr.h"

class SqlConnection
{
public:
	SqlConnection(std::unique_ptr<sql::Connection> con,int64_t lasttime)
		:m_con(std::move(con)),m_last_oper_time(lasttime)
	{}
	
	int64_t m_last_oper_time;
	std::unique_ptr<sql::Connection> m_con;
};

//mysql���ӳ�
class MysqlPool
{
public:
	//���캯������ʼ�����ӳأ�������ʼ����
	MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize);
	//�������ӳ�
	~MysqlPool();

	void checkConnection();

	//��ȡ���ӣ������ӳ��л�ȡһ����������
	std::unique_ptr<SqlConnection> getConnection();
	

	//�黹���ӣ���ʹ����ϵ����ӹ黹�����ӳ���
	void returnConnection(std::unique_ptr<SqlConnection> con);

	//�ر����ӳأ�ֹͣ���ӳصĹ�����֪ͨ���еȴ����̡߳�
	void Close();

private:
	//���ݿ����ӵĻ�����Ϣ
	std::string m_url;		//host
	std::string m_user;		//���ݿ��û���
	std::string m_pass;		//���ݿ�����
	std::string m_schema;	//���ݿ������

	int m_poolSize;			//���ӳصĴ�С
	std::mutex m_mtx;
	std::queue<std::unique_ptr<SqlConnection>> m_pool;	//�洢���ӵĶ���
	std::condition_variable m_cond;
	std::atomic<bool> m_b_stop;
	std::thread m_check_thread;
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class MysqlDAO
{
public:
	//��ʼ�� MySqlPool
	MysqlDAO();
	~MysqlDAO();

	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	//��������Ƿ�һ��
	bool CheckEmail(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	//��������
	bool UpdatePwd(const std::string& name, const std::string& newpwd);

private:
	std::unique_ptr<MysqlPool> m_pool;
};

