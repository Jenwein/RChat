#include "LogicSystem.h"
#include "VerifyGrpcClient.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"

LogicSystem::~LogicSystem()
{

}

void LogicSystem::DealMsg()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_mtx);

		//消息队列为空且服务未停止，挂起等待唤醒
		m_consume.wait(lock, [this]() { return !m_msg_que.empty() || m_b_stop;  });

		//判断当前是否为关闭状态,如果关闭，则执行所有逻辑处理后退出循环
		if (m_b_stop)
		{
			while (!m_msg_que.empty())
			{
				//取出队列中剩余待处理消息
				auto msg_node = m_msg_que.front();
				std::cout << "recv_msg id is " << msg_node->m_recvnode->m_msg_id << std::endl;
				auto call_back_iter = m_fun_callbacks.find(msg_node->m_recvnode->m_msg_id);
				if (call_back_iter == m_fun_callbacks.end())
				{
					//没找到对应的处理函数，非法数据，pop出队列，处理下一个消息
					m_msg_que.pop();
					continue;
				}

				//执行对应回调
				call_back_iter->second(msg_node->m_session, msg_node->m_recvnode->m_msg_id, std::string(msg_node->m_recvnode->m_data,msg_node->m_recvnode->m_cur_len));
				m_msg_que.pop();
			}
			break;
		}
		//如果服务没有停止
		auto msg_node = m_msg_que.front();
		std::cout << "recv_msg id is " << msg_node->m_recvnode->m_msg_id << std::endl;
		auto call_back_iter = m_fun_callbacks.find(msg_node->m_recvnode->m_msg_id);
		if (call_back_iter == m_fun_callbacks.end())
		{
			//没找到对应的处理函数，非法数据，pop出队列，处理下一个消息
			m_msg_que.pop();
			continue;
		}

		//执行对应回调
		call_back_iter->second(msg_node->m_session, msg_node->m_recvnode->m_msg_id, std::string(msg_node->m_recvnode->m_data, msg_node->m_recvnode->m_cur_len));
		m_msg_que.pop();

	}
}

void LogicSystem::PostMsgToQue(std::shared_ptr <LogicNode> msg)
{
	std::unique_lock<std::mutex>lock(m_mtx);
	m_msg_que.push(msg);
	//当消息队列从空变化为非空时通知消费者线程,可以减少不必要的上下文切换和线程唤醒,消费者线程通过异步回调执行来处理队列中的所有数据
	if (m_msg_que.size() == 1)
	{
		lock.unlock();
		m_consume.notify_one();
	}
}

//处理登录信息
void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is " << uid << " user token is " << token << std::endl;
	
	// 到状态服务器去验证
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());

	Json::Value rtvalue;
	//将在函数结束时执行
	Defer defer([this,&rtvalue,session]() {
		Json::StreamWriterBuilder writer;
		std::string json_data = Json::writeString(writer, rtvalue);
		std::cout << "Return String is:" << json_data;
		session->Send(json_data, MSG_CHAT_LOGIN_RSP);
		//回包给对端
		//std::string return_str = rtvalue.toStyledString();
		//std::cout << "Return String is:" << return_str;
		//session->Send(return_str,MSG_CHAT_LOGIN_RSP);
	});


	rtvalue["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::Success)
	{
		return;
	}

	//从redis中获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	//Redis中通过token_key(uid)查找到对应的token，将值存在token_value中
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	//获取token失败
	if (!success)
	{
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	//客户端携带的token与Redis中token不匹配
	if (token_value != token)
	{
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return;
	}

	//如果匹配，则返回用户基本信息
	rtvalue["error"] = ErrorCodes::Success;
	//获取用户基本信息
	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base)
	{
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;

	return;
}

void LogicSystem::RegisterCallBacks()
{
	m_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先在redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base)
	{
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);

		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		std::cout	<< " user login uid is  " << userinfo->uid 
					<< " name  is "<< userinfo->name 
					<< " pwd is " << userinfo->pwd 
					<< " email is " << userinfo->email 
					<< std::endl;
	}
	else
	{
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<UserInfo>user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr)
		{
			return false;
		}
		userinfo = user_info;
		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

	return true;
}

LogicSystem::LogicSystem()
	:m_b_stop(false)
{
	//注册回调
	RegisterCallBacks();
	//开辟线程开始执行处理
	m_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

