#pragma once
/*���ݿ����������ʵ�ַ���㣬�Խ��߼���ĵ���*/
#include "MysqlDAO.h"


class MysqlMgr:public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();

	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	
	bool CheckEmail(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd,UserInfo& UserInfo);
	bool UpdatePwd(const std::string& name, const std::string& pwd);
private:
	MysqlMgr();

	MysqlDAO m_dao;

};

