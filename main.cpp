#include "LoginWidget.h"
#include "MainWindow.h"
#include "account_manager.h"
#include "account_record.h"
#include "AccountBookRecordWidget.h"
#include "AccountBookMainWidget.h"
#include <QApplication>
#include <QObject>

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

    // 4. 记账完成后更新账单数据（原来的全局connect移到这里）
    QObject::connect(&recordWidget, &AccountBookRecordWidget::billRecorded, &bookMainWidget, [&](){
        // 1. 获取当前登录用户ID
        UserManager* userManager = UserManager::getInstance();
        int userId = userManager->getCurrentUser().getId();
        if (userId <= 0) return;

        // 2. 查询当前月份的真实账单（调用AccountManager）
        AccountManager accountManager;
        QDate currentDate = QDate::currentDate();
        QList<AccountRecord> records = accountManager.queryMonthlyRecords(
            userId, currentDate.year(), currentDate.month()
            );

        // 3. 转换为界面所需的QList<QMap<QString, QString>>格式
        QList<QMap<QString, QString>> billList;
        for (const AccountRecord& record : records) {
            QMap<QString, QString> bill;
            QDate createDate = QDate::fromString(record.getCreateTime(), "yyyy-MM-dd HH:mm:ss");
            bill["date"] = createDate.toString("MM/dd dddd");
            bill["cateIcon"] = record.getType().left(1); // 分类首字（如“餐饮”→“餐”）
            bill["cateName"] = record.getType();
            bill["time"] = record.getCreateTime().mid(11, 5) + " · " + (record.getCreateTime().mid(11, 2).toInt() >= 18 ? "晚" : "下午");
            bill["amount"] = QString::number(qAbs(record.getAmount()), 'f', 2); // 显示绝对值
            bill["isExpense"] = (record.getAmount() < 0) ? "true" : "false";
            billList.append(bill);
        }

        // 4. 更新主界面账单
        bookMainWidget.updateBillData(billList);
    });

    // 登录窗口显示
    loginWidget.show();
    return a.exec();

}
