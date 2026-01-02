#ifndef BILL_HANDLER_H
#define BILL_HANDLER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QList>
#include "account_record.h"
#include "account_manager.h"

class SqliteHelper;

class bill_handler
{
public:
    bill_handler();
    ~bill_handler();

    // 处理同步账单请求
    QJsonObject handleSyncBills(const QJsonObject& request);
    
    // 处理单条记账记录请求
    QJsonObject handleAddRecord(const QJsonObject& request);

    // 处理编辑记账记录请求
    QJsonObject handleEditRecord(const QJsonObject& request);

    // 处理删除记账记录请求（软删除）
    QJsonObject handleDeleteRecord(const QJsonObject& request);

    // 处理恢复记账记录请求
    QJsonObject handleRestoreRecord(const QJsonObject& request);

    // 处理永久删除记账记录请求
    QJsonObject handlePermanentDeleteRecord(const QJsonObject& request);
    
    // 处理查询账单请求
    QJsonObject handleQueryBills(const QJsonObject& request);
    
    // 处理备份数据请求
    QJsonObject handleBackupData(const QJsonObject& request);

private:
    SqliteHelper* m_dbHelper;
    AccountManager* m_accountManager;
    
    // 直接操作 MySQL bill 表
    bool insertBillToDatabase(const AccountRecord& record, int defaultBookId = 1);

    // 确保用户和账本存在（处理外键约束）
    void ensureUserAndBookExist(int userId, int bookId);
    int queryCategoryId(const QString& categoryName, int userId);
    
    // 将 AccountRecord 转换为 JSON 对象
    QJsonObject recordToJson(const AccountRecord& record);
    // 将 JSON 对象转换为 AccountRecord
    AccountRecord jsonToRecord(const QJsonObject& json);
    // 将账单列表转换为 JSON 数组
    QJsonArray recordsToJsonArray(const QList<AccountRecord>& records);
};
#endif // BILL_HANDLER_H
