#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "httpmgr.h"
#include "global.h"
#include "clickedlabel.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->passagain_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state","normal");
    repolish(ui->err_tip);

    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_reg_mod_finish,
            this,&RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit,&QLineEdit::editingFinished,this,[this](){
        checkEmailValid();
    });

    connect(ui->pass_edit,&QLineEdit::editingFinished,this,[this](){
        checkPassValid();
    });

    connect(ui->passagain_edit,&QLineEdit::editingFinished,this,[this](){
        checkConfirmValid();
    });

    connect(ui->varify_edit,&QLineEdit::editingFinished,this,[this](){
        checkVarifyValid();
    });

    //设置浮动显示手形状
    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->passagain_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible","unvisible_hover",""
                               "visible","visible_hover","");
    ui->passagain_visible->SetState("unvisible","unvisible_hover","",
                                    "visible","visible_hover","");

    //连接点击事件
    connect(ui->pass_visible,&ClickedLabel::clicked,this,[this](){
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal)
        {
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }
        else
        {
            ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }

        qDebug()<<"Label was clicked!";
    });

    connect(ui->passagain_visible,&ClickedLabel::clicked,this,[this](){
        auto state = ui->passagain_visible->GetCurState();
        if(state == ClickLbState::Normal)
        {
            ui->passagain_edit->setEchoMode(QLineEdit::Password);
        }
        else
        {
            ui->passagain_edit->setEchoMode(QLineEdit::Normal);
        }

        qDebug()<<"Label was clicked!";
    });

    //创建定时器
    m_countdown_timer = new QTimer(this);
    //连接信号和槽
    connect(m_countdown_timer,&QTimer::timeout,[this](){
        if(m_countdown == 0)
        {
            m_countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        m_countdown--;
        auto str = QString ("注册成功，%1 秒后将返回登陆界面").arg(m_countdown);
        ui->tip_lb->setText(str);
    });


}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}



void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
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

void RegisterDialog::showTip(QString str,bool b_ok)
{
    if(b_ok)
    {
        ui->err_tip->setProperty("state","normal");
    }
    else
    {
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void RegisterDialog::initHttpHandlers()
{
    //注册获取验证码回包的逻辑
    m_handlers.insert(ReqId::ID_GET_VARIFY_CODE,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS)
        {
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送至邮箱，请注意查收"),true);
        qDebug()<<"email is "<<email;
    });

    //注册注册用户回包逻辑
    m_handlers.insert(ReqId::ID_REG_USER,[this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error!=ErrorCodes::SUCCESS)
        {
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"),true);
        qDebug()<<"email is "<<email;
        qDebug()<<"email is "<<jsonObj["uuid"].toString();
        ChangeTipPage();
    });
}

void RegisterDialog::on_btn_varify_clicked()
{
    auto email = ui->email_edit->text();
    bool valid = checkEmailValid();
    if(!valid) return;
    else
    {
        //发送http验证码
        QJsonObject json_obj;
        json_obj["email"]=email;
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"),
                                            json_obj,ReqId::ID_GET_VARIFY_CODE,Modules::REGISTERMOD);
    }


}

void RegisterDialog::on_btn_sure_clicked()
{
    //检查输入是否合法
    bool valid = checkUserValid();
    if(!valid) return;
    valid = checkEmailValid();
    if(!valid) return;
    valid = checkPassValid();
    if(!valid) return;
    valid = checkConfirmValid();
    if(!valid) return;
    valid = checkVarifyValid();
    if(!valid) return;

    //发送http请求注册用户
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = encryptPassword(ui->pass_edit->text());
    json_obj["confirm"] = encryptPassword(ui->passagain_edit->text());
    json_obj["verifycode"] = ui->varify_edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    m_tip_errs[te] = tips;
    showTip(tips,false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    m_tip_errs.remove(te);
    if(m_tip_errs.empty())
    {
        ui->err_tip->clear();
        return;
    }
    showTip(m_tip_errs.first(),false);
}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text()=="")
    {
        AddTipErr(TipErr::TIP_USER_ERR,tr("用户名不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkEmailValid()
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

bool RegisterDialog::checkPassValid()
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

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->passagain_edit->text();

    if(pass!=confirm)
    {
        AddTipErr(TipErr::TIP_PWD_CONFIRM,tr("确认密码与密码不匹配"));
        return false;
    }else
    {
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }

    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->varify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}


void RegisterDialog::ChangeTipPage()
{
    m_countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    //启动定时器，设置间隔未1000毫秒
    m_countdown_timer->start(1000);
}


void RegisterDialog::on_pushButton_clicked()
{
    m_countdown_timer->stop();
    emit sigSwitchLogin();
}


void RegisterDialog::on_btn_cancel_clicked()
{
    m_countdown_timer->stop();
    emit sigSwitchLogin();
}

