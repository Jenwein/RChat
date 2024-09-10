#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include <QWidget>
#include "global.h"
#include "listitembase.h"

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override
    {
        return QSize(250,70);
    }

    void SetInfo(QString name,QString head,QString msg);

private:
    QString m_name;
    QString m_head;
    QString m_msg;
    Ui::ChatUserWid *ui;
};

#endif // CHATUSERWID_H
