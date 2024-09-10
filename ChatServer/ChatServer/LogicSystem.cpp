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

		//��Ϣ����Ϊ���ҷ���δֹͣ������ȴ�����
		m_consume.wait(lock, [this]() { return !m_msg_que.empty() || m_b_stop;  });

		//�жϵ�ǰ�Ƿ�Ϊ�ر�״̬,����رգ���ִ�������߼�������˳�ѭ��
		if (m_b_stop)
		{
			while (!m_msg_que.empty())
			{
				//ȡ��������ʣ���������Ϣ
				auto msg_node = m_msg_que.front();
				std::cout << "recv_msg id is " << msg_node->m_recvnode->m_msg_id << std::endl;
				auto call_back_iter = m_fun_callbacks.find(msg_node->m_recvnode->m_msg_id);
				if (call_back_iter == m_fun_callbacks.end())
				{
					//û�ҵ���Ӧ�Ĵ��������Ƿ����ݣ�pop�����У�������һ����Ϣ
					m_msg_que.pop();
					continue;
				}

				//ִ�ж�Ӧ�ص�
				call_back_iter->second(msg_node->m_session, msg_node->m_recvnode->m_msg_id, std::string(msg_node->m_recvnode->m_data,msg_node->m_recvnode->m_cur_len));
				m_msg_que.pop();
			}
			break;
		}
		//�������û��ֹͣ
		auto msg_node = m_msg_que.front();
		std::cout << "recv_msg id is " << msg_node->m_recvnode->m_msg_id << std::endl;
		auto call_back_iter = m_fun_callbacks.find(msg_node->m_recvnode->m_msg_id);
		if (call_back_iter == m_fun_callbacks.end())
		{
			//û�ҵ���Ӧ�Ĵ��������Ƿ����ݣ�pop�����У�������һ����Ϣ
			m_msg_que.pop();
			continue;
		}

		//ִ�ж�Ӧ�ص�
		call_back_iter->second(msg_node->m_session, msg_node->m_recvnode->m_msg_id, std::string(msg_node->m_recvnode->m_data, msg_node->m_recvnode->m_cur_len));
		m_msg_que.pop();

	}
}

void LogicSystem::PostMsgToQue(std::shared_ptr <LogicNode> msg)
{
	std::unique_lock<std::mutex>lock(m_mtx);
	m_msg_que.push(msg);
	//����Ϣ���дӿձ仯Ϊ�ǿ�ʱ֪ͨ�������߳�,���Լ��ٲ���Ҫ���������л����̻߳���,�������߳�ͨ���첽�ص�ִ������������е���������
	if (m_msg_que.size() == 1)
	{
		lock.unlock();
		m_consume.notify_one();
	}
}

//�����¼��Ϣ
void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is " << uid << " user token is " << token << std::endl;
	
	// ��״̬������ȥ��֤
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());

	Json::Value rtvalue;
	//���ں�������ʱִ��
	Defer defer([this,&rtvalue,session]() {
		Json::StreamWriterBuilder writer;
		std::string json_data = Json::writeString(writer, rtvalue);
		std::cout << "Return String is:" << json_data;
		session->Send(json_data, MSG_CHAT_LOGIN_RSP);
		//�ذ����Զ�
		//std::string return_str = rtvalue.toStyledString();
		//std::cout << "Return String is:" << return_str;
		//session->Send(return_str,MSG_CHAT_LOGIN_RSP);
	});


	rtvalue["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::Success)
	{
		return;
	}

	//��redis�л�ȡ�û�token�Ƿ���ȷ
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	//Redis��ͨ��token_key(uid)���ҵ���Ӧ��token����ֵ����token_value��
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	//��ȡtokenʧ��
	if (!success)
	{
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	//�ͻ���Я����token��Redis��token��ƥ��
	if (token_value != token)
	{
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return;
	}

	//���ƥ�䣬�򷵻��û�������Ϣ
	rtvalue["error"] = ErrorCodes::Success;
	//��ȡ�û�������Ϣ
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
	//������redis�в�ѯ�û���Ϣ
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
		//redis��û�����ѯmysql
		//��ѯ���ݿ�
		std::shared_ptr<UserInfo>user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr)
		{
			return false;
		}
		userinfo = user_info;
		//�����ݿ�����д��redis����
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
	//ע��ص�
	RegisterCallBacks();
	//�����߳̿�ʼִ�д���
	m_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

