#ifndef TCPMGR_H
#define TCPMGR_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonArray>
#include <QAbstractSocket>

#include "global.h"
#include "singleton.h"

class TCPMgr : public QObject,
               public Singleton<TCPMgr>,
               public std::enable_shared_from_this<TCPMgr>
{
    Q_OBJECT
    friend class Singleton<TCPMgr>;
public:
    explicit TCPMgr(QObject *parent = nullptr);
    ~TCPMgr();
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);
private:
    QTcpSocket m_socket;    //用于tcp连接的socket
    QString m_host;         //ip地址
    uint16_t m_port;        //端口
    QMap<ReqId,std::function<void(ReqId id, int len, QByteArray data)>> m_handlers;

    QByteArray m_buffer;    //存储接受的数据
    bool m_b_recv_pending;  //接收状态,是否有未处理完的数据(本次tvl处理)
    quint16 m_message_id;   //存储消息id
    quint16 m_message_len;  //存储消息长度
public slots:
    void slot_send_data(ReqId reqId, QString data);
    void slot_tcp_connect(ServerInfo);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QString data);
    void sig_login_failed(int err);
    void sig_switch_chatlg();
};

#endif // TCPMGR_H
