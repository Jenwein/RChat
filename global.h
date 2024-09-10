#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDir>
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QJsonObject>
#include <QSettings>

#include "QStyle"

#include <memory>
#include <mutex>
#include <iostream>

/************************************
 *
 * @file    global.h
 * @brief   repolish 用来刷新qss
 *
*******************************************/

extern std::function<void(QWidget*)> repolish;  //重新应用样式表
extern QString encryptPassword(const QString &password);
// 请求类型id
enum ReqId{
    ID_GET_VARIFY_CODE = 1001,  //获取验证码
    ID_REG_USER = 1002,         //注册用户
    ID_RESET_PWD = 1003,        //重置密码
    ID_LOGIN_USER = 1004,       //用户登录
    ID_CHAT_LOGIN = 1005,       //登陆聊天服务器
    ID_CHAT_LOGIN_RSP = 1006,   //登录聊天服务器的回包
};

// 模块类型
enum Modules{
    REGISTERMOD = 0,    //注册模块
    RESETMOD = 1,       //重置密码
    LOGINMOD = 2,
};

// 错误类型
enum ErrorCodes{
    SUCCESS = 0,    //成功
    ERR_JSON = 1,   //json解析失败
    ERR_NETWORK = 2,//网络错误
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

//基于这两种状态嵌套实现Label的六种状态
enum ClickLbState{
    Normal = 0,         //普通状态
    Selected = 1        //选中状态
};

extern QString gate_url_prefix;

//聊天界面的模式
enum ChatUIMode
{
    SearchMode,
    ChatMode,
    ContactMode,
};

//自定义QListWidgetItem的几种类型
enum ListItemType
{
    CHAT_USER_ITEM,         //聊天用户
    CONTACT_USER_ITEM,      //联系人用户
    SEARCH_USER_ITEM,       //搜索到的用户
    ADD_USER_TIP_ITEM,      //提示添加用户
    INVALID_ITEM,           //不可点击条目
    GROUP_TIP_ITEM,         //分组提示条目
};

#endif // GLOBAL_H
