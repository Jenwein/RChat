#include <iostream>
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include <assert.h> 

int main()
{
	//TestRedisMgr();

	// 获取配置信息
	auto& gCfgMgr = ConfigMgr::Inst();		
	std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
	unsigned short gate_port = atoi(gate_port_str.c_str());

	try
	{
		unsigned short port = static_cast<unsigned short>(8080);
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error,int sig_num){
				if (error)
				{
					return;
				}
				ioc.stop();
			}
		);
		std::make_shared<CServer>(ioc, port)->Start();
		std::cout << "Gate Server listen on port: " << port << std::endl;
		ioc.run();
	}
	catch (const std::exception& e)
	{
		std::cout <<"Error:" << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "OK";
	std::cin.get();
}