// account_manager.cpp
#include "account_manager.h"

AccountManager::AccountManager() {
    m_dbHelper = SqliteHelper::getInstance();
    m_dbHelper->openDatabase();
}

bool AccountManager::addAccountRecord(const AccountRecord& record) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString sql = QString(R"(
        INSERT INTO account_record (user_id, amount, type, remark, voucher_path, is_deleted, create_time, modify_time)
        VALUES (%1, %2, '%3', '%4', '%5', 0, '%6', '%7')
    )").arg(record.getUserId())
                      .arg(record.getAmount())
                      .arg(record.getType())
                      .arg(record.getRemark())
                      .arg(record.getVoucherPath())
                      .arg(now)
                      .arg(now);

    return m_dbHelper->executeSql(sql);
}

bool AccountManager::editAccountRecord(const AccountRecord& record) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString sql = QString(R"(
        UPDATE account_record SET amount = %1, type = '%2', remark = '%3', voucher_path = '%4', modify_time = '%5'
        WHERE id = %6 AND user_id = %7
    )").arg(record.getAmount())
                      .arg(record.getType())
                      .arg(record.getRemark())
                      .arg(record.getVoucherPath())
                      .arg(now)
                      .arg(record.getId())
                      .arg(record.getUserId());

    return m_dbHelper->executeSql(sql);
}

bool AccountManager::batchEditAccountRecord(const QList<AccountRecord>& records) {
    // 开启事务
    QSqlDatabase db = m_dbHelper->getDatabase();
    db.transaction();

    bool ret = true;
    for (const AccountRecord& record : records) {
        if (!editAccountRecord(record)) {
            ret = false;
            break;
        }
    }

    if (ret) db.commit();
    else db.rollback();

    return ret;
}

bool AccountManager::deleteAccountRecord(int recordId) {
    QString deleteTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString sql = QString(R"(
        UPDATE account_record SET is_deleted = 1, delete_time = '%1'
        WHERE id = %2
    )").arg(deleteTime).arg(recordId);

    return m_dbHelper->executeSql(sql);
}

bool AccountManager::restoreAccountRecord(int recordId) {
    QString sql = QString(R"(
        UPDATE account_record SET is_deleted = 0, delete_time = ''
        WHERE id = %1
    )").arg(recordId);

    return m_dbHelper->executeSql(sql);
}

bool AccountManager::permanentDeleteAccountRecord(int recordId) {
    QString sql = QString("DELETE FROM account_record WHERE id = %1").arg(recordId);
    return m_dbHelper->executeSql(sql);
}

QList<AccountRecord> AccountManager::queryAccountRecord(int userId,
                                                        const QString& timeRange,
                                                        const QString& type,
                                                        double minAmount,
                                                        double maxAmount,
                                                        bool isDeleted) {
    QList<AccountRecord> records;

    // 构建查询条件
    QString condition = QString("user_id = %1 AND is_deleted = %2").arg(userId).arg(isDeleted ? 1 : 0);
    if (!timeRange.isEmpty()) {
        condition += QString(" AND create_time BETWEEN '%1' AND '%2'").arg(timeRange.split("-").at(0)).arg(timeRange.split("-").at(1));
    }
    if (!type.isEmpty()) {
        condition += QString(" AND type = '%1'").arg(type);
    }
    if (minAmount > 0 || maxAmount > 0) {
        condition += QString(" AND amount >= %1 AND amount <= %2").arg(minAmount).arg(maxAmount);
    }

    QString sql = QString("SELECT * FROM account_record WHERE %1 ORDER BY create_time DESC").arg(condition);
    QSqlQuery query = m_dbHelper->executeQuery(sql);

    // 封装查询结果
    while (query.next()) {
        AccountRecord record;
        record.setId(query.value("id").toInt());
        record.setUserId(query.value("user_id").toInt());
        record.setAmount(query.value("amount").toDouble());
        record.setType(query.value("type").toString());
        record.setRemark(query.value("remark").toString());
        record.setVoucherPath(query.value("voucher_path").toString());
        record.setIsDeleted(query.value("is_deleted").toInt());
        record.setDeleteTime(query.value("delete_time").toString());
        record.setCreateTime(query.value("create_time").toString());
        record.setModifyTime(query.value("modify_time").toString());
        records.append(record);
    }

    return records;
}

QStringList AccountManager::getPresetTypes() {
    return m_presetTypes;
}
