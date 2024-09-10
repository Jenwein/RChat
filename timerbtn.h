#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget* parent = nullptr);
    ~TimerBtn();

    //重写mouseReleaseEvent
    virtual void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QTimer* m_Timer;
    int m_Counter;
};

#endif // TIMERBTN_H
