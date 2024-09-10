#pragma once
#include "const.h"
#include "CSession.h"
#include "MysqlDAO.h"

typedef std::function<void(std::shared_ptr<CSession>,const short& msg_id,const std::string& msg_data)>FunCallBack;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void DealMsg();
	void PostMsgToQue(std::shared_ptr <LogicNode> msg);
	void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	void RegisterCallBacks();
	bool GetBaseInfo(std::string base_key,int uid, std::shared_ptr<UserInfo>& user_info);
private:
	LogicSystem();

private:
	std::thread m_worker_thread;
	std::queue<std::shared_ptr<LogicNode>> m_msg_que;
	std::mutex m_mtx;
	std::condition_variable m_consume;
	bool m_b_stop;
	std::map<short, FunCallBack> m_fun_callbacks;
};

