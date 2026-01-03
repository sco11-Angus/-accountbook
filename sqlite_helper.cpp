#include "sqlite_helper.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QDateTime>

// 静态成员初始化
SqliteHelper* SqliteHelper::m_instance = nullptr;
QMutex SqliteHelper::m_mutex;

SqliteHelper::SqliteHelper() {}

SqliteHelper::~SqliteHelper() {
    closeDatabase();
}

SqliteHelper* SqliteHelper::getInstance() {
    if (m_instance == nullptr) {
        m_mutex.lock();
        if (m_instance == nullptr) {
            m_instance = new SqliteHelper();
        }
        m_mutex.unlock();
    }
    return m_instance;
}

bool SqliteHelper::openDatabase(const QString& dbPath) {
    m_dbPath = dbPath;

    // 检查连接是否已存在
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
        if (m_db.isOpen()) return true;  // 已打开直接返回
        else return m_db.open();  // 关闭状态则重新打开
    }

    // 创建新连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = "数据库打开失败：" + m_db.lastError().text();
        qDebug() << m_lastError;
        return false;
    }

    // 启用外键约束（保障用户与收支记录的关联完整性）
    enableForeignKeys();
    
    // 创建用户表
    QString createUserTable = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT UNIQUE NOT NULL,  -- 手机号/邮箱，唯一
            password TEXT NOT NULL,       -- 加密存储
            nickname TEXT DEFAULT '默认昵称',
            avatar TEXT DEFAULT '',       -- 头像路径
            gender INTEGER DEFAULT 0,     -- 0:未知 1:男 2:女
            pay_method TEXT DEFAULT '',   -- 常用支付方式
            login_fail_count INTEGER DEFAULT 0,  -- 登录失败次数
            lock_time TEXT DEFAULT '',    -- 账号锁定时间
            create_time TEXT NOT NULL     -- 创建时间
        );
    )";
    if (!executeSql(createUserTable)) return false;

    // 创建收支记录表（通用：兼容客户端与服务端）
    QString createAccountTable = R"(
        CREATE TABLE IF NOT EXISTS account_record (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,      -- 用户ID
            bill_date TEXT,                -- 账单日期 (yyyy-MM-dd HH:mm:ss)
            amount REAL NOT NULL,          -- 金额
            type INTEGER DEFAULT 0,        -- 类型 (0=支出, 1=收入)
            category TEXT,                 -- 分类名称 (如: 餐饮, 服饰)
            remark TEXT,                   -- 备注/描述
            description TEXT,              -- 备注 (服务端兼容)
            voucher_path TEXT,             -- 凭证路径
            is_deleted INTEGER DEFAULT 0,  -- 是否删除
            delete_time TEXT,              -- 删除时间
            create_time TEXT NOT NULL,     -- 创建时间
            modify_time TEXT,              -- 修改时间
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE
        );
    )";
    if (!executeSql(createAccountTable)) return false;

    // 升级表结构：确保旧表也有新列
    executeSql("ALTER TABLE account_record ADD COLUMN category TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN remark TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN description TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN bill_date TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN modify_time TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN voucher_path TEXT");
    executeSql("ALTER TABLE account_record ADD COLUMN is_deleted INTEGER DEFAULT 0");
    executeSql("ALTER TABLE account_record ADD COLUMN delete_time TEXT");

    // 创建预算表
    QString createBudgetTable = R"(
        CREATE TABLE IF NOT EXISTS budget (
            user_id INTEGER PRIMARY KEY,
            daily_budget REAL DEFAULT 0,
            monthly_budget REAL DEFAULT 0,
            yearly_budget REAL DEFAULT 0,
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE
        );
    )";
    if (!executeSql(createBudgetTable)) return false;

    // --- 服务端兼容表结构 ---
    // 创建账本表
    QString createAccountBookTable = R"(
        CREATE TABLE IF NOT EXISTS account_book (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            icon TEXT,
            sort_order INTEGER DEFAULT 0,
            create_time TEXT NOT NULL,
            update_time TEXT,
            is_deleted INTEGER DEFAULT 0,
            sync_status INTEGER DEFAULT 0,
            last_sync_time TEXT,
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE
        );
    )";
    if (!executeSql(createAccountBookTable)) return false;

    // 创建账单分类表
    QString createBillCategoryTable = R"(
        CREATE TABLE IF NOT EXISTS bill_category (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            type INTEGER NOT NULL,
            icon TEXT,
            color TEXT,
            sort_order INTEGER DEFAULT 0,
            create_time TEXT NOT NULL,
            update_time TEXT,
            is_deleted INTEGER DEFAULT 0,
            sync_status INTEGER DEFAULT 0,
            last_sync_time TEXT,
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            UNIQUE(user_id, name)
        );
    )";
    if (!executeSql(createBillCategoryTable)) return false;

    // 创建账单表 (服务端同步用)
    QString createBillTable = R"(
        CREATE TABLE IF NOT EXISTS bill (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            book_id INTEGER NOT NULL,
            category_id INTEGER NOT NULL,
            bill_date TEXT NOT NULL,
            amount REAL NOT NULL,
            type INTEGER NOT NULL,
            description TEXT,
            payment_method TEXT,
            merchant TEXT,
            tag TEXT,
            voucher_path TEXT,
            voucher_url TEXT,
            is_deleted INTEGER DEFAULT 0,
            delete_time TEXT,
            create_time TEXT NOT NULL,
            update_time TEXT,
            sync_status INTEGER DEFAULT 0,
            local_id INTEGER DEFAULT 0,
            last_sync_time TEXT,
            sync_error_msg TEXT,
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            FOREIGN KEY (book_id) REFERENCES account_book(id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES bill_category(id) ON DELETE RESTRICT
        );
    )";
    if (!executeSql(createBillTable)) return false;

    // 创建索引提升查询性能
    createIndexes();

    // 初始化数据库版本（用于后续 schema 升级）
    initializeVersion();

    // 插入默认数据（确保外键约束有基本保障）
    insertDefaultData();

    qDebug() << "数据库初始化成功：" << dbPath;
    return true;
}

bool SqliteHelper::insertDefaultData() {
    // 1. 确保至少有一个默认用户 (id=1)
    QString checkUser = "SELECT COUNT(*) FROM user WHERE id = 1";
    QSqlQuery query = executeQuery(checkUser);
    if (query.next() && query.value(0).toInt() == 0) {
        QString insertUser = R"(
            INSERT INTO user (id, account, password, nickname, create_time) 
            VALUES (1, 'admin', '123456', '管理员', datetime('now'))
        )";
        executeSql(insertUser);
    }

    // 2. 确保至少有一个默认账本 (id=1)
    QString checkBook = "SELECT COUNT(*) FROM account_book WHERE id = 1";
    query = executeQuery(checkBook);
    if (query.next() && query.value(0).toInt() == 0) {
        QString insertBook = R"(
            INSERT INTO account_book (id, user_id, name, create_time) 
            VALUES (1, 1, '默认账本', datetime('now'))
        )";
        executeSql(insertBook);
    }

    // 3. 确保至少有一个默认分类 (id=1)
    QString checkCategory = "SELECT COUNT(*) FROM bill_category WHERE id = 1";
    query = executeQuery(checkCategory);
    if (query.next() && query.value(0).toInt() == 0) {
        QString insertCategory = R"(
            INSERT INTO bill_category (id, user_id, name, type, create_time) 
            VALUES (1, 1, '其他', 0, datetime('now'))
        )";
        executeSql(insertCategory);
    }
    return true;
}

void SqliteHelper::closeDatabase() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool SqliteHelper::executeSql(const QString& sql) {
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = "SQL执行失败：" + sql + " 错误：" + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

bool SqliteHelper::executeSqlWithParams(const QString& sql, const QVariantList& params) {
    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    if (!query.exec()) {
        m_lastError = "参数化SQL执行失败：" + sql + " 错误：" + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

QSqlQuery SqliteHelper::executeQuery(const QString& sql) {
    QMutexLocker locker(&m_mutex);  // 线程安全
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = "SQL查询失败：" + sql + " 错误：" + query.lastError().text();
        qDebug() << m_lastError;
    }
    return query;
}

QSqlQuery SqliteHelper::executeQueryWithParams(const QString& sql, const QVariantList& params) {
    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    if (!query.exec()) {
        m_lastError = "参数化SQL查询失败：" + sql + " 错误：" + query.lastError().text();
        qDebug() << m_lastError;
    }
    return query;
}

QSqlDatabase SqliteHelper::getDatabase() {
    return m_db;
}

// ============ 事务管理 ============
bool SqliteHelper::beginTransaction() {
    if (!m_db.transaction()) {
        m_lastError = "开启事务失败：" + m_db.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

bool SqliteHelper::commitTransaction() {
    if (!m_db.commit()) {
        m_lastError = "提交事务失败：" + m_db.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

bool SqliteHelper::rollbackTransaction() {
    if (!m_db.rollback()) {
        m_lastError = "回滚事务失败：" + m_db.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

// ============ 数据库备份与恢复 ============
bool SqliteHelper::createBackup(const QString& backupDir) {
    QFile dbFile(m_dbPath);
    if (!dbFile.exists()) {
        m_lastError = "数据库文件不存在：" + m_dbPath;
        qDebug() << m_lastError;
        return false;
    }

    QDir dir(backupDir);
    if (!dir.exists()) {
        if (!dir.mkpath(backupDir)) {
            m_lastError = "无法创建备份目录：" + backupDir;
            qDebug() << m_lastError;
            return false;
        }
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString backupFileName = "account_book_" + timestamp + ".db";
    QString backupFilePath = backupDir + "/" + backupFileName;

    if (!QFile::copy(m_dbPath, backupFilePath)) {
        m_lastError = "备份失败：无法复制数据库文件";
        qDebug() << m_lastError;
        return false;
    }

    qDebug() << "备份成功：" << backupFilePath;

    // 清理过期备份（仅保留最近10个）
    QStringList backups = listBackups(backupDir);
    if (backups.size() > 10) {
        for (int i = 0; i < backups.size() - 10; ++i) {
            deleteBackup(backupDir + "/" + backups[i]);
        }
    }

    return true;
}

bool SqliteHelper::restoreBackup(const QString& backupFilePath) {
    QFile backupFile(backupFilePath);
    if (!backupFile.exists()) {
        m_lastError = "备份文件不存在：" + backupFilePath;
        qDebug() << m_lastError;
        return false;
    }

    closeDatabase();

    QFile targetFile(m_dbPath);
    if (targetFile.exists()) {
        if (!targetFile.remove()) {
            m_lastError = "无法删除旧数据库文件";
            qDebug() << m_lastError;
            return false;
        }
    }

    if (!QFile::copy(backupFilePath, m_dbPath)) {
        m_lastError = "恢复失败：无法复制备份文件";
        qDebug() << m_lastError;
        return false;
    }

    // 重新打开数据库
    return openDatabase(m_dbPath);
}

QStringList SqliteHelper::listBackups(const QString& backupDir) {
    QDir dir(backupDir);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList filters;
    filters << "account_book_*.db";

    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    QStringList result;
    for (const QFileInfo& fileInfo : fileList) {
        result.append(fileInfo.fileName());
    }

    return result;
}

bool SqliteHelper::deleteBackup(const QString& backupFilePath) {
    QFile file(backupFilePath);
    if (!file.exists()) {
        m_lastError = "备份文件不存在：" + backupFilePath;
        return false;
    }

    if (!file.remove()) {
        m_lastError = "删除备份文件失败：" + backupFilePath;
        qDebug() << m_lastError;
        return false;
    }

    qDebug() << "删除备份成功：" << backupFilePath;
    return true;
}

// ============ 数据库维护与优化 ============
bool SqliteHelper::optimizeDatabase() {
    // VACUUM操作重新整理数据库文件
    if (!executeSql("VACUUM")) {
        m_lastError = "数据库优化失败";
        return false;
    }

    // ANALYZE操作更新统计信息以优化查询
    if (!executeSql("ANALYZE")) {
        m_lastError = "数据库分析失败";
        return false;
    }

    qDebug() << "数据库优化完成";
    return true;
}

bool SqliteHelper::checkIntegrity() {
    QString sql = "PRAGMA integrity_check";
    QSqlQuery query = executeQuery(sql);

    if (query.next()) {
        QString result = query.value(0).toString();
        if (result == "ok") {
            qDebug() << "数据库完整性检查通过";
            return true;
        } else {
            m_lastError = "数据库完整性检查失败：" + result;
            qDebug() << m_lastError;
            return false;
        }
    }

    return false;
}

bool SqliteHelper::enableForeignKeys() {
    return executeSql("PRAGMA foreign_keys = ON");
}

bool SqliteHelper::fixOrphanedRecords() {
    QString sql = "DELETE FROM account_record WHERE user_id NOT IN (SELECT id FROM user)";
    if (!executeSql(sql)) {
        m_lastError = "删除孤立记录失败";
        return false;
    }
    qDebug() << "孤立记录清理完成";
    return true;
}

QString SqliteHelper::getDatabaseStatistics() {
    QString stats;

    // 获取数据库文件大小
    QFile dbFile(m_dbPath);
    if (dbFile.exists()) {
        int dbSize = dbFile.size();
        stats += QString("数据库大小：%1 KB\n").arg(dbSize / 1024);
    }

    // 获取用户统计
    QString sql = "SELECT COUNT(*) as count FROM user";
    QSqlQuery query = executeQuery(sql);
    if (query.next()) {
        stats += QString("用户总数：%1\n").arg(query.value("count").toInt());
    }

    // 获取记录统计
    sql = "SELECT COUNT(*) as count FROM account_record WHERE is_deleted = 0";
    query = executeQuery(sql);
    if (query.next()) {
        stats += QString("有效记录：%1\n").arg(query.value("count").toInt());
    }

    // 获取已删除记录统计
    sql = "SELECT COUNT(*) as count FROM account_record WHERE is_deleted = 1";
    query = executeQuery(sql);
    if (query.next()) {
        stats += QString("已删除记录：%1\n").arg(query.value("count").toInt());
    }

    // 获取总收入
    sql = "SELECT SUM(amount) as total FROM account_record WHERE is_income = 1 AND is_deleted = 0";
    query = executeQuery(sql);
    if (query.next()) {
        double total = query.value("total").toDouble();
        stats += QString("总收入：%.2f\n").arg(total);
    }

    // 获取总支出
    sql = "SELECT SUM(amount) as total FROM account_record WHERE is_income = 0 AND is_deleted = 0";
    query = executeQuery(sql);
    if (query.next()) {
        double total = query.value("total").toDouble();
        stats += QString("总支出：%.2f\n").arg(total);
    }

    return stats;
}

// ============ 版本管理 ============
bool SqliteHelper::initializeVersion() {
    createVersionTable();

    QString sql = "SELECT version FROM db_version LIMIT 1";
    QSqlQuery query = executeQuery(sql);

    if (!query.next()) {
        QString insertVersion = "INSERT INTO db_version (version, migrate_time) VALUES (1, datetime('now'))";
        return executeSql(insertVersion);
    }

    return true;
}

bool SqliteHelper::createVersionTable() {
    QString createVersionTable = R"(
        CREATE TABLE IF NOT EXISTS db_version (
            id INTEGER PRIMARY KEY,
            version INTEGER NOT NULL,
            migrate_time TEXT NOT NULL
        );
    )";

    return executeSql(createVersionTable);
}

int SqliteHelper::getCurrentVersion() {
    QString sql = "SELECT version FROM db_version ORDER BY id DESC LIMIT 1";
    QSqlQuery query = executeQuery(sql);

    if (query.next()) {
        return query.value("version").toInt();
    }

    return 0;
}

bool SqliteHelper::setVersion(int version) {
    QString sql = "INSERT INTO db_version (version, migrate_time) VALUES (?, datetime('now'))";
    QVariantList params;
    params << version;

    return executeSqlWithParams(sql, params);
}

// ============ 错误处理 ============
QString SqliteHelper::getLastError() const {
    return m_lastError;
}

void SqliteHelper::clearError() {
    m_lastError = "";
}

// ============ 私有方法 ============
bool SqliteHelper::createIndexes() {
    QStringList stmts = {
        "CREATE INDEX IF NOT EXISTS idx_user_account ON user(account)",
        "CREATE INDEX IF NOT EXISTS idx_account_user_id ON account_record(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_account_create_time ON account_record(create_time)"
    };

    bool txnStarted = beginTransaction();
    for (const QString& sql : stmts) {
        if (!executeSql(sql)) {
            if (txnStarted) {
                rollbackTransaction();
            }
            return false;
        }
    }
    if (txnStarted && !commitTransaction()) {
        return false;
    }
    return true;
}

