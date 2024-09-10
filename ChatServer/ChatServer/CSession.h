#pragma once
#include "const.h"
#include "MsgNode.h"


class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);
	~CSession();
	
	tcp::socket& GetSocket();
	std::string& GetSessionID();
	void SetUserID(int);
	int GetUserID();
	// ������Ϣ
	void Send(char* msg, short max_length, short msgid);
	void Send(std::string msg, short msgid);

	void Close();
	//��ȡ����ָ��
	std::shared_ptr<CSession> SharedSelf();
	
	void Start();
	// �첽��ȡͷ������
	void AsyncReadHead(int total_len);
	// �첽��ȡ��Ϣ������
	void AsyncReadBody(int total_len);

private:
	// �첽��ȡָ�����ȵ�����
	void AsyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)>handler);
	// �첽��ȡ�������ȵ�����
	void AsyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code&, std::size_t)>handler);
	// ����д�������
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
private:
	tcp::socket m_socket;
	std::string m_session_id;
	char m_data[MAX_LENGTH];
	CServer* m_server;
	//���Ͷ���
	std::queue<std::shared_ptr<SendNode>> m_send_que;
	
	bool m_b_close;
	std::mutex m_send_lock;
	
	//�յ�����Ϣ�ṹ��ͷ��+��Ϣ�壩
	std::shared_ptr<RecvNode> m_recv_msg_node;
	//�յ���ͷ���ṹ
	std::shared_ptr<MsgNode> m_recv_head_node;

	// ͷ��������־
	bool m_b_head_parse;
	int m_user_uid;
};

class LogicNode {
	friend class LogicSystem;
public:
	// ���캯������ʼ���Ự�ͽ��սڵ�
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
	std::shared_ptr<CSession> m_session;
	std::shared_ptr<RecvNode> m_recvnode;
};