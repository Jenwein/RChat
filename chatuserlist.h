#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H

#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>

class ChatUserList:public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget* parent = nullptr);
protected:
    bool eventFilter(QObject* watched,QEvent* event) override;
signals:
    void sig_loading_chat_user();
};

#endif // CHATUSERLIST_H
