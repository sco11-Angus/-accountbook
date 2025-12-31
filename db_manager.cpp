#include "db_manager.h"
#include "sqlite_helper.h"
#include "mysql_helper.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

// 静态成员初始化
DBManager* DBManager::m_instance = nullptr;
QMutex DBManager::m_mutex;

DBManager::DBManager() {}

DBManager::~DBManager() {
    if (m_remoteDb) {
        m_remoteDb->disconnect();
    }
}

DBManager* DBManager::getInstance() {
    if (m_instance == nullptr) {
        m_mutex.lock();
        if (m_instance == nullptr) {
            m_instance = new DBManager();
        }
        m_mutex.unlock();
    }
    return m_instance;
}

bool DBManager::initialize(const QString& localDbPath) {
    m_localDb = SqliteHelper::getInstance();
    bool localOk = m_localDb->openDatabase(localDbPath);
    if (!localOk) {
        qWarning() << "本地数据库初始化失败（可能在服务器环境）: " << m_localDb->getLastError();
        // 不直接返回 false，因为后续可能只使用远程数据库
    }

    m_remoteDb = MySqlHelper::getInstance();
    m_isInitialized = true;
    qDebug() << "DBManager初始化标记已设置";
    return true; // 总是返回 true，允许后续连接远程数据库
}

bool DBManager::connectRemoteDatabase(const QString& host, int port,
                                      const QString& username, const QString& password,
                                      const QString& database) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    if (!m_remoteDb->connect(host, port, username, password, database)) {
        setError("远程数据库连接失败: " + m_remoteDb->getLastError());
        return false;
    }

    qDebug() << "远程数据库连接成功";
    return true;
}

void DBManager::disconnectRemoteDatabase() {
    if (m_remoteDb) {
        m_remoteDb->disconnect();
    }
}

bool DBManager::isRemoteConnected() const {
    return m_remoteDb && m_remoteDb->isConnected();
}

// ============ 账单管理实现 ============

int DBManager::addBill(const BillData& bill) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return -1;
    }

    // 1. 写入本地数据库
    QString sql = R"(
        INSERT INTO account_record (user_id, amount, type, is_income, remark, voucher_path, create_time, modify_time)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";

    QVariantList params;
    params << bill.userId
           << bill.amount
           << (bill.type == 0 ? "expense" : "income")
           << bill.type
           << bill.description
           << bill.voucherPath
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    if (!m_localDb->executeSqlWithParams(sql, params)) {
        setError("本地账单插入失败: " + m_localDb->getLastError());
        return -1;
    }

    // 获取本地生成的ID
    QSqlQuery query = m_localDb->executeQuery("SELECT last_insert_rowid() as id");
    int localBillId = -1;
    if (query.next()) {
        localBillId = query.value("id").toInt();
    }

    // 2. 添加到同步队列
    if (isRemoteConnected()) {
        SyncQueueItem item;
        item.userId = bill.userId;
        item.entityId = localBillId;
        item.entityType = "bill";
        item.operation = "insert";
        item.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        item.status = 0; // 待同步

        // 序列化为JSON
        QJsonObject json;
        json["id"] = localBillId;
        json["amount"] = bill.amount;
        json["type"] = bill.type;
        json["description"] = bill.description;
        json["paymentMethod"] = bill.paymentMethod;
        item.payload = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));

        addToSyncQueue(item);
    }

    qDebug() << "新增账单成功，ID:" << localBillId;
    return localBillId;
}

QList<BillData> DBManager::getBillByMonth(int userId, int year, int month) {
    QList<BillData> bills;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return bills;
    }

    QString startDate = QString("%1-%2-01").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0'));
    QString endDate = QString("%1-%2-31").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0'));

    QString sql = R"(
        SELECT * FROM account_record
        WHERE user_id = ? AND date(create_time) BETWEEN ? AND ? AND is_deleted = 0
        ORDER BY create_time DESC
    )";

    QVariantList params;
    params << userId << startDate << endDate;

    QSqlQuery query = m_localDb->executeQueryWithParams(sql, params);
    while (query.next()) {
        BillData bill;
        bill.id = query.value("id").toInt();
        bill.userId = query.value("user_id").toInt();
        bill.amount = query.value("amount").toDouble();
        bill.type = query.value("is_income").toInt();
        bill.description = query.value("remark").toString();
        bill.date = query.value("create_time").toString();
        bills.append(bill);
    }

    return bills;
}

QList<BillData> DBManager::getBillByDateRange(int userId, const QString& startDate, const QString& endDate) {
    QList<BillData> bills;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return bills;
    }

    QString sql = R"(
        SELECT * FROM account_record
        WHERE user_id = ? AND date(create_time) BETWEEN ? AND ? AND is_deleted = 0
        ORDER BY create_time DESC
    )";

    QVariantList params;
    params << userId << startDate << endDate;

    QSqlQuery query = m_localDb->executeQueryWithParams(sql, params);
    while (query.next()) {
        BillData bill;
        bill.id = query.value("id").toInt();
        bill.userId = query.value("user_id").toInt();
        bill.amount = query.value("amount").toDouble();
        bill.type = query.value("is_income").toInt();
        bill.description = query.value("remark").toString();
        bill.date = query.value("create_time").toString();
        bills.append(bill);
    }

    return bills;
}

BillQueryResult DBManager::getBillByCategoryStats(int userId, int year, int month) {
    BillQueryResult result;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return result;
    }

    // 获取该月所有账单
    QList<BillData> bills = getBillByMonth(userId, year, month);

    result.totalCount = bills.size();
    for (const BillData& bill : bills) {
        if (bill.type == 1) {
            result.totalIncome += bill.amount;
        } else {
            result.totalExpense += bill.amount;
        }
    }
    result.netAmount = result.totalIncome - result.totalExpense;
    result.bills = bills;

    return result;
}

BillData DBManager::getBillById(int billId) {
    BillData bill;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return bill;
    }

    QString sql = "SELECT * FROM account_record WHERE id = ?";
    QVariantList params;
    params << billId;

    QSqlQuery query = m_localDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        bill.id = query.value("id").toInt();
        bill.userId = query.value("user_id").toInt();
        bill.amount = query.value("amount").toDouble();
        bill.type = query.value("is_income").toInt();
        bill.description = query.value("remark").toString();
        bill.date = query.value("create_time").toString();
    }

    return bill;
}

bool DBManager::updateBill(const BillData& bill) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    QString sql = R"(
        UPDATE account_record
        SET amount = ?, type = ?, is_income = ?, remark = ?, modify_time = ?
        WHERE id = ?
    )";

    QVariantList params;
    params << bill.amount
           << (bill.type == 0 ? "expense" : "income")
           << bill.type
           << bill.description
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
           << bill.id;

    if (!m_localDb->executeSqlWithParams(sql, params)) {
        setError("更新账单失败: " + m_localDb->getLastError());
        return false;
    }

    // 添加到同步队列
    if (isRemoteConnected()) {
        SyncQueueItem item;
        item.userId = bill.userId;
        item.entityId = bill.id;
        item.entityType = "bill";
        item.operation = "update";
        item.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        item.status = 0;
        addToSyncQueue(item);
    }

    return true;
}

bool DBManager::deleteBill(int billId) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    QString sql = R"(
        UPDATE account_record
        SET is_deleted = 1, delete_time = ?
        WHERE id = ?
    )";

    QVariantList params;
    params << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << billId;

    if (!m_localDb->executeSqlWithParams(sql, params)) {
        setError("删除账单失败: " + m_localDb->getLastError());
        return false;
    }

    return true;
}

bool DBManager::permanentlyDeleteBill(int billId) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    QString sql = "DELETE FROM account_record WHERE id = ?";
    QVariantList params;
    params << billId;

    if (!m_localDb->executeSqlWithParams(sql, params)) {
        setError("永久删除账单失败: " + m_localDb->getLastError());
        return false;
    }

    return true;
}

int DBManager::getBillCount(int userId, int year, int month) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return 0;
    }

    QString sql = "SELECT COUNT(*) as count FROM account_record WHERE user_id = ? AND is_deleted = 0";
    QVariantList params;
    params << userId;

    if (year > 0 && month > 0) {
        QString startDate = QString("%1-%2-01").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0'));
        QString endDate = QString("%1-%2-31").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0'));
        sql += " AND date(create_time) BETWEEN ? AND ?";
        params << startDate << endDate;
    }

    QSqlQuery query = m_localDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        return query.value("count").toInt();
    }

    return 0;
}

// ============ 账本管理实现 ============

int DBManager::addAccountBook(const AccountBookData& book) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return -1;
    }

    QString sql = R"(
        INSERT INTO account_book (user_id, name, description, icon, create_time)
        VALUES (?, ?, ?, ?, ?)
    )";

    QVariantList params;
    params << book.userId << book.name << book.description << book.icon
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("新增账本失败: " + m_remoteDb->getLastError());
        return -1;
    }

    qDebug() << "新增账本成功";
    return 1;
}

QList<AccountBookData> DBManager::getAccountBooks(int userId) {
    QList<AccountBookData> books;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return books;
    }

    QString sql = "SELECT * FROM account_book WHERE user_id = ? AND is_deleted = 0 ORDER BY sort_order";
    QVariantList params;
    params << userId;

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    while (query.next()) {
        AccountBookData book;
        book.id = query.value("id").toInt();
        book.userId = query.value("user_id").toInt();
        book.name = query.value("name").toString();
        book.description = query.value("description").toString();
        book.icon = query.value("icon").toString();
        books.append(book);
    }

    return books;
}

AccountBookData DBManager::getAccountBookById(int bookId) {
    AccountBookData book;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return book;
    }

    QString sql = "SELECT * FROM account_book WHERE id = ?";
    QVariantList params;
    params << bookId;

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        book.id = query.value("id").toInt();
        book.userId = query.value("user_id").toInt();
        book.name = query.value("name").toString();
        book.description = query.value("description").toString();
        book.icon = query.value("icon").toString();
    }

    return book;
}

bool DBManager::updateAccountBook(const AccountBookData& book) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE account_book SET name = ?, description = ?, icon = ? WHERE id = ?";
    QVariantList params;
    params << book.name << book.description << book.icon << book.id;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("更新账本失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

bool DBManager::deleteAccountBook(int bookId) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE account_book SET is_deleted = 1 WHERE id = ?";
    QVariantList params;
    params << bookId;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("删除账本失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

// ============ 账单分类管理实现 ============

int DBManager::addBillCategory(const BillCategoryData& category) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return -1;
    }

    QString sql = R"(
        INSERT INTO bill_category (user_id, name, type, icon, color, create_time)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    QVariantList params;
    params << category.userId << category.name << category.type << category.icon << category.color
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("新增分类失败: " + m_remoteDb->getLastError());
        return -1;
    }

    return 1;
}

QList<BillCategoryData> DBManager::getBillCategories(int userId, int type) {
    QList<BillCategoryData> categories;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return categories;
    }

    QString sql = "SELECT * FROM bill_category WHERE user_id = ? AND is_deleted = 0";
    QVariantList params;
    params << userId;

    if (type >= 0) {
        sql += " AND type = ?";
        params << type;
    }

    sql += " ORDER BY sort_order";

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    while (query.next()) {
        BillCategoryData category;
        category.id = query.value("id").toInt();
        category.userId = query.value("user_id").toInt();
        category.name = query.value("name").toString();
        category.type = query.value("type").toInt();
        category.icon = query.value("icon").toString();
        category.color = query.value("color").toString();
        categories.append(category);
    }

    return categories;
}

BillCategoryData DBManager::getBillCategoryById(int categoryId) {
    BillCategoryData category;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return category;
    }

    QString sql = "SELECT * FROM bill_category WHERE id = ?";
    QVariantList params;
    params << categoryId;

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        category.id = query.value("id").toInt();
        category.userId = query.value("user_id").toInt();
        category.name = query.value("name").toString();
        category.type = query.value("type").toInt();
        category.icon = query.value("icon").toString();
        category.color = query.value("color").toString();
    }

    return category;
}

bool DBManager::updateBillCategory(const BillCategoryData& category) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE bill_category SET name = ?, icon = ?, color = ? WHERE id = ?";
    QVariantList params;
    params << category.name << category.icon << category.color << category.id;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("更新分类失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

bool DBManager::deleteBillCategory(int categoryId) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE bill_category SET is_deleted = 1 WHERE id = ?";
    QVariantList params;
    params << categoryId;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("删除分类失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

// ============ 用户管理实现 ============

int DBManager::addUser(const UserData& user) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return -1;
    }

    QString sql = R"(
        INSERT INTO user (account, password, nickname, gender, create_time)
        VALUES (?, ?, ?, ?, ?)
    )";

    QVariantList params;
    params << user.account << user.password << user.nickname << user.gender
           << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("新增用户失败: " + m_remoteDb->getLastError());
        return -1;
    }

    return 1;
}

UserData DBManager::getUserById(int userId) {
    UserData user;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return user;
    }

    QString sql = "SELECT * FROM user WHERE id = ? AND is_deleted = 0";
    QVariantList params;
    params << userId;

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        user.id = query.value("id").toInt();
        user.account = query.value("account").toString();
        user.password = query.value("password").toString();
        user.nickname = query.value("nickname").toString();
        user.gender = query.value("gender").toInt();
    }

    return user;
}

UserData DBManager::getUserByAccount(const QString& account) {
    UserData user;

    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return user;
    }

    QString sql = "SELECT * FROM user WHERE account = ? AND is_deleted = 0";
    QVariantList params;
    params << account;

    QSqlQuery query = m_remoteDb->executeQueryWithParams(sql, params);
    if (query.next()) {
        user.id = query.value("id").toInt();
        user.account = query.value("account").toString();
        user.password = query.value("password").toString();
        user.nickname = query.value("nickname").toString();
        user.gender = query.value("gender").toInt();
    }

    return user;
}

bool DBManager::updateUser(const UserData& user) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE user SET nickname = ?, gender = ?, pay_method = ?, update_time = CURRENT_TIMESTAMP WHERE id = ?";
    QVariantList params;
    params << user.nickname << user.gender << user.payMethod << user.id;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("更新用户失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

bool DBManager::deleteUser(int userId) {
    if (!m_isInitialized || !m_remoteDb) {
        setError("DBManager未初始化或无远程数据库");
        return false;
    }

    QString sql = "UPDATE user SET is_deleted = 1 WHERE id = ?";
    QVariantList params;
    params << userId;

    if (!m_remoteDb->executeSqlWithParams(sql, params)) {
        setError("删除用户失败: " + m_remoteDb->getLastError());
        return false;
    }

    return true;
}

// ============ 同步管理实现 ============

bool DBManager::syncBillsFromRemote(int userId, const QList<BillData>& bills) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    for (const BillData& bill : bills) {
        addBill(bill);
    }

    return true;
}

QList<BillData> DBManager::getUnsyncedBills(int userId) {
    QList<BillData> bills;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return bills;
    }

    // 从同步队列中获取待同步的账单
    QString sql = "SELECT * FROM sync_queue WHERE user_id = ? AND status = 0 AND entity_type = 'bill'";
    QVariantList params;
    params << userId;

    // 这里需要配合sync_manager实现
    return bills;
}

bool DBManager::markBillAsSynced(int billId, int remoteBillId) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    // 标记本地账单为已同步
    QString sql = "UPDATE account_record SET sync_status = 0 WHERE id = ?";
    QVariantList params;
    params << billId;

    return m_localDb->executeSqlWithParams(sql, params);
}

bool DBManager::markBillSyncFailed(int billId, const QString& errorMsg) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    QString sql = "UPDATE account_record SET sync_status = 2, sync_error_msg = ? WHERE id = ?";
    QVariantList params;
    params << errorMsg << billId;

    return m_localDb->executeSqlWithParams(sql, params);
}

QList<SyncQueueItem> DBManager::getPendingSyncItems(int userId, int limit) {
    QList<SyncQueueItem> items;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return items;
    }

    // 从同步队列表获取待同步项
    QString sql = "SELECT * FROM sync_queue WHERE user_id = ? AND status = 0 LIMIT ?";
    QVariantList params;
    params << userId << limit;

    // 这里需要根据实际的sync_queue表结构查询
    return items;
}

bool DBManager::addToSyncQueue(const SyncQueueItem& item) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    // 添加到本地SQLite的同步队列表
    // 实现细节依赖于是否在SQLite中也建立了同步队列表

    return true;
}

bool DBManager::removeSyncQueueItem(int itemId) {
    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return false;
    }

    return true;
}

SyncStatistics DBManager::getSyncStatistics(int userId) {
    SyncStatistics stats;

    if (!m_isInitialized) {
        setError("DBManager未初始化");
        return stats;
    }

    // 从同步队列获取统计信息
    return stats;
}

void DBManager::setError(const QString& error) {
    m_lastError = error;
    qWarning() << error;
}
