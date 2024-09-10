#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <memory>
#include <iostream>
#include <map>
#include <unordered_map>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

#include "Singleton.h"

#include "hiredis.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes
{
	Success = 0,
	Error_Json = 1001,		//json解析错误
	RPCFailed = 1002,		//RPC请求错误
	VerifyExpired = 1003,	//验证码过期
	VerifyCodeErr = 1004,	//验证码错误
	UserExist = 1005,		//用户已存在
	PasswdErr = 1006,		//密码错误
	EmailNotMatch = 1007,	//邮箱不匹配
	PasswdUpFailed = 1008,	//更新密码失败
	PasswdInvalid = 1009,	//密码无效

};

#define CODEPREFIX  "code_"

//Defer类
class Defer
{
public:
	//接受一个函数对象
	Defer(std::function<void()>func) :m_func(func){}

	~Defer()
	{
		//析构中执行传入的可执行对象
		m_func();
	}

private:
	std::function<void()>m_func;
};