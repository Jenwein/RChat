#include "tcpmgr.h"
#include "QJsonDocument"
#include "usermgr.h"

TCPMgr::TCPMgr(QObject *parent)
    : QObject{parent},
    m_host(""), m_port(0), m_b_recv_pending(false), m_message_id(0), m_message_len(0)

{
    // 连接成功
    QObject::connect(&m_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server!";
        emit sig_con_success(true);
    });

    //有数据可读
    QObject::connect(&m_socket, &QTcpSocket::readyRead, [&]() {
        //读取所有数据并追加到缓冲区
        m_buffer.append(m_socket.readAll());
        qDebug() << "Received JSON:" << m_buffer;
        //创建一个QDataStream数据流，用于从m_buffer中读取数据
        QDataStream stream(&m_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_7);

        //无限循环，用于处理缓冲区中的数据
        forever
        {
            //读取消息头
            //如果没有未处理完的数据（即 m_b_recv_pending 为 false），则尝试解析消息头。
            if (!m_b_recv_pending)
            {
                //检查缓冲区中的数据是否足够解析出一个消息头（消息ID+消息长度）,如果数据不够，退出循环等待更多数据
                if (m_buffer.size() < static_cast<int>(sizeof(quint16) * 2))
                {
                    return;
                }

                //使用 QDataStream 从缓冲区中读取消息 ID 和消息长度。
                stream >> m_message_id >> m_message_len;
                //将缓冲区中的前四个字节（消息 ID 和消息长度）移除。
                m_buffer = m_buffer.mid(sizeof(quint16) * 2); //截取消息头到末尾的全部数据

                //输出读取到的消息 ID 和消息长度。
                qDebug() << "Message ID: " << m_message_id << " ,Message length: " << m_message_len;
            }

            //检查消息体长度
            //检查缓冲区中的数据是否足够解析出一个完整的消息体。
            //如果数据不够，设置 m_b_recv_pending 为 true 并退出循环等待更多数据
            if (m_buffer.size() < m_message_len)
            {
                m_b_recv_pending = true;
                return;
            }
            //如果缓冲区中的数据足够解析出一个完整的消息体，设置 m_b_recv_pending 为 false。
            m_b_recv_pending = false;

            //读取消息体
            //从缓冲区中读取消息体，并输出调试信息。
            QByteArray messageBody = m_buffer.mid(0,m_message_len);
            qDebug() << "receive body msg is " << messageBody;
            //将缓冲区中的已处理数据移除。
            m_buffer = m_buffer.mid(m_message_len); //从已处理数据的末尾截取剩余内容
            handleMsg(ReqId(m_message_id),m_message_len, messageBody);
        }
    });

    //发生错误
    //处理信号重载,明确指定要连接的重载版本，即带有QAbstractSocket::SocketError参数的版本
    QObject::connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
        qDebug() << "Error: " << m_socket.errorString();
        switch (socketError)
        {
            //连接拒绝
        case QTcpSocket::ConnectionRefusedError:
            qDebug() << "Connection Refused!";
            emit sig_con_success(false);
            break;

            //远端关闭
        case QTcpSocket::RemoteHostClosedError:
            qDebug() << "Remote Host Closed Connection!";
            break;

            //未找到IP
        case QTcpSocket::HostNotFoundError:
            qDebug() << "Host Not Found!";
            emit sig_con_success(false);
            break;

            //连接超时
        case QTcpSocket::SocketTimeoutError:
            qDebug() << "Connection Timeout!";
            emit sig_con_success(false);
            break;

            //网络错误
        case QTcpSocket::NetworkError:
            qDebug() <<"Network Error!";
            break;

        default:
            qDebug() << "Other Error!";
            break;
        }
    });

    //断开连接
    QObject::connect(&m_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnection from server";
    });

    QObject::connect(this, &TCPMgr::sig_send_data, this, &TCPMgr::slot_send_data);

    initHandlers();
}
TCPMgr::~TCPMgr(){

}

void TCPMgr::initHandlers()
{

    //连接Chat服务器的验证得到的回包
    m_handlers.insert(ID_CHAT_LOGIN_RSP,[this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<" data is"<<data;
        QByteArray datastr = QByteArray::fromHex(data);
        QJsonParseError parseError;
         //将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument. Error:" << parseError.errorString();
            return;
        }


        QJsonObject jsonObj = jsonDoc.object();
        qDebug()<<"jsonObj is "<<jsonObj;

        //如果不包含error字段
        if(!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug()<<"Login Failed, err is Json Parse Err "<<err;
            emit sig_login_failed(err);
            return;
        }


        int err = jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS)
        {
            qDebug()<<"Login Failed,err is "<<err;
            emit sig_login_failed(err);
            return;
        }

        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        //切换到聊天界面
        emit sig_switch_chatlg();
    });

}

void TCPMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = m_handlers.find(id);
    if(find_iter == m_handlers.end())
    {
        qDebug()<<"not found id["<<id<<"] to handle";
        return ;
    }
    find_iter.value()(id,len,data);
}
//将数据发送到 TCP 连接，连接到分配的ChatServer
void TCPMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "A TCP connection signal is received";
    //尝试连接到服务器
    qDebug() << "Connecting to server...";
    m_host = si.Host;
    m_port = static_cast<uint16_t>(si.Port.toUInt());
    m_socket.connectToHost(m_host, m_port);
    //连接成功将发送信号&QTcpSocket::connected
}

void TCPMgr::slot_send_data(ReqId reqId, QString data)
{
    uint16_t id = reqId;

    QByteArray dataArray = data.toUtf8();
    // 计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(data.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    //设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    //写入ID和长度
    out << id << len;

    //添加字符串数据
    block.append(dataArray);

    //发送数据
    m_socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block;
}
