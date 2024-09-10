#ifndef TCPMGR1_H
#define TCPMGR1_H

#include <QTcpSocket>
#include <QObject>

#include "global.h"
#include "singleton.h"

class TCPMgr
    : public Singleton<TCPMgr>,
      public std::enable_shared_from_this<TCPMgr>
{
    Q_OBJECT
public:
    TCPMgr();

private:
    QTcpSocket m_socket;    //用于tcp连接的socket
    QString m_host;         //ip地址
    uint16_t m_port;        //端口

    QByteArray m_buffer;    //存储接受的数据
    bool m_b_recv_pending;  //接收状态,是否有未处理完的数据(本次tvl处理)
    quint16 m_message_id;   //存储消息id
    quint16 m_message_len;  //存储消息长度
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QString data);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QString data);
};

#endif // TCPMGR1_H
