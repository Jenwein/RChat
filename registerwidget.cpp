#include "registerwidget.h"
#include "ui_registerwidget.h"

RegisterWidget::RegisterWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterWidget)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->passagain_edit->setEchoMode(QLineEdit::Password);

}

RegisterWidget::~RegisterWidget()
{
    delete ui;
}
