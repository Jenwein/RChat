#include "loginwidget.h"
#include "ui_loginwidget.h"

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    connect(ui->btn_register,&QPushButton::clicked,this,&LoginWidget::switchRegister);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}
