#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    void addChatUserList();
    ~ChatDialog();

private:
    void ShowSearch(bool b_search);
private slots:
    void slot_loading_chat_user();
private:
    ChatUIMode m_mode;
    ChatUIMode m_state;
    bool m_b_loading;
    Ui::ChatDialog *ui;
};

#endif // CHATDIALOG_H
