#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
//CRTP实现HttpMgr
class HttpMgr:public QObject,
                public Singleton<HttpMgr>,
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:  
    ~HttpMgr();//用于模板析构，所以public

    //发送Http请求。
    //url-目标地址；
    //json-序列化；
    //req_id-消息所属类型；
    //mod-消息所属模块；
    void PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod);
private:
    friend class Singleton<HttpMgr>;//声明基类为友元，基类访问子类的构造函数

    QNetworkAccessManager m_manager;

    HttpMgr();


signals:
    // 发送结束信号
    void sig_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqId id,QString res,ErrorCodes err);
    void sig_reset_mod_finish(ReqId id,QString res,ErrorCodes err);
    void sig_log_mod_finish(ReqId id,QString res,ErrorCodes err);
private slots:
    void slot_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);

};

#endif // HTTPMGR_H
