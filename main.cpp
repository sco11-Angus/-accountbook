#include "mainwindow.h"
#include "LoginWidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LoginWidget loginWidget;
    MainWindow mainWindow;

    QObject::connect(&loginWidget, &LoginWidget::loginSuccess, [&](const User& user){
        Q_UNUSED(user);  // 实际项目中可使用登录用户信息
        mainWindow.show();
    });

    loginWidget.show();
    return a.exec();
}
