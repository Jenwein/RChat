/*
ʵ��һ������ gRPC ���������������
	��ȡ�����������Ϣ��ͨ�� GetChatServer �������ͻ��˿��������ȡ��ǰ���õ��������������Ϣ��
	�û���¼��ͨ�� Login �������ͻ��˿��Է��͵�¼���󣬲����յ�¼��Ӧ��
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

//�洢����������������Ͷ˿���Ϣ
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
	
	std::string host;	//����
	std::string port;	//�˿�
	std::string name;	//����
	int con_count;		//������
};

//ʵ�� gRPC ���� StatusService,�����ṩ�������������Ϣ
class StatusServiceImpl final:public StatusService::Service	//���ɼ̳е�������
{
public:
	//��ʼ������,�����ù������ж�ȡ����������������Ͷ˿���Ϣ
	StatusServiceImpl();
	//�����ȡ���������������
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)override;
	//�����¼����
	Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)override;

private:
	//�����û�����
	void insertToken(int uid, std::string token);
	//��ȡ�����������Ϣ
	ChatServer getChatServer();

private:
	std::unordered_map<std::string, ChatServer> m_servers;		//�洢�����������Ϣ<uid,ChatServer>
	std::mutex m_server_mtx;				
	
};

