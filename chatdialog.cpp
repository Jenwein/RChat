#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "chatuserwid.h"
#include "loadingdlg.h"
#include <QTimer>
#include <QRandomGenerator>

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent), m_mode(ChatUIMode::ChatMode),m_state(ChatUIMode::ChatMode),m_b_loading(false),ui(new Ui::ChatDialog)
{
    ui->setupUi(this);

    ui->add_btn->SetState("normal","hover","press");

    //为search_edit控件添加搜索和清除功能
    QAction* searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    //创建清除动作
    QAction* clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    //将 clearAction 添加到 QLineEdit 的末尾位置（TrailingPosition）
    ui->search_edit->addAction(clearAction,QLineEdit::TrailingPosition);

    //根据文本内容动态更新清除图标
    connect(ui->search_edit,&QLineEdit::textChanged,this,[clearAction](const QString &text){
        if(!text.isEmpty())
        {
            //当文本不为空时，将清除图标设置为实际的清除图标；
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        }
        else
        {
            //当文本为空时，将图标设置为透明图标
            clearAction->setIcon(QIcon(":/res/close_transparent.png"));
        }
    });

    //清除文本功能
    connect(clearAction,&QAction::triggered,[this,clearAction](){
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png"));
        ui->search_edit->clearFocus();
        ShowSearch(false);
    });

    //连接加载信号和槽
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);


    ui->search_edit->SetMaxLength(20);

    ShowSearch(false);
    addChatUserList();
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::ShowSearch(bool b_search)
{
    if(b_search)
    {
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        m_mode = ChatUIMode::SearchMode;
    }else if(m_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        m_mode = ChatUIMode::ChatMode;
    }else if(m_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->con_user_list->show();
        ui->search_list->hide();
        m_mode = ChatUIMode::ContactMode;
    }

}

void ChatDialog::slot_loading_chat_user()
{
    if(m_b_loading)
        return;

    m_b_loading = true;
    LoadingDlg* loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug()<<"add new data to list...";
//QT实现睡眠
    // QEventLoop eventloop;
    // QTimer::singleShot(1000, &eventloop, SLOT(quit()));
    // eventloop.exec();

    //模拟加载更多
    addChatUserList();
    //加载完毕后关闭对话框
    loadingDialog->deleteLater();
    m_b_loading = false;
}


//test
std::vector<QString> strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

std::vector<QString> heads = {
    ":/res/head_1.png",
    ":/res/head_2.png",
    ":/res/head_3.png",
    ":/res/head_4.png",
};

std::vector<QString> names = {
    "rgw",
    "zack",
    "golang",
    "cpp",
    "java",
    "nodejs",
    "python",
    "rust"
};

void ChatDialog::addChatUserList()
{
    // 创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
    }
}

