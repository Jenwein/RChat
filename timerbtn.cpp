#include "timerbtn.h"

#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget* parent)
    :QPushButton(parent),m_Counter(10)
{
    m_Timer = new QTimer(this);

    connect(m_Timer,&QTimer::timeout,[this](){
        m_Counter--;
        if(m_Counter<=0)
        {
            m_Timer->stop();
            m_Counter = 10;
            this->setText("获取验证码");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(m_Counter));
    });

}

TimerBtn::~TimerBtn()
{
    m_Timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        //处理鼠标左键释放事件
        qDebug()<<"MyButton was released!";
        this->setEnabled(false);
        this->setText(QString::number(m_Counter));
        m_Timer->start(1000);   //每一秒触发一次
        emit clicked();
    }
    //调用基类mouseReleaseEvent保证事件正常处理
    QPushButton::mouseReleaseEvent(e);
}
