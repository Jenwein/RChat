#pragma once
#include "Singleton.h"
#include "const.h"

class RedisConPool {
public:
	RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
		:m_poolSize(poolSize),m_host(host),m_port(port),m_b_stop(false) 
	{
		for (size_t i = 0; i < m_poolSize; ++i) {
			auto* context = redisConnect(host, port);
			if (context == nullptr || context->err != 0) {
				if (context != nullptr) {
					redisFree(context);
				}
				continue;
			}
			auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
			if (reply->type == REDIS_REPLY_ERROR) {
				std::cout << "认证失败" << std::endl;
				//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
				freeReplyObject(reply);
				continue;
			}
			//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
			freeReplyObject(reply);
			std::cout << "认证成功" << std::endl;
			m_connections.push(context);
		}
	}
	~RedisConPool() 
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		//使用空队列赋值来清空队列
		m_connections = std::queue<redisContext*>();
	}
	redisContext* getConnection() 
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cond.wait(lock, [this] {
			if (m_b_stop) {
				return true;
			}
			return !m_connections.empty();
			});
		//如果停止则直接返回空指针
		if (m_b_stop) {
			return  nullptr;
		}
		auto* context = m_connections.front();
		m_connections.pop();
		return context;
	}
	void returnConnection(redisContext* context) 
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_b_stop) {
			return;
		}
		m_connections.push(context);
		m_cond.notify_one();
	}
	void Close() 
	{
		m_b_stop = true;
		m_cond.notify_all();
	}
private:
	std::atomic<bool> m_b_stop;
	size_t m_poolSize;
	const char* m_host;
	int m_port;
	std::queue<redisContext*> m_connections;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};


//封装Redis操作
//Redis管理类
class RedisMgr :public Singleton<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Auth(const std::string& password);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close();

private:
	RedisMgr();

	std::unique_ptr<RedisConPool> m_con_pool;

	redisContext* m_connect;
	redisReply* m_reply;
};
