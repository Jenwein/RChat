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
	// 发送消息
	void Send(char* msg, short max_length, short msgid);
	void Send(std::string msg, short msgid);

	void Close();
	//获取共享指针
	std::shared_ptr<CSession> SharedSelf();
	
	void Start();
	// 异步读取头部数据
	void AsyncReadHead(int total_len);
	// 异步读取消息体数据
	void AsyncReadBody(int total_len);

private:
	// 异步读取指定长度的数据
	void AsyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)>handler);
	// 异步读取完整长度的数据
	void AsyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code&, std::size_t)>handler);
	// 处理写操作完成
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
private:
	tcp::socket m_socket;
	std::string m_session_id;
	char m_data[MAX_LENGTH];
	CServer* m_server;
	//发送队列
	std::queue<std::shared_ptr<SendNode>> m_send_que;
	
	bool m_b_close;
	std::mutex m_send_lock;
	
	//收到的消息结构（头部+消息体）
	std::shared_ptr<RecvNode> m_recv_msg_node;
	//收到的头部结构
	std::shared_ptr<MsgNode> m_recv_head_node;

	// 头部解析标志
	bool m_b_head_parse;
	int m_user_uid;
};

class LogicNode {
	friend class LogicSystem;
public:
	// 构造函数，初始化会话和接收节点
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
	std::shared_ptr<CSession> m_session;
	std::shared_ptr<RecvNode> m_recvnode;
};