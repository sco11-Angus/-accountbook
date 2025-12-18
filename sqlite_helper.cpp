#include "sqlite_helper.h"

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
    //检查连接是否已存在，存在则直接返回
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
        // 如果连接已关闭，重新打开
        if (m_db.isOpen()) return true;
        else return m_db.open(); // 重新打开失败则返回false
    }

    // 连接不存在时，新建连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qDebug() << "数据库打开失败：" << m_db.lastError().text();
        return false;
    }

    // 创建用户表
    QString createUserTable = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT UNIQUE NOT NULL,  -- 手机号/邮箱
            password TEXT NOT NULL,
            nickname TEXT DEFAULT '默认昵称',
            avatar TEXT DEFAULT '',
            gender INTEGER DEFAULT 0,     -- 0:未知 1:男 2:女
            pay_method TEXT DEFAULT '',
            login_fail_count INTEGER DEFAULT 0,
            lock_time TEXT DEFAULT '',
            create_time TEXT NOT NULL
        );
    )";
    if (!executeSql(createUserTable)) return false;

    // 创建收支记录表（软删除）
    QString createAccountTable = R"(
        CREATE TABLE IF NOT EXISTS account_record (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            amount REAL NOT NULL,
            type TEXT NOT NULL,  -- 收支类型（餐饮/交通等）
            remark TEXT DEFAULT '',
            voucher_path TEXT DEFAULT '',
            is_deleted INTEGER DEFAULT 0,  -- 0:正常 1:回收站
            delete_time TEXT DEFAULT '',
            create_time TEXT NOT NULL,
            modify_time TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES user(id)
        );
    )";
    return executeSql(createAccountTable);
}

void SqliteHelper::closeDatabase() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool SqliteHelper::executeSql(const QString& sql) {
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        qDebug() << "SQL执行失败：" << sql << "错误：" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQuery SqliteHelper::executeQuery(const QString& sql) {
    QSqlQuery query(m_db);
    query.exec(sql);
    return query;
}

QSqlDatabase SqliteHelper::getDatabase() {
    return m_db;
}
