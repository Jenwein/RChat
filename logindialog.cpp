#include "logindialog.h"
#include "ui_logindialog.h"
#include "httpmgr.h"
#include "tcpmgr.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QByteArray>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->btn_register,&QPushButton::clicked,this,&LoginDialog::switchRegister);
    ui->forget_label->SetState("normal","hover","selected",
                               "selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    ui->pass_edit->setEchoMode(QLineEdit::Password);

    connect(ui->forget_label,&ClickedLabel::clicked,this,&LoginDialog::slot_forget_pwd);
    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_log_mod_finish,
            this,&LoginDialog::slot_log_mod_finish);
    //连接 tcp连接请求 的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TCPMgr::GetInstance().get(), &TCPMgr::slot_tcp_connect);

    //连接 tcp管理者发出的连接成功信号
    connect(TCPMgr::GetInstance().get(),&TCPMgr::sig_con_success,this,&LoginDialog::slot_tcp_con_finish);

    //连接tcp管理者发出的登陆失败信号
    connect(TCPMgr::GetInstance().get(), &TCPMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);

    //初始化头像显示
    initHead();

    //注册事件
    initHttpHandlers();
}

void LoginDialog::initHead()
{

    QPixmap pixmap(":/res/ice.png"); // 替换为你的图片路径
    //QPixmap roundedPixmap = createRoundedPixmap(pixmap, 20); // 20是圆角半径，可以根据需要调整
    QPixmap rounded(pixmap.size());
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(pixmap.rect(), 20, 20);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, pixmap);
    ui->head_label->setScaledContents(true);

    ui->head_label->setPixmap(rounded);

}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    m_tip_errs[te] = tips;
    showTip(tips,false);
}

void LoginDialog::DelTipErr(TipErr te)
{
    m_tip_errs.remove(te);
    if(m_tip_errs.empty())
    {
        ui->err_tip->clear();
        return;
    }
    showTip(m_tip_errs.first(),false);
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok)
        ui->err_tip->setProperty("state","normal");
    else
        ui->err_tip->setProperty("state","err");

    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

bool LoginDialog::checkEmailValid()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex (R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");// 正则匹配邮箱
    bool match = regex.match(email).hasMatch();
    if(!match)
    {
        AddTipErr(TipErr::TIP_EMAIL_ERR,tr("邮件地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool LoginDialog::checkPwdValid()
{
    auto pass = ui->pass_edit->text();
    if(pass.length()<6||pass.length()>15)
    {
        AddTipErr(TipErr::TIP_PWD_ERR,tr("密码长度应为6~15"));
        return false;
    }
    //创建正则
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match)//密码不符合要求
    {
        AddTipErr(TipErr::TIP_PWD_ERR,tr("不能包含非法字符"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->btn_login->setEnabled(enabled);
    ui->btn_register->setEnabled(enabled);
    return true;
}

void LoginDialog::initHttpHandlers()
{
    //注册登录回包的逻辑
    m_handlers.insert(ReqId::ID_LOGIN_USER,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error!=ErrorCodes::SUCCESS)
        {
            showTip(tr("参数错误"),false);
            enableBtn(true);
            return;
        }
        //回包正确会得到分配的服务器的配置信息以及账号信息
        auto email = jsonObj["email"].toString();

        //发送信号通知TCPMgr发起长连接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host= jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token= jsonObj["token"].toString();

        m_uid = si.Uid;
        m_token = si.Token;
        //登录完成，准备连接分配的ChatServer，开始tcp连接
        emit sig_connect_tcp(si);
        qDebug() << " email is " << email
                 << " uid is " << si.Uid
                 << " host is "<< si.Host
                 << " Port is " << si.Port
                 << " Token is " << si.Token;

        showTip(tr("登录成功"),true);
    });


}


void LoginDialog::on_btn_login_clicked()
{
    bool valid = checkEmailValid();
    if(!valid) return;
    valid = checkPwdValid();
    if(!valid) return;

    enableBtn(false);
    auto email = ui->email_edit->text();
    auto pwd = encryptPassword(ui->pass_edit->text());
    //发送Http请求注册用户
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"]= pwd;
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj,ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

void LoginDialog::slot_log_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS)
    {
        showTip(tr("网络请求错误"),false);
        return;
    }
    //解析JSON，字符串，res转换为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull())
    {
        showTip(tr("json解析失败"),false);
        return;
    }
    //json解析内容错误
    if(!jsonDoc.isObject())
    {
        showTip(tr("json解析错误"),false);
        return;
    }

    //解析成功
    m_handlers[id](jsonDoc.object());
    return;
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    if(bsuccess)
    {
        showTip(tr("聊天服务器连接成功，正在登陆..."),true);
        QJsonObject jsonObj;
        jsonObj["uid"] = m_uid;
        jsonObj["token"] = m_token;

        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);

        //发送tcp请求(uid+token)给ChatServer
        emit TCPMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN,jsonString);

    }else
    {
        showTip(tr("网络异常"),false);
        enableBtn(true);
    }

}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("Login failed,err is %1").arg(err);
    showTip(result,false);
    enableBtn(true);
}



