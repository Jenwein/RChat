#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    void initHead();
    ~LoginDialog();
signals:
    void switchRegister();
    void switchReset();

    void sig_connect_tcp(ServerInfo);
private slots:
    void slot_forget_pwd();
    void on_btn_login_clicked();
    void slot_log_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int err);
private:
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str,bool b_ok);

    bool checkEmailValid();
    bool checkPwdValid();

    bool enableBtn(bool);

    void initHttpHandlers();
private:
    Ui::LoginDialog *ui;

    int m_uid;
    QString m_token;

    QMap<TipErr,QString>m_tip_errs;
    QMap<ReqId,std::function<void(const QJsonObject&)>> m_handlers;


};

#endif // LOGINDIALOG_H
