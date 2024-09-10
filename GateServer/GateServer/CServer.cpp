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
				//������󣬷�����ǰ���ӣ�����������������
				self->Start();
				return;
			}
			//���������ӣ����Ҵ���HttpConnection������������
			//std::make_shared<HttpConnection>(std::move(self->m_socket))->Start();
			new_con->Start();

			//��������
			self->Start();
		}
		catch (const std::exception& e)
		{
			std::cerr << "" << e.what() << std::endl;
		}
		}
	);
}
