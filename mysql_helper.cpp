#include "mysql_helper.h"
#include <QDebug>
#include <QSqlRecord>

// 静态成员变量初始化
MySqlHelper* MySqlHelper::m_instance = nullptr;
QMutex MySqlHelper::m_mutex;

MySqlHelper::MySqlHelper() {}

MySqlHelper::~MySqlHelper() {
    disconnect();
}

MySqlHelper* MySqlHelper::getInstance() {
    if (m_instance == nullptr) {
        m_mutex.lock();
        if (m_instance == nullptr) {
            m_instance = new MySqlHelper();
        }
        m_mutex.unlock();
    }
    return m_instance;
}

bool MySqlHelper::connect(const QString& host, int port, const QString& username,
                         const QString& password, const QString& database) {
    // 检查是否已存在连接
    if (QSqlDatabase::contains("mysql_connection")) {
        m_db = QSqlDatabase::database("mysql_connection");
        if (m_db.isOpen()) {
            return true;
        }
    }

    // 创建新的MySQL连接
    m_db = QSqlDatabase::addDatabase("QMYSQL", "mysql_connection");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setUserName(username);
    m_db.setPassword(password);
    m_db.setDatabaseName("");

    if (!m_db.open()) {
        m_lastError = "MySQL服务器连接失败: " + m_db.lastError().text();
        qDebug() << m_lastError;
        m_isConnected = false;
        return false;
    }

    //检查并创建数据库
    if(!creatdatebase(database)){
        m_db.close();
        m_isConnected = false;
        return false;
    }

    // 设置数据库名称并重新连接以选中该数据库
    m_db.setDatabaseName(database);
    if (!m_db.open()) {
        m_lastError = "选中数据库失败: " + m_db.lastError().text();
        qDebug() << m_lastError;
        m_isConnected = false;
        return false;
    }

    m_isConnected = true;
    qDebug() << "MySQL连接成功: " << host << ":" << port;
    qDebug() << "已选中数据库: " << database;
    
    // 初始化表结构
    if (!initializeTables()) {
        qWarning() << "数据库表初始化失败";
        return false;
    }

    return true;
}

void MySqlHelper::disconnect() {
    if (m_db.isOpen()) {
        m_db.close();
        m_isConnected = false;
    }
}

bool MySqlHelper::isConnected() const {
    return m_isConnected && m_db.isOpen();
}

bool MySqlHelper::testConnection() {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT 1")) {
        m_lastError = "连接测试失败: " + query.lastError().text();
        return false;
    }
    return true;
}

bool MySqlHelper::initializeTables() {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }

    qDebug() << "开始初始化MySQL表结构...";

    if (!createUserTable()) return false;
    if (!createAccountBookTable()) return false;
    if (!createBillCategoryTable()) return false;
    if (!createBillTable()) return false;
    if (!createSyncQueueTable()) return false;
    if (!createIndexes()) return false;

    qDebug() << "MySQL表结构初始化完成";
    return true;
}

bool MySqlHelper::createUserTable() {
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INT PRIMARY KEY AUTO_INCREMENT COMMENT '用户ID',
            account VARCHAR(255) UNIQUE NOT NULL COMMENT '账户（邮箱/手机号）',
            password VARCHAR(255) NOT NULL COMMENT '加密后的密码',
            nickname VARCHAR(100) DEFAULT '默认昵称' COMMENT '昵称',
            avatar LONGTEXT DEFAULT NULL COMMENT '头像路径',
            gender INT DEFAULT 0 COMMENT '性别（0=未知，1=男，2=女）',
            pay_method VARCHAR(255) DEFAULT '' COMMENT '常用支付方式',
            login_fail_count INT DEFAULT 0 COMMENT '登录失败次数',
            lock_time DATETIME DEFAULT NULL COMMENT '账户锁定时间',
            create_time DATETIME NOT NULL COMMENT '创建时间',
            update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
            is_deleted INT DEFAULT 0 COMMENT '是否删除',
            INDEX idx_account (account),
            INDEX idx_create_time (create_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表'
    )";

    return executeSql(sql);
}

bool MySqlHelper::createAccountBookTable() {
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS account_book (
            id INT PRIMARY KEY AUTO_INCREMENT COMMENT '账本ID',
            user_id INT NOT NULL COMMENT '所属用户ID',
            name VARCHAR(100) NOT NULL COMMENT '账本名称',
            description TEXT DEFAULT NULL COMMENT '账本描述',
            icon VARCHAR(50) DEFAULT NULL COMMENT '账本图标/颜色',
            sort_order INT DEFAULT 0 COMMENT '排序顺序',
            create_time DATETIME NOT NULL COMMENT '创建时间',
            update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
            is_deleted INT DEFAULT 0 COMMENT '是否删除',
            sync_status INT DEFAULT 0 COMMENT '同步状态（0=已同步，1=待同步，2=同步失败）',
            last_sync_time DATETIME DEFAULT NULL COMMENT '最后同步时间',
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            INDEX idx_user_id (user_id),
            INDEX idx_create_time (create_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='账本表'
    )";

    return executeSql(sql);
}

bool MySqlHelper::createBillCategoryTable() {
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS bill_category (
            id INT PRIMARY KEY AUTO_INCREMENT COMMENT '分类ID',
            user_id INT NOT NULL COMMENT '所属用户ID',
            name VARCHAR(100) NOT NULL COMMENT '分类名称',
            type INT NOT NULL COMMENT '分类类型（0=支出，1=收入）',
            icon VARCHAR(50) DEFAULT NULL COMMENT '分类图标',
            color VARCHAR(20) DEFAULT NULL COMMENT '分类颜色',
            sort_order INT DEFAULT 0 COMMENT '排序',
            create_time DATETIME NOT NULL COMMENT '创建时间',
            update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
            is_deleted INT DEFAULT 0 COMMENT '是否删除',
            sync_status INT DEFAULT 0 COMMENT '同步状态',
            last_sync_time DATETIME DEFAULT NULL COMMENT '最后同步时间',
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            INDEX idx_user_id (user_id),
            INDEX idx_type (type),
            UNIQUE KEY uk_user_category (user_id, name)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='账单分类表'
    )";

    return executeSql(sql);
}

bool MySqlHelper::createBillTable() {
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS bill (
            id INT PRIMARY KEY AUTO_INCREMENT COMMENT '账单ID',
            user_id INT NOT NULL COMMENT '所属用户ID',
            book_id INT NOT NULL COMMENT '所属账本ID',
            category_id INT NOT NULL COMMENT '分类ID',
            bill_date DATETIME NOT NULL COMMENT '账单日期',
            amount DECIMAL(12, 2) NOT NULL COMMENT '金额',
            type INT NOT NULL COMMENT '类型（0=支出，1=收入）',
            description TEXT DEFAULT NULL COMMENT '账单描述/备注',
            payment_method VARCHAR(50) DEFAULT NULL COMMENT '支付方式',
            merchant VARCHAR(100) DEFAULT NULL COMMENT '商家名称',
            tag VARCHAR(255) DEFAULT NULL COMMENT '标签（逗号分隔）',
            voucher_path VARCHAR(500) DEFAULT NULL COMMENT '凭证图片本地路径',
            voucher_url VARCHAR(500) DEFAULT NULL COMMENT '凭证图片服务器URL',
            is_deleted INT DEFAULT 0 COMMENT '是否删除',
            delete_time DATETIME DEFAULT NULL COMMENT '删除时间',
            create_time DATETIME NOT NULL COMMENT '创建时间',
            update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后修改时间',
            sync_status INT DEFAULT 0 COMMENT '同步状态（0=已同步，1=待同步，2=同步失败）',
            local_id INT DEFAULT 0 COMMENT '本地临时ID',
            last_sync_time DATETIME DEFAULT NULL COMMENT '最后同步时间',
            sync_error_msg TEXT DEFAULT NULL COMMENT '同步失败消息',
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            FOREIGN KEY (book_id) REFERENCES account_book(id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES bill_category(id) ON DELETE RESTRICT,
            INDEX idx_user_id (user_id),
            INDEX idx_book_id (book_id),
            INDEX idx_category_id (category_id),
            INDEX idx_bill_date (bill_date),
            INDEX idx_type (type),
            INDEX idx_create_time (create_time),
            INDEX idx_sync_status (sync_status)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='账单表'
    )";

    return executeSql(sql);
}

bool MySqlHelper::createSyncQueueTable() {
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS sync_queue (
            id INT PRIMARY KEY AUTO_INCREMENT COMMENT '队列项ID',
            user_id INT NOT NULL COMMENT '用户ID',
            entity_id INT NOT NULL COMMENT '对应实体ID',
            entity_type VARCHAR(50) NOT NULL COMMENT '实体类型（bill/category/book等）',
            operation VARCHAR(20) NOT NULL COMMENT '操作类型（insert/update/delete）',
            payload LONGTEXT NOT NULL COMMENT 'JSON格式数据',
            create_time DATETIME NOT NULL COMMENT '创建时间',
            status INT DEFAULT 0 COMMENT '状态（0=待同步，1=同步中，2=已同步，3=同步失败）',
            retry_count INT DEFAULT 0 COMMENT '重试次数',
            error_msg TEXT DEFAULT NULL COMMENT '错误信息',
            FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE CASCADE,
            INDEX idx_user_id (user_id),
            INDEX idx_status (status),
            INDEX idx_create_time (create_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='同步队列表'
    )";

    return executeSql(sql);
}

bool MySqlHelper::createIndexes() {
    // 定义要创建的索引
    struct IndexInfo {
        QString tableName;
        QString indexName;
        QString columns;
    };

    QList<IndexInfo> indexes = {
        {"bill", "idx_bill_user_date", "user_id, bill_date"},
        {"bill", "idx_bill_user_type", "user_id, type"},
        {"sync_queue", "idx_sync_queue_user_status", "user_id, status"}
    };

    for (const IndexInfo& idx : indexes) {
        // 先检查索引是否存在
        QString checkSql = QString(
                               "SELECT COUNT(1) FROM INFORMATION_SCHEMA.STATISTICS "
                               "WHERE TABLE_SCHEMA = DATABASE() "
                               "AND TABLE_NAME = '%1' "
                               "AND INDEX_NAME = '%2'")
                               .arg(idx.tableName)
                               .arg(idx.indexName);

        QSqlQuery checkQuery(checkSql, m_db);
        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            qDebug() << "索引" << idx.indexName << "已存在";
            continue;
        }

        // 创建索引
        QString createSql = QString("CREATE INDEX %1 ON %2(%3)")
                                .arg(idx.indexName)
                                .arg(idx.tableName)
                                .arg(idx.columns);

        if (!executeSql(createSql)) {
            m_lastError = QString("创建索引失败: %1").arg(createSql);
            return false;
        }

        qDebug() << "索引创建成功:" << idx.indexName;
    }

    return true;
}

bool MySqlHelper::executeSql(const QString& sql) {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = "SQL执行失败: " + sql + " | 错误: " + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

bool MySqlHelper::executeSqlWithParams(const QString& sql, const QVariantList& params) {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    if (!query.exec()) {
        m_lastError = "参数化SQL执行失败: " + sql + " | 错误: " + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    return true;
}

QSqlQuery MySqlHelper::executeQuery(const QString& sql) {
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = "SQL查询失败: " + sql + " | 错误: " + query.lastError().text();
        qDebug() << m_lastError;
    }
    return query;
}

QSqlQuery MySqlHelper::executeQueryWithParams(const QString& sql, const QVariantList& params) {
    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    if (!query.exec()) {
        m_lastError = "参数化SQL查询失败: " + sql + " | 错误: " + query.lastError().text();
        qDebug() << m_lastError;
    }
    return query;
}

bool MySqlHelper::beginTransaction() {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }
    return m_db.transaction();
}

bool MySqlHelper::commitTransaction() {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }
    return m_db.commit();
}

bool MySqlHelper::rollbackTransaction() {
    if (!isConnected()) {
        m_lastError = "数据库未连接";
        return false;
    }
    return m_db.rollback();
}


bool MySqlHelper::creatdatebase(const QString& database){
    QSqlQuery query(m_db);

    // 1. 检查数据库是否存在
    QString checkDbSql = QString("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = '%1'").arg(database);
    if (!query.exec(checkDbSql)) {
        m_lastError = "检查数据库失败: " + query.lastError().text();
        qDebug() << m_lastError;
        m_isConnected = false;
        return false;
    }

    // 2. 如果数据库不存在，创建它
    if (!query.next()) {
        QString createDbSql = QString("CREATE DATABASE IF NOT EXISTS %1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci").arg(database);
        if (!query.exec(createDbSql)) {
            m_lastError = "创建数据库失败: " + query.lastError().text();
            qDebug() << m_lastError;
            m_isConnected = false;
            return false;
        }
        qDebug() << "数据库创建成功:" << database;
    } else {
        qDebug() << "数据库已存在:" << database;
    }
    return true;
}
