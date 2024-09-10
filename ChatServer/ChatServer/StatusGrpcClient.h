#pragma once

#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"

#include "message.pb.h"
#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;
using message::LoginRsp;
using message::LoginReq;

//gRPC 连接池,用于管理 StatusService 服务的客户端连接
class StatusConPool 
{
public:
	//构造函数初始化连接池，创建指定数量的 gRPC 连接并将它们存储在 connections_ 队列中
	StatusConPool(size_t poolSize,std::string host,std::string port);

	~StatusConPool();

	std::unique_ptr<StatusService::Stub> getConnection();

	void returnConnection(std::unique_ptr<StatusService::Stub>context);

	void Close();

private:
	std::atomic<bool> m_b_stop;		//指示连接池是否停止
	size_t m_poolSize;				//连接池的大小
	std::string m_host;				//gRPC 服务的主机和端口
	std::string m_port;				
	std::queue<std::unique_ptr<StatusService::Stub>> m_connections;	//存储可用的 StatusService::Stub 连接
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

//与 gRPC 服务进行通信
class StatusGrpcClient:public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient();

	//获取聊天服务器信息
	GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();

	std::unique_ptr<StatusConPool> m_pool;	//管理 gRPC 连接池

};

