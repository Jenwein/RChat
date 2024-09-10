#include "clickedlabel.h"

ClickedLabel::ClickedLabel(QWidget *parent)
    :QLabel(parent),m_curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
}

//鼠标点击事件
void ClickedLabel::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)   //左键点击
    {
        if(m_curstate == ClickLbState::Normal)  //未悬停在label上，且选中/未选中
        {
            qDebug()<<"clicked,change to selected hover: "<<m_selected_hover;
            m_curstate = ClickLbState::Selected;
            setProperty("state",m_selected_hover);
            repolish(this);
            update();
        }else//未悬停在label上，且未选中/选中
        {
            qDebug()<<"clicked,change to normal hover: "<<m_normal_hover;
            m_curstate = ClickLbState::Normal;
            setProperty("state",m_normal_hover);
            repolish(this);
            update();
        }
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);

}
void ClickedLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(m_curstate == ClickLbState::Normal){
            // qDebug()<<"ReleaseEvent , change to normal hover: "<< _normal_hover;
            setProperty("state",m_normal_hover);
            repolish(this);
            update();

        }else{
            //  qDebug()<<"ReleaseEvent , change to select hover: "<< _selected_hover;
            setProperty("state",m_selected_hover);
            repolish(this);
            update();
        }
        emit clicked();
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);
}
//处理鼠标悬停进入事件
void ClickedLabel::enterEvent(QEnterEvent* event)
{
    if(m_curstate == ClickLbState::Normal)
    {
        qDebug()<<"enter , change to normal hover: "<< m_normal_hover;
        setProperty("state",m_normal_hover);
        repolish(this);
        update();
    }
    else
    {
        qDebug()<<"enter , change to selected hover: "<< m_selected_hover;
        setProperty("state",m_selected_hover);
        repolish(this);
        update();
    }
    QLabel::enterEvent(event);
}

void ClickedLabel::leaveEvent(QEvent *event)
{
    // 在这里处理鼠标悬停离开的逻辑
    if(m_curstate == ClickLbState::Normal){
        qDebug()<<"leave , change to normal : "<< m_normal;
        setProperty("state",m_normal);
        repolish(this);
        update();

    }else{
        qDebug()<<"leave , change to normal hover: "<< m_selected;
        setProperty("state",m_selected);
        repolish(this);
        update();
    }

    QLabel::leaveEvent(event);
}

void ClickedLabel::SetState(QString normal, QString hover, QString press,
                            QString select, QString select_hover, QString select_press)
{
    m_normal = normal;
    m_normal_hover = hover;
    m_normal_press = press;

    m_selected = select;
    m_selected_hover = select_hover;
    m_selected_press = select_press;

    setProperty("state",normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState()
{
    return m_curstate;
}



