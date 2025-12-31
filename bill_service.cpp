#include "bill_service.h"
#include "account_manager.h"  // 你的原有类
#include "tcpclient.h"        // 你的原有类
#include "sqlite_helper.h"    // 你的原有类
#include <QDate>
#include <QDebug>
#include <QSqlQuery>

// 单例实例实现
BillService* BillService::getInstance() {
    static BillService instance;
    return &instance;
}

BillService::BillService(QObject* parent) : QObject(parent) {
    // 构造函数中统一连接 TcpClient 的信号
    connect(TcpClient::getInstance(), &TcpClient::syncBillsResponse, this, [=](bool success, const QString& msg) {
        // 这里暂时无法直接获取到具体的 billId，因为 TcpClient 的信号没带这个
        // 但可以发出一个全局的同步完成信号
        emit billSyncStatusChanged(-1, success);
        qDebug() << "账单同步结果反馈：" << (success ? "成功" : "失败") << msg;
    });
}

bool BillService::saveBill(const AccountRecord& record) {
    // 保存到本地SQLite
    AccountManager accountManager;
    int localId = accountManager.addAccountRecord(record);
    
    if (localId <= 0) {
        emit getInstance()->billSaved(false, "本地保存失败：" + SqliteHelper::getInstance()->getLastError());
        return false;
    }

    // 构造带本地ID的账单对象
    AccountRecord newRecord = record;
    newRecord.setId(localId);

    // 异步同步到服务端
    TcpClient* tcpClient = TcpClient::getInstance();
    
    // 1. 先连接服务端
    if (!tcpClient->isConnected()) {
        bool connectSuccess = tcpClient->connectToServer("localhost", 12345);
        if (!connectSuccess) {
            emit getInstance()->billSaved(true, QString("本地保存成功（ID：%1），但同步失败：未连接到服务端").arg(localId));
            return true;
        }
    }

    // 2. 同步账单
    QList<AccountRecord> bills;
    bills.append(newRecord);
    bool syncRequestSent = tcpClient->syncBills(bills);

    // 3. 发射保存结果信号
    if (syncRequestSent) {
        emit getInstance()->billSaved(true, QString("账单已保存到本地并开始同步（本地ID：%1）").arg(localId));
    } else {
        emit getInstance()->billSaved(true, QString("账单已保存到本地，但同步请求发送失败（本地ID：%1）").arg(localId));
    }

    return true;
}

QList<AccountRecord> BillService::getMonthlyBills(int userId, const QDate& date) {
    AccountManager accountManager;
    // 获取指定月份未删除的账单
    return accountManager.queryMonthlyRecords(userId, date.year(), date.month(), false);
}

QList<AccountRecord> BillService::getCurrentMonthBills(int userId) {
    return getMonthlyBills(userId, QDate::currentDate());
}
