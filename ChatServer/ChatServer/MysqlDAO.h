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

//mysql连接池
class MysqlPool
{
public:
	//构造函数：初始化连接池，创建初始连接
	MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize);
	//清理连接池
	~MysqlPool();

	void checkConnection();

	//获取连接：从连接池中获取一个可用连接
	std::unique_ptr<SqlConnection> getConnection();
	

	//归还连接：将使用完毕的连接归还到连接池中
	void returnConnection(std::unique_ptr<SqlConnection> con);

	//关闭连接池：停止连接池的工作，通知所有等待的线程。
	void Close();

private:
	//数据库连接的基本信息
	std::string m_url;		//host
	std::string m_user;		//数据库用户名
	std::string m_pass;		//数据库密码
	std::string m_schema;	//数据库的名称

	int m_poolSize;			//连接池的大小
	std::mutex m_mtx;
	std::queue<std::unique_ptr<SqlConnection>> m_pool;	//存储连接的队列
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
	//初始化 MySqlPool
	MysqlDAO();
	~MysqlDAO();

	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	//检查邮箱是否一致
	bool CheckEmail(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	//更新密码
	bool UpdatePwd(const std::string& name, const std::string& newpwd);

private:
	std::unique_ptr<MysqlPool> m_pool;
};

