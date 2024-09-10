#include "CServer.h"
#include "CSession.h"
#include "AsioIOServicePool.h"


CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
	:m_ioc(ioc),m_acceptor(ioc,tcp::endpoint(tcp::v4(),port))
{
	std::cout << "Server start success,listen on Port : " << port;
	StartAccept();
}

CServer::~CServer()
{

}

void CServer::ClearSession(std::string uuid)
{
	if (m_sessions.find(uuid) != m_sessions.end())
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_sessions.erase(uuid);
	}

}

//����������ӣ�
void CServer::HandleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& error)
{
	if (!error)
	{
		{
			session->Start();
			std::lock_guard<std::mutex> lock(m_mtx);
			m_sessions.insert(std::make_pair(session->GetSessionID(), session));
		}
	}
	else
	{
		std::cout << "session accept failed,error is" << error.what() << std::endl;
	}
	//������һ�ν�������
	StartAccept();
}

//�����첽���ܲ���
void CServer::StartAccept()
{
	auto& ioc = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(ioc,this);
	
	//�����첽������������¼���ִ��HandleAccept
	m_acceptor.async_accept(new_session->GetSocket(),
		std::bind(&CServer::HandleAccept,this,
			new_session,std::placeholders::_1));
}
