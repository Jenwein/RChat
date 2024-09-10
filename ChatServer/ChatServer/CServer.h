#pragma once
#include "const.h"
#include <boost/asio.hpp>

class CSession;

class CServer:public std::enable_shared_from_this<CServer>
{
public:
	//初始化服务器并开始监听
	CServer(boost::asio::io_context& ioc, unsigned short port);
	~CServer();
	//清理指定会话
	void ClearSession(std::string);
private:
	//处理接受到的连接
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void StartAccept();
	
private:
	tcp::acceptor m_acceptor;	//监听器
	net::io_context& m_ioc;
	short m_port;
	std::map<std::string,std::shared_ptr<CSession>> m_sessions;
	std::mutex m_mtx;
};

