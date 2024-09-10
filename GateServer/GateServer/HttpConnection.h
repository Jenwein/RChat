#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend  class LogicSystem;
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	tcp::socket& GetSocket();
private:
	void CheckDeadline();	//超时检测
	void WriteResponse();	//写回应
	void PreParseGetParam();
	void HandleReq();		//处理请求
	 
	tcp::socket m_socket;
	beast::flat_buffer m_buffer{8192};
	http::request<http::dynamic_body> m_request;	//请求体
	http::response<http::dynamic_body> m_response;	//响应体
	net::steady_timer m_deadline{					//定时器
		m_socket.get_executor(),std::chrono::seconds(60)
	};
	std::string m_get_url;										//存放URL
	std::unordered_map<std::string, std::string> m_get_params;	//存放参数


};

