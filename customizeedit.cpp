#include "customizeedit.h"

CustomizeEdit::CustomizeEdit(QWidget* parent)
    :QLineEdit(parent),m_max_len(0)
{
    connect(this,&QLineEdit::textChanged,this,&CustomizeEdit::limitTextLength);


}


void CustomizeEdit::SetMaxLength(int maxlen)
{
    m_max_len = maxlen;
}

//执行失去焦点时的逻辑处理
void CustomizeEdit::focusOutEvent(QFocusEvent *event)
{
    //调用基类的focusOutEvent方法
    QLineEdit::focusOutEvent(event);
    //发送失去焦点信号
    emit sig_focus_out();
}

void CustomizeEdit::limitTextLength(QString text)
{
    if(m_max_len<=0)
    {
        return;
    }
    //输入的文本可能是任何类型字符，所以统一转换处理
    QByteArray byteArray = text.toUtf8();

    if(byteArray.size()>m_max_len)
    {
        byteArray = byteArray.left(m_max_len);
        this->setText(byteArray);
    }
}
