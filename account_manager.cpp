#include "account_manager.h"
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QApplication>
#include "tcpclient.h"

AccountManager::AccountManager() {
    m_dbHelper = SqliteHelper::getInstance();
    m_dbHelper->openDatabase();
}

bool AccountManager::addAccountRecord(const AccountRecord& record) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 使用参数化查询，防止 SQL 注入，并避免特殊字符导致的问题
    QString sql = QString(R"(
        INSERT INTO account_record (user_id, amount, type, remark, voucher_path, is_deleted, create_time, modify_time)
        VALUES (?, ?, ?, ?, ?, 0, ?, ?)
    )");
    
    QVariantList params;
    params << record.getUserId()
           << record.getAmount()
           << record.getType()
           << record.getRemark()
           << record.getVoucherPath()
           << now
           << now;
    
    bool success = m_dbHelper->executeSqlWithParams(sql, params);
    
    if (success) {
        qDebug() << "账单保存成功：用户ID=" << record.getUserId() 
                 << "金额=" << record.getAmount() 
                 << "分类=" << record.getType()
                 << "时间=" << record.getCreateTime();

        // 同步到服务端
        syncRecordToServer(record);
    } else {
        qDebug() << "账单保存失败：" << m_dbHelper->getLastError();
    }
    
    return success;
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

//按日期范围查询
QList<AccountRecord> AccountManager::queryRecordsByDateRange(int userId,
                                                             const QDate& startDate,
                                                             const QDate& endDate,
                                                             bool isDeleted) {
    QString start = startDate.toString("yyyy-MM-dd 00:00:00");
    QString end = endDate.toString("yyyy-MM-dd 23:59:59");

    return queryAccountRecord(userId, start + "-" + end, "", 0, 0, isDeleted);
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


void AccountManager::syncRecordToServer(const AccountRecord& record)
{
    // 获取全局 TCP 客户端
    QObject* mainApp = qApp->property("tcpClient").value<QObject*>();
    if (!mainApp) {
        qDebug() << "未找到 TCP 客户端，账单仅保存到本地";
        return;
    }

    TcpClient* client = qobject_cast<TcpClient*>(mainApp);
    if (!client || !client->isConnected()) {
        qDebug() << "服务器未连接，账单仅保存到本地";
        return;
    }

    // 使用 syncBills 发送单条记录
    QList<AccountRecord> records;
    records << record;
    client->syncBills(records);
}
