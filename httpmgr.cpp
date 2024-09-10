#include "httpmgr.h"
#include <QEventLoop>
HttpMgr::~HttpMgr()
{

}

HttpMgr::HttpMgr()
{
    connect(this,&HttpMgr::sig_http_finish,this,&HttpMgr::slot_http_finish);
}

//发送http请求并接受回包
void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    //转换JSON对象为字节数组，将传入的QJsonObject转换为QByteArray，以便在HTTP请求中发送
    QByteArray data = QJsonDocument(json).toJson();
    //创建HTTP请求对象
    QNetworkRequest request(url);
    //设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");   //内容类型
    request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(data.length()));//内容长度

    //伪闭包延长生命周期
    auto self = shared_from_this();
    //发送HTTP POST请求，收到的回包返回在reply
    QNetworkReply* reply = m_manager.post(request,data);


    //发送结束后对接收到的回包的处理
    QObject::connect(reply,&QNetworkReply::finished,[self,reply,req_id,mod](){
        //处理错误
        if(reply->error() != QNetworkReply::NoError)
        {
            qDebug()<<reply->errorString();
            //发送信号通知完成
            emit self->sig_http_finish(req_id,"",ErrorCodes::ERR_NETWORK,mod);
            reply->deleteLater();
            return;
        }
        //无错误
        QString res = reply->readAll();
        //发送信号通知
        emit self->sig_http_finish(req_id,res,ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });

 /*
    // 发送 HTTP POST 请求，收到的返回在 reply
    //QNetworkReply* reply = m_manager.post(request, data);

    // 创建一个事件循环
    QEventLoop eventLoop;
    // 连接 finished 信号到事件循环的 quit 槽
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    // 进入事件循环，等待请求完成
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    // 处理请求完成后的逻辑
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
        reply->deleteLater();
        return;
    }
    QString res = reply->readAll();
    emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
    reply->deleteLater();
*/
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    //如果是注册模块，发送信号通知指定模块http的响应结束
    if(mod == Modules::REGISTERMOD)
        emit sig_reg_mod_finish(id,res,err);

    if(mod == Modules::RESETMOD)
        emit sig_reset_mod_finish(id,res,err);

    if(mod == Modules::LOGINMOD)
        emit sig_log_mod_finish(id,res,err);
}
