#pragma once
#include "const.h"
#include <boost/asio.hpp>

class CSession;

class CServer:public std::enable_shared_from_this<CServer>
{
public:
	//��ʼ������������ʼ����
	CServer(boost::asio::io_context& ioc, unsigned short port);
	~CServer();
	//����ָ���Ự
	void ClearSession(std::string);
private:
	//������ܵ�������
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void StartAccept();
	
private:
	tcp::acceptor m_acceptor;	//������
	net::io_context& m_ioc;
	short m_port;
	std::map<std::string,std::shared_ptr<CSession>> m_sessions;
	std::mutex m_mtx;
};

