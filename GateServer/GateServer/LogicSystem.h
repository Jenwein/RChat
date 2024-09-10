#pragma once
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	bool HandleGet(std::string,std::shared_ptr<HttpConnection>);	//����Get����,arg1-url
	bool HandlePost(std::string path, std::shared_ptr<HttpConnection> connection);
	void RegGet(std::string, HttpHandler handler);		//ע��Get����

	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> m_post_handlers;
	std::map<std::string, HttpHandler> m_get_handlers;
};

