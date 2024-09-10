#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"

LogicSystem::~LogicSystem()
{

}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> connection)
{
	if (m_get_handlers.find(path) == m_get_handlers.end())
		return false;

	m_get_handlers[path](connection);	//���ô���
	return true;

}
bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> connection)
{
	if (m_post_handlers.find(path) == m_post_handlers.end())
		return false;

	m_post_handlers[path](connection);	//���ô���
	return true;

}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
	m_get_handlers.insert(std::make_pair(url, handler));
}
void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
	m_post_handlers.insert(std::make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
	RegGet("/get_test", [](std::shared_ptr<HttpConnection>connection) {
		beast::ostream(connection->m_response.body())<<"receive get_test req";
		int i = 0;
		for (auto& elem:connection->m_get_params)
		{
			i++;
			beast::ostream(connection->m_response.body()) << ",param " << i << "key is " << elem.first;
			beast::ostream(connection->m_response.body()) << ",param " << i << "value is " << elem.second;
		}

		}
	);

	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection>connection) {
		//��ȡ������
		auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->m_response.set(http::field::content_type, "text/json");
		//������������
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email"))
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		//ȡ���ͻ��˷��͵����䣬������gRPC������䷢����֤��
		auto email = src_root["email"].asString();
		GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();//���л�
		beast::ostream(connection->m_response.body()) << jsonstr;
		return true;

		}
	);
	//ע��
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection){
		auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		//��body_str�н������ݵ�src_root
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "Failed to prase JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		//�����ɹ����õ�ע����Ϣ
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		auto veriftcodeddd = src_root["verifycode"].asString();
		//ƥ�������ȷ�������Ƿ�һ��
		if (pwd!=confirm)
		{
			//��һ����д�ش�����Ϣ
			std::cout << "password err " << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body());
			return true;
		}

		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string verify_code;	//�洢��redis��ȡ����������Ϣ
		bool b_get_verify = RedisMgr::GetInstance()->Get(CODEPREFIX+src_root["email"].asString(), verify_code);
		if (!b_get_verify)
		{
			std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		//��֤���Ƿ���ȷ
		if (verify_code != src_root["verifycode"].asString())
		{
			std::cout << "verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		//�����ݿ����ע����Ϣ
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
		if (uid == 0||uid == -1)
		{
			std::cout << "user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		//�������ݿ��ж��û��Ƿ����
		root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["varifycode"] = src_root["varifycode"].asString();
		//json���л�
		std::string jsonstr = root.toStyledString();
		//д��connection��Ӧ��ظ��ͻ���
		beast::ostream(connection->m_response.body()) << jsonstr;
		return true;
		}
	);
	
	//��������
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection>connection) {
		//��ȡ������
		auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
		std::cout << "receive body is" << body_str << std::endl;
		connection->m_response.set(http::field::content_type, "text/json");	//������Ӧͷ����
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool prase_success = reader.parse(body_str, src_root);
		if (!prase_success)
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//����redis��email�Ƿ�����Լ��Ƿ���ȷ
		std::string verify_code;
		bool b_get_verify = RedisMgr::GetInstance()->Get(CODEPREFIX+src_root["email"].asString(),verify_code);
		if (!b_get_verify)
		{
			std::cout << "Get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		//	redis����֤��!=��д����֤��
		if (verify_code !=src_root["verifycode"].asString())
		{
			std::cout << "verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		//��ѯ���ݿ��ж��û����������Ƿ�ƥ��
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid)
		{
			std::cout << "user email not match " << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		//��������
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up)
		{
			std::cout << "update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		
		std::cout << "succeed to update password" << pwd << std::endl;
		//�ذ����ͻ���
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["verifycode"] = src_root["verifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->m_response.body()) << jsonstr;
		return true;

	});

	RegPost("/user_login", [](std::shared_ptr<HttpConnection>connection) {
		//��ȡ������
		auto body_str = boost::beast::buffers_to_string(connection->m_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->m_response.set(http::field::content_type, "text/json");

		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonObj = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonObj;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;

		//��ѯ���ݿ��ж��û����������Ƿ�ƥ��,���õ�uid��
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd,userInfo);
		if (!pwd_valid)
		{
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}

		//��ѯStatusServer�ҵ����ʵ�����
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_response.body()) << jsonstr;
			return true;
		}
		
		//��ȡ�����ʵ������Ժ����ûذ����ͻ��˵����ݣ�token��
		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::cout << reply.token();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->m_response.body()) << jsonstr;
		return true;


	});

}
