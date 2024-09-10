#include "listitembase.h"


#include <QStyleOption>
#include <QPaintEvent>
#include <QPainter>

ListItemBase::ListItemBase(QWidget* parent)
{
    Q_UNUSED(parent);
}

void ListItemBase::SetItemType(ListItemType itemType)
{
    m_itemType = itemType;
}

ListItemType ListItemBase::GetItemType()
{
    return m_itemType;
}

void ListItemBase::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    //使用当前样式的 drawPrimitive 方法来绘制小部件。QStyle::PE_Widget 是一个枚举值，表示要绘制的是一个通用的小部件
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

}
