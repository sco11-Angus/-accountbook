#include "LoginWidget.h"
#include "MainWindow.h"
#include "account_manager.h"
#include "account_record.h"
#include "AccountBookRecordWidget.h"
#include "AccountBookMainWidget.h"
#include "sqlite_helper.h"
#include "server_main.h"
#include "user_manager.h"
#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 初始化数据库
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dbDir);
    if (!dir.exists()) {
        dir.mkpath(dbDir);
    }
    QString dbPath = dbDir + "/account_book.db";
    
    SqliteHelper* dbHelper = SqliteHelper::getInstance();
    if (!dbHelper->openDatabase(dbPath)) {
        qCritical() << "数据库初始化失败，程序即将退出";
        return -1;
    }

    // 配置邮件发送服务（QQ邮箱）
    // TODO: 请在这里填写你的QQ邮箱和授权码
    // 授权码获取方法：登录QQ邮箱网页版 -> 设置 -> 账户 -> 开启SMTP服务 -> 生成授权码
    QString senderEmail = "123456789@qq.com";  // 改成你的QQ邮箱，例如: 123456789@qq.com
    QString authCode = "abcdef";        // 改成你的QQ邮箱授权码（不是QQ密码！）

    UserManager::getInstance()->configureEmailSender(senderEmail, authCode);
    qDebug() << "邮件服务已配置";

    // 2. 启动服务端 (监听端口 12345)
    server_main server;
    if (server.startServer(12345)) {
        qDebug() << "服务端启动成功，监听端口: 12345";
    } else {
        qWarning() << "服务端启动失败，可能是端口被占用";
    }

    LoginWidget loginWidget;
    MainWindow mainWindow; // 整合了账本界面的主窗口

    // 登录成功后：显示主窗口（含账本界面），关闭登录窗口
    QObject::connect(&loginWidget, &LoginWidget::loginSuccess, [&](const User& user){
        UserManager::getInstance()->setCurrentUser(user);
        mainWindow.show(); // 显示完整账本界面
        loginWidget.close(); // 关闭登录窗口，避免后台残留
    });

    // 登录窗口显示
    loginWidget.show();
    return a.exec();
}
