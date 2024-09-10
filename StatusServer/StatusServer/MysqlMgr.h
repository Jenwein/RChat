#pragma once
/*数据库管理者用来实现服务层，对接逻辑层的调用*/
#include "MysqlDAO.h"


class MysqlMgr:public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();

	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	
	bool CheckEmail(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd,UserInfo& UserInfo);

	bool UpdatePwd(const std::string& name, const std::string& pwd);
private:
	MysqlMgr();

	MysqlDAO m_dao;

};

