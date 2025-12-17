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

private:
    SqliteHelper* m_dbHelper;
    // 预设收支类型
    QStringList m_presetTypes = {"餐饮", "交通", "娱乐", "薪资", "购物", "房租", "水电", "医疗", "理财", "其他"};
};

#endif // ACCOUNT_MANAGER_H
