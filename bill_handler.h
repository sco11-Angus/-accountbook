#ifndef BILL_HANDLER_H
#define BILL_HANDLER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QList>
#include "account_record.h"
#include "account_manager.h"

class bill_handler
{
public:
    bill_handler();
    ~bill_handler();

    // 处理同步账单请求
    QJsonObject handleSyncBills(const QJsonObject& request);
    
    // 处理查询账单请求
    QJsonObject handleQueryBills(const QJsonObject& request);
    
    // 处理备份数据请求
    QJsonObject handleBackupData(const QJsonObject& request);

private:
    AccountManager* m_accountManager;
    
    // 将 AccountRecord 转换为 JSON 对象
    QJsonObject recordToJson(const AccountRecord& record);
    // 将 JSON 对象转换为 AccountRecord
    AccountRecord jsonToRecord(const QJsonObject& json);
    // 将账单列表转换为 JSON 数组
    QJsonArray recordsToJsonArray(const QList<AccountRecord>& records);
};
#endif // BILL_HANDLER_H
