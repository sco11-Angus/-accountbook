#include "account_manager.h"
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QApplication>
#include "tcpclient.h"
#include "user_manager.h"

AccountManager::AccountManager() {
    m_dbHelper = SqliteHelper::getInstance();
    m_dbHelper->openDatabase();
}

int AccountManager::addAccountRecord(const AccountRecord& record) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 计算类型：金额小于0为支出(0)，大于等于0为收入(1)
    int type = (record.getAmount() < 0) ? 0 : 1;

    // 使用更新后的通用表结构
    QString sql = QString(R"(
        INSERT INTO account_record (
            user_id, bill_date, amount, type, category, remark, 
            voucher_path, is_deleted, create_time, modify_time
        ) VALUES (?, ?, ?, ?, ?, ?, ?, 0, ?, ?)
    )");
    
    QString recordTime = record.getCreateTime().isEmpty() ? now : record.getCreateTime();
    
    QVariantList params;
    params << record.getUserId()
           << recordTime
           << record.getAmount()
           << type
           << record.getType()    // 分类名称
           << record.getRemark()  // 备注
           << record.getVoucherPath()
           << recordTime
           << now;
    
    bool success = m_dbHelper->executeSqlWithParams(sql, params);
    
    if (success) {
        qDebug() << "账单保存成功：用户ID=" << record.getUserId() 
                 << "金额=" << record.getAmount() 
                 << "分类=" << record.getType()
                 << "时间=" << record.getCreateTime();

        // 同步到服务端 (已迁移到 BillService 处理)
        // syncRecordToServer(record);

        // 获取最后插入的ID
        QSqlQuery query = m_dbHelper->executeQuery("SELECT last_insert_rowid()");
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << "账单保存失败：" << m_dbHelper->getLastError();
    }
    
    return -1;
}

bool AccountManager::editAccountRecord(const AccountRecord& record) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    
    // 计算类型：金额小于0为支出(0)，大于等于0为收入(1)
    int type = (record.getAmount() < 0) ? 0 : 1;
    
    QString sql = QString(R"(
        UPDATE account_record 
        SET bill_date = ?, amount = ?, type = ?, category = ?, remark = ?, 
            voucher_path = ?, create_time = ?, modify_time = ?
        WHERE id = ? AND user_id = ?
    )");
    
    QVariantList params;
    params << record.getCreateTime()
           << record.getAmount()
           << type
           << record.getType()    // 分类名称
           << record.getRemark()  // 备注
           << record.getVoucherPath()
           << record.getCreateTime() // 同步更新创建时间，确保显示一致
           << now
           << record.getId()
           << record.getUserId();

    bool success = m_dbHelper->executeSqlWithParams(sql, params);
    if (success) {
        syncEditRecordToServer(record);
    }
    return success;
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

    bool success = m_dbHelper->executeSql(sql);
    if (success) {
        syncDeleteRecordToServer(recordId);
    }
    return success;
}

bool AccountManager::restoreAccountRecord(int recordId) {
    QString sql = QString(R"(
        UPDATE account_record SET is_deleted = 0, delete_time = ''
        WHERE id = %1
    )").arg(recordId);

    bool success = m_dbHelper->executeSql(sql);
    if (success) {
        syncRestoreRecordToServer(recordId);
    }
    return success;
}

bool AccountManager::permanentDeleteAccountRecord(int recordId) {
    QString sql = QString("DELETE FROM account_record WHERE id = %1").arg(recordId);
    bool success = m_dbHelper->executeSql(sql);
    if (success) {
        syncPermanentDeleteRecordToServer(recordId);
    }
    return success;
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
        QStringList range = timeRange.split("|");
        if (range.size() == 2) {
            condition += QString(" AND create_time BETWEEN '%1' AND '%2'").arg(range.at(0)).arg(range.at(1));
        }
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
        // 兼容旧数据：优先取 category，如果为空取 type (旧版存的是分类名)
        QString category = query.value("category").toString();
        if (category.isEmpty()) {
            category = query.value("type").toString();
        }
        record.setType(category); 
        
        record.setRemark(query.value("remark").toString());
        if (record.getRemark().isEmpty()) {
            record.setRemark(query.value("description").toString());
        }
        
        record.setVoucherPath(query.value("voucher_path").toString());
        record.setIsDeleted(query.value("is_deleted").toInt());
        record.setDeleteTime(query.value("delete_time").toString());
        record.setCreateTime(query.value("create_time").toString());
        record.setModifyTime(query.value("modify_time").toString());
        records.append(record);
    }

    return records;
}

//按日期范围查询
QList<AccountRecord> AccountManager::queryRecordsByDateRange(int userId,
                                                             const QDate& startDate,
                                                             const QDate& endDate,
                                                             bool isDeleted) {
    QString start = startDate.toString("yyyy-MM-dd 00:00:00");
    QString end = endDate.toString("yyyy-MM-dd 23:59:59");

    return queryAccountRecord(userId, start + "|" + end, "", 0, 0, isDeleted);
}

//按类型查询
QList<AccountRecord> AccountManager::queryRecordsByType(int userId,
                                                        const QString& type,
                                                        bool isDeleted) {
    return queryAccountRecord(userId, "", type, 0, 0, isDeleted);
}

//获取指定月份的所有记录
QList<AccountRecord> AccountManager::queryMonthlyRecords(int userId, int year, int month, bool isDeleted) {
    QDate firstDayOfMonth(year, month, 1);
    QDate lastDayOfMonth = firstDayOfMonth.addMonths(1).addDays(-1);

    return queryRecordsByDateRange(userId, firstDayOfMonth, lastDayOfMonth, isDeleted);
}

// 获取记录总数
int AccountManager::getRecordCount(int userId, bool isDeleted) {
    QString sql = QString("SELECT COUNT(*) FROM account_record WHERE user_id = %1 AND is_deleted = %2")
    .arg(userId).arg(isDeleted ? 1 : 0);

    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QStringList AccountManager::getPresetTypes() {
    return {"餐饮", "购物", "交通", "居住", "娱乐", "医疗", "教育", "人情", "其他"};
}

QList<AccountRecord> AccountManager::queryRecordsByAmountRange(int userId,
                                                               double minAmount,
                                                               double maxAmount,
                                                               bool isDeleted) {
    return queryAccountRecord(userId, "", "", minAmount, maxAmount, isDeleted);
}

void AccountManager::syncRecordToServer(const AccountRecord& record)
{
    // 获取全局 TCP 客户端
    TcpClient* client = TcpClient::getInstance();
    if (!client || !client->isConnected()) {
        qDebug() << "服务器未连接，账单仅保存到本地";
        return;
    }

    // 使用 addRecord 发送单条记录
    client->addRecord(record.getUserId(), record);
}

void AccountManager::syncEditRecordToServer(const AccountRecord& record) {
    TcpClient* client = TcpClient::getInstance();
    if (client && client->isConnected()) {
        client->editRecord(record.getUserId(), record);
    }
}

void AccountManager::syncDeleteRecordToServer(int recordId) {
    TcpClient* client = TcpClient::getInstance();
    if (client && client->isConnected()) {
        // 这里的 userId 需要从 record 中获取，或者从 UserManager 获取当前用户
        int userId = UserManager::getInstance()->getCurrentUser().getId();
        client->deleteRecord(userId, recordId);
    }
}

void AccountManager::syncRestoreRecordToServer(int recordId) {
    TcpClient* client = TcpClient::getInstance();
    if (client && client->isConnected()) {
        int userId = UserManager::getInstance()->getCurrentUser().getId();
        client->restoreRecord(userId, recordId);
    }
}

void AccountManager::syncPermanentDeleteRecordToServer(int recordId) {
    TcpClient* client = TcpClient::getInstance();
    if (client && client->isConnected()) {
        int userId = UserManager::getInstance()->getCurrentUser().getId();
        client->permanentDeleteRecord(userId, recordId);
    }
}
