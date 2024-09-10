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

//gRPC ���ӳ�,���ڹ��� StatusService ����Ŀͻ�������
class StatusConPool 
{
public:
	//���캯����ʼ�����ӳأ�����ָ�������� gRPC ���Ӳ������Ǵ洢�� connections_ ������
	StatusConPool(size_t poolSize,std::string host,std::string port);

	~StatusConPool();

	std::unique_ptr<StatusService::Stub> getConnection();

	void returnConnection(std::unique_ptr<StatusService::Stub>context);

	void Close();

private:
	std::atomic<bool> m_b_stop;		//ָʾ���ӳ��Ƿ�ֹͣ
	size_t m_poolSize;				//���ӳصĴ�С
	std::string m_host;				//gRPC ����������Ͷ˿�
	std::string m_port;				
	std::queue<std::unique_ptr<StatusService::Stub>> m_connections;	//�洢���õ� StatusService::Stub ����
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

//�� gRPC �������ͨ��
class StatusGrpcClient:public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient();

	//��ȡ�����������Ϣ
	GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();

	std::unique_ptr<StatusConPool> m_pool;	//���� gRPC ���ӳ�

};

