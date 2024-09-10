#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

#include "const.h"
#include "ConfigMgr.h"
#include "hiredis.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "AsioIOServicePool.h"
#include "StatusServiceImpl.h"

void RunServer()
{
	auto& cfg = ConfigMgr::Inst();
	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
	
	StatusServiceImpl service;
	
	//添加监听端口和服务。
	grpc::ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	//构建并启动gRPC服务器
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

//这里为了优雅的关闭服务器，使用io_context的信号来关闭
	//创建Boost::Asio的io_context
	boost::asio::io_context io_context;
	//创建signal_set用于捕获SIGINT
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

	//设置异步等待的信号
	signals.async_wait([&server](const boost::system::error_code& error,int signal_number){
		if (!error)
		{
			std::cout << "Shutting down server..." << std::endl;
			server->Shutdown();	//优雅关闭服务器
		}	
	});

	//在单独的线程中运行io_context
	std::thread([&io_context]() {io_context.run(); }).detach();

	//等待服务器关闭
	server->Wait();
	//停止io_context
	io_context.stop();
}


int main(int argc, char** argv)
{
	try
	{
		RunServer();
		RedisMgr::GetInstance()->Close();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;

}