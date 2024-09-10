#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:

    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    void on_pushButton_clicked();
    void on_btn_sure_clicked();
    void on_btn_varify_clicked();
    void on_btn_cancel_clicked();

private:
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str,bool b_ok);

    //检测输入合法性
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkConfirmValid();
    bool checkVarifyValid();

    //页面切换
    void ChangeTipPage();
    //注册Http回包处理
    void initHttpHandlers();

private:
    Ui::RegisterDialog *ui;
    //根据id查询执行函数
    QMap<ReqId,std::function<void(const QJsonObject&)>>m_handlers;

    QMap<TipErr,QString> m_tip_errs;//缓存各个输入框输入完成后提示的错误,输入框错误清除后就显示剩余的错误

    QTimer* m_countdown_timer;
    int m_countdown;
signals:
    void sigSwitchLogin();

};

#endif // REGISTERDIALOG_H
