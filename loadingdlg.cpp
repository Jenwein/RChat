#include "loadingdlg.h"
#include "ui_loadingdlg.h"
#include <QMovie>

LoadingDlg::LoadingDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadingDlg)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowSystemMenuHint|Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground); //设置背景透明
    //获取屏幕尺寸
    setFixedSize(parent->size());

    QMovie* movie = new QMovie(":/res/loading.gif");//加载动画资源
    ui->loading_lb->setMovie(movie);
    movie->start();

}

LoadingDlg::~LoadingDlg()
{
    delete ui;
}
