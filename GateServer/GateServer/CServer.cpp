#include "CServer.h"
#include "AsioIOServicePool.h"
#include "HttpConnection.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
	:m_ioc(ioc),m_acceptor(ioc,tcp::endpoint(tcp::v4(),port)),m_socket(ioc)
{

}

void CServer::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);	
	m_acceptor.async_accept(new_con->GetSocket(), [self,new_con](beast::error_code ec) {
		try
		{
			if (ec)
			{
				//如果错误，放弃当前连接，继续监听其他连接
				self->Start();
				return;
			}
			//创建新连接，并且创建HttpConnection类管理这个连接
			//std::make_shared<HttpConnection>(std::move(self->m_socket))->Start();
			new_con->Start();

			//继续监听
			self->Start();
		}
		catch (const std::exception& e)
		{
			std::cerr << "" << e.what() << std::endl;
		}
		}
	);
}
