#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

RPConpool::RPConpool(size_t poolsize, std::string host, std::string port) :m_poolSize(poolsize), m_host(host), m_port(port), m_b_stop(false)
{
	for (size_t i = 0; i < m_poolSize; ++i)
	{
		//创建一个 gRPC 通道，连接到 127.0.0.1:50051，使用不安全的通道凭证
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
		//使用创建的通道初始化 m_stub，构造 VerifyService::Stub 对象
		//push一个右值则对应右值版本的移动构造
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
	ClientContext context;	//客户端上下文
	GetVerifyRsp reply;		//创建响应对象，用于存储服务器返回的数据
	GetVerifyReq request;	//创建请求对象，用于发送给服务器

	request.set_email(email); //设置请求参数（邮箱）

	//发送请求并接收响应
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
	std::string host = gCfgMgr["VerifyServer"]["Host"];	// 取验证ip
	std::string port = gCfgMgr["VerifyServer"]["Port"];	// 取验证服务端口
	m_pool.reset(new RPConpool(5,host,port));		//初始化5个连接，并指定初始host和port
}
