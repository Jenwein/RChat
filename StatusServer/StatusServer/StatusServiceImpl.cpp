#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "RedisMgr.h"

// 生成唯一字符串作为token
std::string generate_unique_string()
{
	//创建uuid对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	//将uuid转换为字符串
	std::string unique_string = boost::uuids::to_string(uuid);

	return unique_string;
}

StatusServiceImpl::StatusServiceImpl() 
{
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["ChatServers"]["Name"];
	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss,word,','))
	{
		words.push_back(word);
	}


	//将配置的服务器参数存在map中
	for (auto& server : words)
	{
		if (cfg[server.c_str()]["Name"].empty())
		{
			continue;
		}
		
		ChatServer chatserver;
		chatserver.name = cfg[server.c_str()]["Name"];
		chatserver.host = cfg[server.c_str()]["Host"];
		chatserver.port = cfg[server.c_str()]["Port"];
		m_servers[chatserver.name] = chatserver;

	}
}


//注：这几个参数在 gRPC 方法中是必需的，提供了元数据、控制信息和请求参数的访问接口，确保代码的扩展性和灵活性
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("llfc status server has received :  ");
	const auto& server = getChatServer();
	//成功获取到了服务器，设置回包（回给GateServer）内容
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());
	std::cout <<"token is :" << reply->token() << std::endl;
	return Status::OK;
}

//找到连接数最小的Server
ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex>lock(m_server_mtx);
	if (m_servers.empty()) {
		std::cerr << "No servers available "<<std::endl;
		throw std::runtime_error("No servers available");
	}
	auto minServer = m_servers.begin()->second;
	//从 Redis 中的 LOGIN_COUNT 哈希表中，获取名为 minServer.name 的服务器的连接数
	auto const_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);//HGET获取哈希表中指定字段
	if (const_str.empty())
	{
		//不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else
	{
		minServer.con_count = std::stoi(const_str);
	}    
	//遍历 _servers 容器中的所有服务器,更新minServer来获取连接数最小的服务器
	for (auto& server:m_servers)
	{
		//跳过与 minServer 同名的服务器
		if (server.second.name == minServer.name)
		{
			continue;
		}
		auto const_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);

		if (const_str.empty())
		{
			server.second.con_count = INT_MAX;
		}
		else
		{
			server.second.con_count = std::stoi(const_str);
		}

		if (server.second.con_count < minServer.con_count)
		{
			minServer = server.second;
		}
	}
	return minServer;
}

//处理用户登录请求
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	//从请求中提取用户 ID 和令牌。
	auto uid = request->uid();
	auto token = request->token();
	//构建 Redis 键并从 Redis 中获取存储的令牌值。
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key,token_value);
	//验证获取的令牌值是否与请求中的令牌值匹配。
	if (!success)
	{
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}
	if (token != token_value)
	{
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	//根据验证结果设置响应中的错误码，并返回相应的状态。
	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}
