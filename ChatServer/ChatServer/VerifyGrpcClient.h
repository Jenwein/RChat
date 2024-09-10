#pragma once
#include <grpcpp/grpcpp.h>

#include "const.h"
#include "Singleton.h"
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifyReq;	//验证请求
using message::GetVerifyRsp;	//验证回复
using message::VerifyService;	//验证服务

class RPConpool
{
public:
	RPConpool(size_t poolsize,std::string host,std::string port);

	~RPConpool();

	void Close();

	std::unique_ptr<VerifyService::Stub>getConnection();

	void returnConnection(std::unique_ptr<VerifyService::Stub>context);

private:
	std::atomic<bool> m_b_stop;
	size_t m_poolSize;
	std::string m_host;
	std::string m_port;
	std::condition_variable m_cond;
	std::mutex m_mutex;
	std::queue<std::unique_ptr<VerifyService::Stub>> m_connections;
};

// gRPC客户端，获取验证码
class VerifyGrpcClient:public Singleton< VerifyGrpcClient>
{
	//声明友元，可以访问 VerifyGrpcClient 的私有构造函数
	friend class Singleton <VerifyGrpcClient>;

public:
	//发送 gRPC 请求，获取验证码
	GetVerifyRsp GetVerifyCode(std::string email);


private:
	//std::unique_ptr<VerifyService::Stub> m_stub;
	std::unique_ptr<RPConpool> m_pool;
	VerifyGrpcClient();

};

