#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QLineEdit>

class CustomizeEdit:public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget* parent = nullptr);
    void SetMaxLength(int maxlen);

protected:
    void focusOutEvent(QFocusEvent* event) override;

private:
    void limitTextLength(QString text);

private:
    int m_max_len;
signals:
    void sig_focus_out();
};

#endif // CUSTOMIZEEDIT_H
