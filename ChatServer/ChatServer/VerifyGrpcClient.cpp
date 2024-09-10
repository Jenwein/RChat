#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

RPConpool::RPConpool(size_t poolsize, std::string host, std::string port) :m_poolSize(poolsize), m_host(host), m_port(port), m_b_stop(false)
{
	for (size_t i = 0; i < m_poolSize; ++i)
	{
		//����һ�� gRPC ͨ�������ӵ� 127.0.0.1:50051��ʹ�ò���ȫ��ͨ��ƾ֤
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
		//ʹ�ô�����ͨ����ʼ�� m_stub������ VerifyService::Stub ����
		//pushһ����ֵ���Ӧ��ֵ�汾���ƶ�����
		m_connections.push(VerifyService::NewStub(channel));
	}
}

RPConpool::~RPConpool()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	Close();
	while (!m_connections.empty())
	{
		m_connections.pop();
	}
}

void RPConpool::Close()
{
	m_b_stop = true;
	m_cond.notify_all();
}

std::unique_ptr<message::VerifyService::Stub> RPConpool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.wait(lock, [this]() {

		if (m_b_stop)
			return true;

		return !m_connections.empty();
		}
	);
	if (m_b_stop)
	{
		return nullptr;
	}
	auto context = std::move(m_connections.front());
	m_connections.pop();
	return context;
}

void RPConpool::returnConnection(std::unique_ptr<VerifyService::Stub>context)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_b_stop)
	{
		return;
	}

	m_connections.push(std::move(context));
	m_cond.notify_one();
}

message::GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email)
{
	ClientContext context;	//�ͻ���������
	GetVerifyRsp reply;		//������Ӧ�������ڴ洢���������ص�����
	GetVerifyReq request;	//��������������ڷ��͸�������

	request.set_email(email); //����������������䣩

	//�������󲢽�����Ӧ
	auto stub = m_pool->getConnection();
	Status status = stub->GetVerifyCode(&context, request, &reply);
	if (status.ok())
	{
		m_pool->returnConnection(std::move(stub));
		return reply;
	}
	else
	{
		m_pool->returnConnection(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VerifyGrpcClient::VerifyGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VerifyServer"]["Host"];	// ȡ��֤ip
	std::string port = gCfgMgr["VerifyServer"]["Port"];	// ȡ��֤����˿�
	m_pool.reset(new RPConpool(5,host,port));		//��ʼ��5�����ӣ���ָ����ʼhost��port
}
