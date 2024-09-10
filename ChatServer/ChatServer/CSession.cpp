#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	:m_data{0},m_socket(ioc), m_server(server), m_b_close(false), m_b_head_parse(false), m_user_uid(0)
{
	//生成uuid
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	//转字符串
	m_session_id = boost::uuids::to_string(a_uuid);
	m_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);

}

CSession::~CSession()
{
	std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket()
{
	return m_socket;
}

std::string& CSession::GetSessionID()
{
	return m_session_id;
}

void CSession::SetUserID(int uid)
{
	m_user_uid = uid;
}

int CSession::GetUserID()
{
	return m_user_uid;
}

void CSession::Close()
{
	m_socket.close();
	m_b_close = true;
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
	return shared_from_this();
}

/*
任一时刻只会有一个异步写操作在进行。这种方式确保了消息按顺序发送，
避免了多个异步写操作同时进行，防止数据冲突和不一致
*/
void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(m_send_lock);
	int send_que_size = m_send_que.size();
	//判断队列当前大小，防止队列过载
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << m_session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	//先将当前信息构建为发送节点插入发送队列
	m_send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
	//判断插入前队列是否有数据，如果插入前队列内已经有了其他待发送节点，则直接退出
	//也就是说如果发送队列在添加新消息之前已经有其他消息，说明已经有一个异步写操作正在进行，所以为了避免启动新的异步写操作，防止多个异步写操作同时进行，会直接退出
	if (send_que_size > 0) {
		return;
	}
	//如果插入前队列没有数据，则当前队列中只有这一个数据，所以取出并发送该节点
	auto& msgnode = m_send_que.front();
	//异步写数据，写完后调用HandleWrite，在HandleWrite中会一直回调直到队列元素都发送完毕
	boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->m_data, msgnode->m_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(m_send_lock);
	int send_que_size = m_send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << m_session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	m_send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = m_send_que.front();

	std::cout << "msg:" << msg << std::endl;
	std::string str(msgnode->m_data+4, msgnode->m_total_len);
	std::cout << "str:" << str << std::endl;
	boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->m_data, msgnode->m_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self)
{
	//进入该处理函数表示发送节点已完成
	try
	{
		//如果写事件没有错误
		if (!error)
		{
			std::lock_guard<std::mutex> lock(m_send_lock);
			//将已发送节点pop出队列
			m_send_que.pop();
			//判断队列是否为空
			if (!m_send_que.empty())
			{
				//如果pop后队列非空表示内部还有节点待发送，取出队首节点，继续发送
				auto& msgnode = m_send_que.front();
				boost::asio::async_write(m_socket, boost::asio::buffer(msgnode->m_data, msgnode->m_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1,shared_self));
			}
		}
		else
		{
			std::cout << "handle write failed,error is " << error.what() << std::endl;
			Close();
			m_server->ClearSession(m_session_id);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception code: " << e.what() << std::endl;
	}
}

void CSession::Start()
{
	AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::AsyncReadHead(int total_len)
{
	auto self = shared_from_this();
	AsyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec,std::size_t bytes_transfered){		
		try
		{
			//发生错误
			if (ec)
			{
				std::cout << "handle read failed,error is " << ec.what() << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}
			/*
			 * 因为这读取到足够长度的数据已经在AsyncReadFull中被指定且管理，
			 * 在AsyncReadLen中会保证读取够这些长度的数据，
			 * 但是如果最后将数据返回检查的时候发现读取的长度与要求的长度不匹配，
			 * 所以并不是没有读够数据，而是出现了一些错误导致的问题，我这么理解对吗
			 */
			//已读取的字节数不足一个头部长度
			if (bytes_transfered < HEAD_TOTAL_LEN)
			{
				std::cout << "read length not match,read[" << bytes_transfered << "],total [ " << HEAD_TOTAL_LEN << "]" << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}

		//将数据打包成数据节点
			//构造消息头部节点
			m_recv_head_node->Clear();
			memcpy(m_recv_head_node->m_data, m_data, bytes_transfered);

			//获取头部ID字段
			short msg_id = 0;
			memcpy(&msg_id, m_recv_head_node->m_data, HEAD_ID_LEN);
			//网络字节序转换本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
			if (msg_id > MAX_LENGTH)
			{
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				m_server->ClearSession(m_session_id);
				return;
			}

			//获取头部长度字段
			short msg_len = 0;
			memcpy(&msg_len, m_recv_head_node->m_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//转换字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			if (msg_len > MAX_LENGTH)
			{
				std::cout << "invalid data length is " << msg_len << std::endl;
				m_server->ClearSession(m_session_id);
				return;
			}

			//构造一个接收的消息节点
			m_recv_msg_node = std::make_shared<RecvNode>(msg_len,msg_id);
			//读取消息头完成，开始根据规定长度读取消息体
			AsyncReadBody(msg_len);

		}
		catch (const std::exception& e)
		{
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});
}

void CSession::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	//读取到全部数据后进入lambda
	AsyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try
		{
			if (ec)
			{
				std::cout << "handle read failed ,error is " << ec.what() << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}

			if (bytes_transfered < total_len)
			{
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< total_len << "]" << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}

			//此时的m_data中为一个完整的消息，将其全部放入消息节点中，并更新消息当前长度，并完善消息数组
			memcpy(m_recv_msg_node->m_data, m_data, bytes_transfered);
			m_recv_msg_node->m_cur_len += bytes_transfered;
			m_recv_msg_node->m_data[m_recv_msg_node->m_total_len] = '\0';
			std::cout << "receive data is " << m_recv_msg_node->m_data << std::endl;
			//将消息节点投递到逻辑队列中进行处理
			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), m_recv_msg_node));
			//继续监听头部接收事件
			AsyncReadHead(total_len);
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});

}

//读取完整长度
void CSession::AsyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code&, std::size_t)>handler)
{
	//初始化
	memset(m_data, 0, max_length);
	//读取指定长度
	AsyncReadLen(0, max_length, handler);
}
//读取指定长度的数据
//read_len：已经读取的数据长度。
//total_len：需要读取的总数据长度。
//handler：读取完成后的回调函数
void CSession::AsyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)>handler)
{
	auto self = shared_from_this();
	//从socket读取数据到m_data中，从read_len开始，读取剩余需要的长度total_len。
	//buffer的第二个参数表示buffer的大小，即async_read_some读取的最大长度
	m_socket.async_read_some(boost::asio::buffer(m_data + read_len,total_len-read_len),[this,self,read_len,total_len,handler](const boost::system::error_code& ec,std::size_t bytes_transfered){
		if (ec)
		{
			//出现错误
			handler(ec, read_len + bytes_transfered);
			return;
		}

		if (read_len + bytes_transfered >= total_len)
		{
			//已读取足够
			handler(ec, read_len + bytes_transfered);//已读取长度+传输的长度即所需的长度
			return;
		}
		//没有出现错误，但仍未读完要求长度
		//更新已读取的长度		read_len=  ↓，total_len会在async_read_some中减去更新后的read_len来读取剩余字节
		self->AsyncReadLen(read_len + bytes_transfered, total_len, handler);	
	});
}


LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	:m_session(session),m_recvnode(recvnode)
{

}
