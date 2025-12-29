#include "bill_handler.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantList>
#include "mysql_helper.h"

bill_handler::bill_handler()
    : m_dbHelper(MySqlHelper::getInstance())
{
}

bill_handler::~bill_handler()
{
    // MySqlHelper 是单例，不需要手动删除
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
    // 获取账本ID（可选，默认为1）
    int bookId = request.contains("bookId") ? request["bookId"].toInt() : 1;
    
    // 开启事务处理批量同步
    if (!m_dbHelper->beginTransaction()) {
        response["success"] = false;
        response["message"] = "无法开启事务";
        return response;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    // 解析账单数据并直接插入到 MySQL bill 表
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
        
        // 直接插入到 MySQL bill 表
        if (insertBillToDatabase(record, bookId)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    // 提交或回滚事务
    if (successCount > 0) {
        m_dbHelper->commitTransaction();
        response["success"] = true;
        response["message"] = QString("同步成功：%1条记录，失败：%2条").arg(successCount).arg(failCount);
        response["successCount"] = successCount;
        response["failCount"] = failCount;
        qDebug() << "【handleSyncBills】同步完成：" << response["message"].toString();
    } else {
        m_dbHelper->rollbackTransaction();
        response["success"] = false;
        response["message"] = "同步失败：所有记录都无法处理";
        response["failCount"] = failCount;
        qWarning() << "【handleSyncBills】同步失败，已回滚事务";
    }
    
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
    
    // 构造 SQL 查询 MySQL bill 表
    QString sql = QString(R"(
        SELECT id, user_id, category_id, amount, bill_date, type, 
               description, voucher_path, is_deleted, delete_time, 
               create_time, update_time
        FROM bill WHERE user_id = %1 AND is_deleted = %2
    )").arg(userId).arg(isDeleted ? 1 : 0);
    
    // 添加分类筛选
    if (!type.isEmpty()) {
        int categoryId = queryCategoryId(type, userId);
        if (categoryId > 0) {
            sql += QString(" AND category_id = %1").arg(categoryId);
        }
    }
    
    // 添加日期范围筛选
    if (!timeRange.isEmpty()) {
        QStringList parts = timeRange.split("-");
        if (parts.size() == 2) {
            sql += QString(" AND bill_date BETWEEN '%1' AND '%2'").arg(parts[0]).arg(parts[1]);
        }
    }
    
    // 添加金额范围筛选
    if (minAmount > 0 || maxAmount > 0) {
        if (minAmount > 0) sql += QString(" AND amount >= %1").arg(minAmount);
        if (maxAmount > 0) sql += QString(" AND amount <= %1").arg(maxAmount);
    }
    
    sql += " ORDER BY bill_date DESC";
    
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    QList<AccountRecord> records;
    
    while (query.next()) {
        AccountRecord record;
        record.setId(query.value("id").toInt());
        record.setUserId(query.value("user_id").toInt());
        record.setAmount(query.value("amount").toDouble());
        record.setCreateTime(query.value("bill_date").toString());
        record.setRemark(query.value("description").toString());
        record.setVoucherPath(query.value("voucher_path").toString());
        record.setIsDeleted(query.value("is_deleted").toInt());
        record.setDeleteTime(query.value("delete_time").toString());
        record.setModifyTime(query.value("update_time").toString());
        
        // 根据 category_id 查询分类名称
        int categoryId = query.value("category_id").toInt();
        QString categoryName = "未知";
        QString catSql = QString("SELECT name FROM bill_category WHERE id = %1").arg(categoryId);
        QSqlQuery catQuery = m_dbHelper->executeQuery(catSql);
        if (catQuery.next()) {
            categoryName = catQuery.value(0).toString();
        }
        record.setType(categoryName);
        
        records.append(record);
    }
    
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
    QString sql = QString(R"(
        SELECT id, user_id, category_id, amount, bill_date, type, 
               description, voucher_path, is_deleted, delete_time, 
               create_time, update_time
        FROM bill WHERE user_id = %1
        ORDER BY bill_date DESC
    )").arg(userId);
    
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    QList<AccountRecord> allRecords;
    
    while (query.next()) {
        AccountRecord record;
        record.setId(query.value("id").toInt());
        record.setUserId(query.value("user_id").toInt());
        record.setAmount(query.value("amount").toDouble());
        record.setCreateTime(query.value("bill_date").toString());
        record.setRemark(query.value("description").toString());
        record.setVoucherPath(query.value("voucher_path").toString());
        record.setIsDeleted(query.value("is_deleted").toInt());
        record.setDeleteTime(query.value("delete_time").toString());
        record.setModifyTime(query.value("update_time").toString());
        
        // 根据 category_id 查询分类名称
        int categoryId = query.value("category_id").toInt();
        QString categoryName = "未知";
        QString catSql = QString("SELECT name FROM bill_category WHERE id = %1").arg(categoryId);
        QSqlQuery catQuery = m_dbHelper->executeQuery(catSql);
        if (catQuery.next()) {
            categoryName = catQuery.value(0).toString();
        }
        record.setType(categoryName);
        
        allRecords.append(record);
    }
    
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

// ==================== 新增：直接操作MySQL bill表的方法 ====================

/**
 * @brief 查询分类ID
 * @param categoryName 分类名称（如"餐饮"、"交通"）
 * @param userId 用户ID
 * @return 分类ID，查不到返回 -1
 */
int bill_handler::queryCategoryId(const QString& categoryName, int userId)
{
    if (categoryName.isEmpty() || userId <= 0) {
        return -1;
    }
    
    QString sql = QString(R"(
        SELECT id FROM bill_category 
        WHERE user_id = %1 AND name = '%2' AND is_deleted = 0
        LIMIT 1
    )").arg(userId).arg(categoryName);
    
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    qWarning() << "【queryCategoryId】未找到分类：" << categoryName << "，用户ID：" << userId;
    return -1;
}

/**
 * @brief 将 AccountRecord 直接插入到 MySQL bill 表
 * @param record 账单记录
 * @param defaultBookId 默认账本ID（当无法确定时使用）
 * @return 是否插入成功
 */
bool bill_handler::insertBillToDatabase(const AccountRecord& record, int defaultBookId)
{
    if (record.getUserId() <= 0 || record.getAmount() == 0) {
        qWarning() << "【insertBillToDatabase】无效的记录：userId=" << record.getUserId() 
                  << "，amount=" << record.getAmount();
        return false;
    }
    
    // 1. 查询分类ID
    int categoryId = queryCategoryId(record.getType(), record.getUserId());
    if (categoryId <= 0) {
        qWarning() << "【insertBillToDatabase】分类不存在：" << record.getType();
        // 可以选择使用默认分类或返回失败
        categoryId = 1;  // 使用默认分类ID为1
    }
    
    // 2. 确定账单类型（0=支出，1=收入）
    // 根据 remark 中的关键字判断，或使用默认的 0（支出）
    int type = 0;  // 默认为支出
    
    // 3. 构造 SQL 插入语句
    QString sql = R"(
        INSERT INTO bill (
            user_id, book_id, category_id, bill_date, amount, type, 
            description, voucher_path, is_deleted, delete_time, 
            create_time, update_time, local_id
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    QVariantList params;
    params << record.getUserId()           // user_id
           << defaultBookId                 // book_id
           << categoryId                    // category_id
           << record.getCreateTime()        // bill_date
           << record.getAmount()            // amount
           << type                          // type（0=支出，1=收入）
           << record.getRemark()            // description
           << record.getVoucherPath()       // voucher_path
           << record.getIsDeleted()         // is_deleted
           << record.getDeleteTime()        // delete_time
           << record.getCreateTime()        // create_time
           << record.getModifyTime()        // update_time
           << record.getId();               // local_id（保存本地ID用于后续同步）
    
    bool success = m_dbHelper->executeSqlWithParams(sql, params);
    
    if (success) {
        qDebug() << "【insertBillToDatabase】成功插入账单：userId=" << record.getUserId() 
                << "，amount=" << record.getAmount() << "，category=" << record.getType();
    } else {
        qWarning() << "【insertBillToDatabase】插入失败：" << m_dbHelper->getLastError();
    }
    
    return success;
}

// ================================================================
