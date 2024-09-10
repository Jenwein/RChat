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
	Error_Json = 1001,		//json��������
	RPCFailed = 1002,		//RPC�������
	VerifyExpired = 1003,	//��֤�����
	VerifyCodeErr = 1004,	//��֤�����
	UserExist = 1005,		//�û��Ѵ���
	PasswdErr = 1006,		//�������
	EmailNotMatch = 1007,	//���䲻ƥ��
	PasswdUpFailed = 1008,	//��������ʧ��
	PasswdInvalid = 1009,	//������Ч

};

#define CODEPREFIX  "code_"

//Defer��
class Defer
{
public:
	//����һ����������
	Defer(std::function<void()>func) :m_func(func){}

	~Defer()
	{
		//������ִ�д���Ŀ�ִ�ж���
		m_func();
	}

private:
	std::function<void()>m_func;
};