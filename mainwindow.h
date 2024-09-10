#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"
#include "tcpmgr.h"
//#include "registerdialog.h"
/************************************
 *
 * @file    mainwindow.h
 * @brief   XXXX Function
 *
 * @author  R.Jenwein
 * @date    2024/07/21
 * @history
*******************************************/
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void slotSwitchRegister();
    void slotSwitchLogin();
    void slotSwitchReset();
    void slotSwitchChat();
private:
    Ui::MainWindow *ui;
    LoginDialog* _login_dlg;
    RegisterDialog* _register_dlg;
    ResetDialog* _reset_dlg;
    ChatDialog* _chat_dlg;
};
#endif // MAINWINDOW_H
