// account_manager.h
#ifndef ACCOUNT_MANAGER_H
#define ACCOUNT_MANAGER_H

#include "account_record.h"
#include "sqlite_helper.h"
#include <QString>
#include <QDateTime>
#include <QList>

class AccountManager {
public:
    AccountManager();

    // 快速记账
    bool addAccountRecord(const AccountRecord& record);
    // 编辑单条记录
    bool editAccountRecord(const AccountRecord& record);
    // 批量编辑记录
    bool batchEditAccountRecord(const QList<AccountRecord>& records);
    // 软删除记录（移入回收站）
    bool deleteAccountRecord(int recordId);
    // 恢复回收站记录
    bool restoreAccountRecord(int recordId);
    // 永久删除记录
    bool permanentDeleteAccountRecord(int recordId);
    // 多条件查询记录
    QList<AccountRecord> queryAccountRecord(int userId,
                                            const QString& timeRange = "",
                                            const QString& type = "",
                                            double minAmount = 0,
                                            double maxAmount = 0,
                                            bool isDeleted = false);
    // 获取预设收支类型
    QStringList getPresetTypes();
    // 按日期范围查询
    QList<AccountRecord> queryRecordsByDateRange(int userId,
                                                 const QDate& startDate,
                                                 const QDate& endDate,
                                                 bool isDeleted = false);
    // 按金额范围查询
    QList<AccountRecord> queryRecordsByAmountRange(int userId,
                                                   double minAmount,
                                                   double maxAmount,
                                                   bool isDeleted = false);
    // 按类型查询
    QList<AccountRecord> queryRecordsByType(int userId,
                                            const QString& type,
                                            bool isDeleted = false);
    // 按月份查询
    QList<AccountRecord> queryMonthlyRecords(int userId, int year, int month, bool isDeleted = false);
    // 获取记录总数
    int getRecordCount(int userId, bool isDeleted = false);

private:
    SqliteHelper* m_dbHelper = SqliteHelper::getInstance();
    
    // 将账单记录同步到服务端
    void syncRecordToServer(const AccountRecord& record);
    void syncEditRecordToServer(const AccountRecord& record);
    void syncDeleteRecordToServer(int recordId);
    void syncRestoreRecordToServer(int recordId);
    void syncPermanentDeleteRecordToServer(int recordId);
};

#endif // ACCOUNT_MANAGER_H
