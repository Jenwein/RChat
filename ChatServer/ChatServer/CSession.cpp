#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	:m_data{0},m_socket(ioc), m_server(server), m_b_close(false), m_b_head_parse(false), m_user_uid(0)
{
	//����uuid
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	//ת�ַ���
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
��һʱ��ֻ����һ���첽д�����ڽ��С����ַ�ʽȷ������Ϣ��˳���ͣ�
�����˶���첽д����ͬʱ���У���ֹ���ݳ�ͻ�Ͳ�һ��
*/
void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(m_send_lock);
	int send_que_size = m_send_que.size();
	//�ж϶��е�ǰ��С����ֹ���й���
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << m_session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	//�Ƚ���ǰ��Ϣ����Ϊ���ͽڵ���뷢�Ͷ���
	m_send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
	//�жϲ���ǰ�����Ƿ������ݣ��������ǰ�������Ѿ��������������ͽڵ㣬��ֱ���˳�
	//Ҳ����˵������Ͷ������������Ϣ֮ǰ�Ѿ���������Ϣ��˵���Ѿ���һ���첽д�������ڽ��У�����Ϊ�˱��������µ��첽д��������ֹ����첽д����ͬʱ���У���ֱ���˳�
	if (send_que_size > 0) {
		return;
	}
	//�������ǰ����û�����ݣ���ǰ������ֻ����һ�����ݣ�����ȡ�������͸ýڵ�
	auto& msgnode = m_send_que.front();
	//�첽д���ݣ�д������HandleWrite����HandleWrite�л�һֱ�ص�ֱ������Ԫ�ض��������
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
	//����ô�������ʾ���ͽڵ������
	try
	{
		//���д�¼�û�д���
		if (!error)
		{
			std::lock_guard<std::mutex> lock(m_send_lock);
			//���ѷ��ͽڵ�pop������
			m_send_que.pop();
			//�ж϶����Ƿ�Ϊ��
			if (!m_send_que.empty())
			{
				//���pop����зǿձ�ʾ�ڲ����нڵ�����ͣ�ȡ�����׽ڵ㣬��������
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
			//��������
			if (ec)
			{
				std::cout << "handle read failed,error is " << ec.what() << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}
			/*
			 * ��Ϊ���ȡ���㹻���ȵ������Ѿ���AsyncReadFull�б�ָ���ҹ���
			 * ��AsyncReadLen�лᱣ֤��ȡ����Щ���ȵ����ݣ�
			 * �������������ݷ��ؼ���ʱ���ֶ�ȡ�ĳ�����Ҫ��ĳ��Ȳ�ƥ�䣬
			 * ���Բ�����û�ж������ݣ����ǳ�����һЩ�����µ����⣬����ô������
			 */
			//�Ѷ�ȡ���ֽ�������һ��ͷ������
			if (bytes_transfered < HEAD_TOTAL_LEN)
			{
				std::cout << "read length not match,read[" << bytes_transfered << "],total [ " << HEAD_TOTAL_LEN << "]" << std::endl;
				Close();
				m_server->ClearSession(m_session_id);
				return;
			}

		//�����ݴ�������ݽڵ�
			//������Ϣͷ���ڵ�
			m_recv_head_node->Clear();
			memcpy(m_recv_head_node->m_data, m_data, bytes_transfered);

			//��ȡͷ��ID�ֶ�
			short msg_id = 0;
			memcpy(&msg_id, m_recv_head_node->m_data, HEAD_ID_LEN);
			//�����ֽ���ת�������ֽ���
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
			if (msg_id > MAX_LENGTH)
			{
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				m_server->ClearSession(m_session_id);
				return;
			}

			//��ȡͷ�������ֶ�
			short msg_len = 0;
			memcpy(&msg_len, m_recv_head_node->m_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//ת���ֽ���
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			if (msg_len > MAX_LENGTH)
			{
				std::cout << "invalid data length is " << msg_len << std::endl;
				m_server->ClearSession(m_session_id);
				return;
			}

			//����һ�����յ���Ϣ�ڵ�
			m_recv_msg_node = std::make_shared<RecvNode>(msg_len,msg_id);
			//��ȡ��Ϣͷ��ɣ���ʼ���ݹ涨���ȶ�ȡ��Ϣ��
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
	//��ȡ��ȫ�����ݺ����lambda
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

			//��ʱ��m_data��Ϊһ����������Ϣ������ȫ��������Ϣ�ڵ��У���������Ϣ��ǰ���ȣ���������Ϣ����
			memcpy(m_recv_msg_node->m_data, m_data, bytes_transfered);
			m_recv_msg_node->m_cur_len += bytes_transfered;
			m_recv_msg_node->m_data[m_recv_msg_node->m_total_len] = '\0';
			std::cout << "receive data is " << m_recv_msg_node->m_data << std::endl;
			//����Ϣ�ڵ�Ͷ�ݵ��߼������н��д���
			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), m_recv_msg_node));
			//��������ͷ�������¼�
			AsyncReadHead(total_len);
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});

}

//��ȡ��������
void CSession::AsyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code&, std::size_t)>handler)
{
	//��ʼ��
	memset(m_data, 0, max_length);
	//��ȡָ������
	AsyncReadLen(0, max_length, handler);
}
//��ȡָ�����ȵ�����
//read_len���Ѿ���ȡ�����ݳ��ȡ�
//total_len����Ҫ��ȡ�������ݳ��ȡ�
//handler����ȡ��ɺ�Ļص�����
void CSession::AsyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)>handler)
{
	auto self = shared_from_this();
	//��socket��ȡ���ݵ�m_data�У���read_len��ʼ����ȡʣ����Ҫ�ĳ���total_len��
	//buffer�ĵڶ���������ʾbuffer�Ĵ�С����async_read_some��ȡ����󳤶�
	m_socket.async_read_some(boost::asio::buffer(m_data + read_len,total_len-read_len),[this,self,read_len,total_len,handler](const boost::system::error_code& ec,std::size_t bytes_transfered){
		if (ec)
		{
			//���ִ���
			handler(ec, read_len + bytes_transfered);
			return;
		}

		if (read_len + bytes_transfered >= total_len)
		{
			//�Ѷ�ȡ�㹻
			handler(ec, read_len + bytes_transfered);//�Ѷ�ȡ����+����ĳ��ȼ�����ĳ���
			return;
		}
		//û�г��ִ��󣬵���δ����Ҫ�󳤶�
		//�����Ѷ�ȡ�ĳ���		read_len=  ����total_len����async_read_some�м�ȥ���º��read_len����ȡʣ���ֽ�
		self->AsyncReadLen(read_len + bytes_transfered, total_len, handler);	
	});
}


LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	:m_session(session),m_recvnode(recvnode)
{

}
