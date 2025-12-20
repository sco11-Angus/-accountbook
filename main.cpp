#include "LoginWidget.h"
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LoginWidget loginWidget;
    MainWindow mainWindow; // 整合了账本界面的主窗口

    // 登录成功后：显示主窗口（含账本界面），关闭登录窗口
    QObject::connect(&loginWidget, &LoginWidget::loginSuccess, [&](const User& user){
        Q_UNUSED(user); // 实际项目中可传递用户信息给账本界面
        mainWindow.show(); // 显示完整账本界面
        loginWidget.close(); // 关闭登录窗口，避免后台残留
    });

    // 登录窗口显示
    loginWidget.show();
    return a.exec();
}
