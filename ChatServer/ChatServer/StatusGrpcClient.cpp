#include "StatusGrpcClient.h"

StatusConPool::StatusConPool(size_t poolSize, std::string host, std::string port) :m_poolSize(poolSize), m_host(host), m_port(port), m_b_stop(false)
{
	//创建指定数量的连接
	for (size_t i = 0; i < m_poolSize; ++i)
	{
		std::shared_ptr<Channel>channel = grpc::CreateChannel(m_host + ":" + m_port, grpc::InsecureChannelCredentials());
		m_connections.push(StatusService::NewStub(channel));
	}
}

StatusConPool::~StatusConPool()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	Close();
	m_connections = std::queue<std::unique_ptr<StatusService::Stub>>();
}

std::unique_ptr<message::StatusService::Stub> StatusConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.wait(lock, [this]() {
		if (m_b_stop)
		{
			return true;
		}
		return !m_connections.empty();
		});

	if (m_b_stop)
		return nullptr;

	auto context = std::move(m_connections.front());
	m_connections.pop();
	return context;
}

void StatusConPool::returnConnection(std::unique_ptr<StatusService::Stub>context)
{
	std::lock_guard<std::mutex>lock(m_mutex);
	if (m_b_stop)
	{
		return;
	}
	m_connections.push(std::move(context));
	m_cond.notify_one();
}

void StatusConPool::Close()
{
	m_b_stop = false;
	m_cond.notify_all();
}

StatusGrpcClient::~StatusGrpcClient()
{
	
}

message::GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	//创建 ClientContext、GetChatServerReq 和 GetChatServerRsp 对象。
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;

	request.set_uid(uid);

	//从连接池中获取一个 StatusService::Stub 对象
	auto stub = m_pool->getConnection();
	//调用 GetChatServer 方法，发送请求并接收响应
	Status status = stub->GetChatServer(&context, request, &reply);

	//使用 Defer 对象确保在方法结束时将连接归还到连接池
	Defer defer([&stub, this]() {
		m_pool->returnConnection(std::move(stub));
		});

	//根据调用结果返回响应或设置错误码
	if (status.ok())
	{
		return reply;
	}
	else
	{
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

message::LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;

	request.set_uid(uid);
	request.set_token(token);

	auto stub = m_pool->getConnection();
	Status status = stub->Login(&context, request, &reply);

	Defer defer([&stub, this]() {
		m_pool->returnConnection(std::move(stub));
		});

	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	m_pool.reset(new StatusConPool(5, host, port));
}
