/*
实现一个基于 gRPC 的聊天服务器服务
	获取聊天服务器信息：通过 GetChatServer 方法，客户端可以请求获取当前可用的聊天服务器的信息。
	用户登录：通过 Login 方法，客户端可以发送登录请求，并接收登录响应。
*/

#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;
using message::LoginReq;
using message::LoginRsp;

//存储聊天服务器的主机和端口信息
class ChatServer {
public:
	ChatServer()
		:host(""), port(""), name(""), con_count(0)
	{}

	ChatServer(const ChatServer& cs)
		:host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count)
	{}
	
	ChatServer& operator=(const ChatServer& cs)
	{
		if (&cs == this)
		{
			return *this;
		}
		host = cs.host;
		name = cs.name;
		port = cs.port;
		con_count = cs.con_count;
		return *this;
	}
	
	std::string host;	//主机
	std::string port;	//端口
	std::string name;	//名称
	int con_count;		//连接数
};

//实现 gRPC 服务 StatusService,用于提供聊天服务器的信息
class StatusServiceImpl final:public StatusService::Service	//不可继承的最终类
{
public:
	//初始化服务,从配置管理器中读取聊天服务器的主机和端口信息
	StatusServiceImpl();
	//处理获取聊天服务器的请求
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)override;
	//处理登录请求
	Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)override;

private:
	//插入用户令牌
	void insertToken(int uid, std::string token);
	//获取聊天服务器信息
	ChatServer getChatServer();

private:
	std::unordered_map<std::string, ChatServer> m_servers;		//存储聊天服务器信息<uid,ChatServer>
	std::mutex m_server_mtx;				
	
};

