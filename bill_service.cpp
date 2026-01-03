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

    qDebug() << "本地账单保存成功，准备同步到服务端";
    
    // 构造带本地ID的账单对象
    AccountRecord newRecord = record;
    newRecord.setId(localId);

    // 异步同步到服务端
    TcpClient* tcpClient = TcpClient::getInstance();
    
    // 1. 先连接服务端
    if (!tcpClient->isConnected()) {
        tcpClient->connectToServer("localhost", 12345);
    }

    // 2. 使用 addRecord 发送单条记录
    bool syncRequestSent = tcpClient->addRecord(newRecord.getUserId(), newRecord);

    // 3. 发射保存结果信号
    // 注意：在这里立即发射信号，让 UI 刷新。同步结果会通过 syncBillsResponse 信号异步返回。
    emit getInstance()->billSaved(true, "账单保存成功");

    if (!syncRequestSent) {
        qWarning() << "同步请求发送失败，账单仅保存在本地";
    }

    return true;
}

bool BillService::updateBill(const AccountRecord& record) {
    // 更新到本地SQLite
    AccountManager accountManager;
    bool success = accountManager.editAccountRecord(record);
    
    if (!success) {
        emit getInstance()->billSaved(false, "本地更新失败：" + SqliteHelper::getInstance()->getLastError());
        return false;
    }

    qDebug() << "本地账单更新成功，准备同步到服务端";
    
    // 异步同步到服务端
    TcpClient* tcpClient = TcpClient::getInstance();
    
    // 1. 先连接服务端
    if (!tcpClient->isConnected()) {
        tcpClient->connectToServer("localhost", 12345);
    }

    // 2. 使用 editRecord 发送更新请求
    bool syncRequestSent = tcpClient->editRecord(record.getUserId(), record);

    // 3. 发射保存结果信号
    emit getInstance()->billSaved(true, "账单更新成功");

    if (!syncRequestSent) {
        qWarning() << "更新请求发送失败，修改仅保存在本地";
    }

    return true;
}

bool BillService::deleteBill(int recordId) {
    // 删除本地SQLite记录
    AccountManager accountManager;
    bool success = accountManager.deleteAccountRecord(recordId);
    
    if (!success) {
        emit getInstance()->billSaved(false, "本地删除失败：" + SqliteHelper::getInstance()->getLastError());
        return false;
    }

    qDebug() << "本地账单删除成功，准备同步到服务端";
    
    // 异步同步到服务端已经在 accountManager.deleteAccountRecord 中通过 syncDeleteRecordToServer 处理了
    
    // 发射保存结果信号（这里复用 billSaved 信号，因为 UI 逻辑一致）
    emit getInstance()->billSaved(true, "账单删除成功");

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
