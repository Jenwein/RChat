#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _login_dlg = new LoginDialog(this);
    setCentralWidget(_login_dlg); // 直接设置 _login_dlg 作为中央窗口小部件
    //创建注册消息链接
    //连接登录界面注册信号
    connect(_login_dlg,&LoginDialog::switchRegister,this,&MainWindow::slotSwitchRegister);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    //连接登录界面忘记密码信号
    connect(_login_dlg,&LoginDialog::switchReset,this,&MainWindow::slotSwitchReset);
    //连接创建聊天界面信号
    connect(TCPMgr::GetInstance().get(),&TCPMgr::sig_switch_chatlg, this, &MainWindow::slotSwitchChat);
    emit TCPMgr::GetInstance()->sig_switch_chatlg();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotSwitchRegister()
{
    _register_dlg = new RegisterDialog(this);
    _register_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    connect(_register_dlg,&RegisterDialog::sigSwitchLogin,this,&MainWindow::slotSwitchLogin);

    setCentralWidget(_register_dlg);
    _login_dlg->hide();
}
void MainWindow::slotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    //判断发送者，根据发送信号的对象来进行hide操作
    QObject* senderObj = sender();
    if(senderObj == _reset_dlg)
        _reset_dlg->hide();
    else if(senderObj == _register_dlg)
        _register_dlg->hide();

    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::slotSwitchRegister);
    //连接忘记密码注册信号
    connect(_login_dlg,&LoginDialog::switchReset,this,&MainWindow::slotSwitchReset);
}


void MainWindow::slotSwitchReset()
{
    //创建一个CentralWidget并将其设置为MainWindow的中心部件
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

    _login_dlg->hide();
    _reset_dlg->show();
    connect(_reset_dlg,&ResetDialog::switchLogin,this,&MainWindow::slotSwitchLogin);
}

void MainWindow::slotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _login_dlg->hide();
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}
