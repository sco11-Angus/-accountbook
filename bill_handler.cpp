#include "bill_handler.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QSqlDatabase>
#include <QJsonDocument>
#include "sqlite_helper.h"

bill_handler::bill_handler()
{
    m_accountManager = new AccountManager();
}

bill_handler::~bill_handler()
{
    if (m_accountManager) {
        delete m_accountManager;
        m_accountManager = nullptr;
    }
}

QJsonObject bill_handler::handleSyncBills(const QJsonObject& request)
{
    QJsonObject response;
    response["type"] = "sync_bills_response";
    
    // 检查请求参数
    if (!request.contains("bills") || !request["bills"].isArray()) {
        response["success"] = false;
        response["message"] = "请求参数错误：缺少账单数据";
        return response;
    }
    
    QJsonArray billsArray = request["bills"].toArray();
    if (billsArray.isEmpty()) {
        response["success"] = false;
        response["message"] = "账单列表为空";
        return response;
    }
    
    // 获取用户ID（如果请求中包含）
    int userId = request.contains("userId") ? request["userId"].toInt() : 0;
    
    // 开启事务处理批量同步
    QSqlDatabase db = SqliteHelper::getInstance()->getDatabase();
    db.transaction();
    
    int successCount = 0;
    int failCount = 0;
    QList<AccountRecord> recordsToSync;
    
    // 解析账单数据
    for (const QJsonValue& value : billsArray) {
        if (!value.isObject()) {
            failCount++;
            continue;
        }
        
        QJsonObject billObj = value.toObject();
        AccountRecord record = jsonToRecord(billObj);
        
        // 如果没有用户ID，尝试从记录中获取
        if (userId == 0 && record.getUserId() > 0) {
            userId = record.getUserId();
        }
        
        // 检查记录是否已存在（根据ID）
        if (record.getId() > 0) {
            // 更新现有记录
            if (m_accountManager->editAccountRecord(record)) {
                successCount++;
            } else {
                failCount++;
            }
        } else {
            // 新记录，添加到列表
            recordsToSync.append(record);
        }
    }
    
    // 批量添加新记录
    for (const AccountRecord& record : recordsToSync) {
        if (m_accountManager->addAccountRecord(record)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    // 提交事务
    if (successCount > 0) {
        db.commit();
        response["success"] = true;
        response["message"] = QString("同步成功：%1条记录，失败：%2条").arg(successCount).arg(failCount);
        response["successCount"] = successCount;
        response["failCount"] = failCount;
    } else {
        db.rollback();
        response["success"] = false;
        response["message"] = "同步失败：所有记录都无法处理";
        response["failCount"] = failCount;
    }
    
    qDebug() << "同步账单处理完成：" << response["message"].toString();
    return response;
}

QJsonObject bill_handler::handleQueryBills(const QJsonObject& request)
{
    QJsonObject response;
    response["type"] = "fetch_latest_response";
    
    // 检查请求参数
    if (!request.contains("userId")) {
        response["success"] = false;
        response["message"] = "请求参数错误：缺少用户ID";
        return response;
    }
    
    int userId = request["userId"].toInt();
    if (userId <= 0) {
        response["success"] = false;
        response["message"] = "用户ID无效";
        return response;
    }
    
    // 获取查询参数
    QString timeRange = request.contains("timeRange") ? request["timeRange"].toString() : "";
    QString type = request.contains("type") ? request["type"].toString() : "";
    double minAmount = request.contains("minAmount") ? request["minAmount"].toDouble() : 0;
    double maxAmount = request.contains("maxAmount") ? request["maxAmount"].toDouble() : 0;
    bool isDeleted = request.contains("isDeleted") ? request["isDeleted"].toBool() : false;
    
    // 如果提供了最后同步时间，只查询该时间之后的数据
    QString lastSyncTime = request.contains("lastSyncTime") ? request["lastSyncTime"].toString() : "";
    if (!lastSyncTime.isEmpty() && timeRange.isEmpty()) {
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        timeRange = QString("%1-%2").arg(lastSyncTime).arg(currentTime);
    }
    
    // 查询账单
    QList<AccountRecord> records = m_accountManager->queryAccountRecord(
        userId, timeRange, type, minAmount, maxAmount, isDeleted
    );
    
    // 转换为JSON数组
    QJsonArray billsArray = recordsToJsonArray(records);
    
    response["success"] = true;
    response["message"] = "查询成功";
    response["bills"] = billsArray;
    response["count"] = records.size();
    
    qDebug() << "查询账单完成，用户ID:" << userId << "记录数:" << records.size();
    return response;
}

QJsonObject bill_handler::handleBackupData(const QJsonObject& request)
{
    QJsonObject response;
    response["type"] = "backup_data_response";
    
    // 检查请求参数
    if (!request.contains("userId")) {
        response["success"] = false;
        response["message"] = "请求参数错误：缺少用户ID";
        return response;
    }
    
    int userId = request["userId"].toInt();
    if (userId <= 0) {
        response["success"] = false;
        response["message"] = "用户ID无效";
        return response;
    }
    
    // 获取备份路径（可选）
    QString backupPath = request.contains("backupPath") ? request["backupPath"].toString() : "";
    if (backupPath.isEmpty()) {
        // 默认备份路径：当前目录下的backup文件夹
        backupPath = "./backup";
    }
    
    // 创建备份目录
    QDir dir;
    if (!dir.exists(backupPath)) {
        if (!dir.mkpath(backupPath)) {
            response["success"] = false;
            response["message"] = "无法创建备份目录";
            return response;
        }
    }
    
    // 查询用户的所有账单数据（包括已删除的）
    QList<AccountRecord> allRecords = m_accountManager->queryAccountRecord(userId, "", "", 0, 0, false);
    QList<AccountRecord> deletedRecords = m_accountManager->queryAccountRecord(userId, "", "", 0, 0, true);
    allRecords.append(deletedRecords);
    
    // 生成备份文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupFileName = QString("%1/user_%2_backup_%3.json").arg(backupPath).arg(userId).arg(timestamp);
    
    // 创建备份JSON对象
    QJsonObject backupData;
    backupData["userId"] = userId;
    backupData["backupTime"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    backupData["recordCount"] = allRecords.size();
    backupData["records"] = recordsToJsonArray(allRecords);
    
    // 保存到文件
    QJsonDocument doc(backupData);
    QFile file(backupFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        response["success"] = false;
        response["message"] = "无法创建备份文件";
        return response;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    response["success"] = true;
    response["message"] = "备份成功";
    response["backupPath"] = backupFileName;
    response["recordCount"] = allRecords.size();
    response["backupTime"] = backupData["backupTime"].toString();
    
    qDebug() << "备份数据完成，用户ID:" << userId << "文件路径:" << backupFileName;
    return response;
}

QJsonObject bill_handler::recordToJson(const AccountRecord& record)
{
    QJsonObject obj;
    obj["id"] = record.getId();
    obj["userId"] = record.getUserId();
    obj["amount"] = record.getAmount();
    obj["type"] = record.getType();
    obj["remark"] = record.getRemark();
    obj["voucherPath"] = record.getVoucherPath();
    obj["isDeleted"] = record.getIsDeleted();
    obj["deleteTime"] = record.getDeleteTime();
    obj["createTime"] = record.getCreateTime();
    obj["modifyTime"] = record.getModifyTime();
    return obj;
}

AccountRecord bill_handler::jsonToRecord(const QJsonObject& json)
{
    AccountRecord record;
    
    if (json.contains("id")) record.setId(json["id"].toInt());
    if (json.contains("userId")) record.setUserId(json["userId"].toInt());
    if (json.contains("amount")) record.setAmount(json["amount"].toDouble());
    if (json.contains("type")) record.setType(json["type"].toString());
    if (json.contains("remark")) record.setRemark(json["remark"].toString());
    if (json.contains("voucherPath")) record.setVoucherPath(json["voucherPath"].toString());
    if (json.contains("isDeleted")) record.setIsDeleted(json["isDeleted"].toInt());
    if (json.contains("deleteTime")) record.setDeleteTime(json["deleteTime"].toString());
    if (json.contains("createTime")) record.setCreateTime(json["createTime"].toString());
    if (json.contains("modifyTime")) record.setModifyTime(json["modifyTime"].toString());
    
    return record;
}

QJsonArray bill_handler::recordsToJsonArray(const QList<AccountRecord>& records)
{
    QJsonArray array;
    for (const AccountRecord& record : records) {
        array.append(recordToJson(record));
    }
    return array;
}
