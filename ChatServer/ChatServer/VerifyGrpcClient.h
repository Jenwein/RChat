#pragma once
#include <grpcpp/grpcpp.h>

#include "const.h"
#include "Singleton.h"
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifyReq;	//��֤����
using message::GetVerifyRsp;	//��֤�ظ�
using message::VerifyService;	//��֤����

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

// gRPC�ͻ��ˣ���ȡ��֤��
class VerifyGrpcClient:public Singleton< VerifyGrpcClient>
{
	//������Ԫ�����Է��� VerifyGrpcClient ��˽�й��캯��
	friend class Singleton <VerifyGrpcClient>;

public:
	//���� gRPC ���󣬻�ȡ��֤��
	GetVerifyRsp GetVerifyCode(std::string email);


private:
	//std::unique_ptr<VerifyService::Stub> m_stub;
	std::unique_ptr<RPConpool> m_pool;
	VerifyGrpcClient();

};

