#include "LoginWidget.h"
#include "MainWindow.h"
#include <QApplication>
#include "accountbookrecordwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LoginWidget loginWidget;
    MainWindow mainWindow; // 整合了账本界面的主窗口
    AccountBookMainWidget bookMainWidget; // 账本主界面
    AccountBookRecordWidget recordWidget; // 记账窗口

    // 登录成功后：显示主窗口（含账本界面），关闭登录窗口
    QObject::connect(&loginWidget, &LoginWidget::loginSuccess, [&](const User& user){
        Q_UNUSED(user); // 实际项目中可传递用户信息给账本界面
        mainWindow.show(); // 显示完整账本界面
        loginWidget.close(); // 关闭登录窗口，避免后台残留
    });

    // 主函数/主界面初始化时，绑定信号槽
    AccountBookMainWidget mainWidget;

    QObject::connect(bookMainWidget.m_addBtn, &QPushButton::clicked, [&](){
        recordWidget.show();
    });

    // 4. 记账完成后更新账单数据（原来的全局connect移到这里）
    QObject::connect(&recordWidget, &AccountBookRecordWidget::billRecorded, &bookMainWidget, [&](){
        // 模拟账单数据（实际从数据库读取）
        QList<QMap<QString, QString>> billList;
        QMap<QString, QString> bill1;
        bill1["date"] = "12/18 星期四";
        bill1["cateIcon"] = "餐";
        bill1["cateName"] = "餐饮-三餐";
        bill1["time"] = "18:07 · 晚";
        bill1["amount"] = "13.00";
        bill1["isExpense"] = "true";
        billList.append(bill1);

        QMap<QString, QString> bill2;
        bill2["date"] = "12/18 星期四";
        bill2["cateIcon"] = "奖";
        bill2["cateName"] = "奖金-奖学金";
        bill2["time"] = "18:44 · 下午";
        bill2["amount"] = "1000.00";
        bill2["isExpense"] = "false";
        billList.append(bill2);

        bookMainWidget.updateBillData(billList);
    });

    // 登录窗口显示
    loginWidget.show();
    return a.exec();

}
