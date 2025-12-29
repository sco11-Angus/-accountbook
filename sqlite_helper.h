#ifndef SQLITE_HELPER_H
#define SQLITE_HELPER_H

#include <QSqlDatabase>//QSqlDatabase 类代表一个数据库连接，它的主要职责是处理与数据库的连接、配置和事务管理。
#include <QSqlQuery>//QSqlQuery 类用于在已经建立的 QSqlDatabase 连接上执行 SQL 语句，以及导航和检索查询结果。
#include <QSqlError>
#include <QMutex>//#include <QMutex> 是 C++ 中用于包含 Qt 框架中互斥锁（Mutex）类的头文件指令。
//在多线程编程中，QMutex 是一个非常重要的类，用于保护共享数据，防止多个线程同时访问造成数据竞争
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QStringList>

class SqliteHelper {
public:
    // ============ 单例管理 ============
    static SqliteHelper* getInstance();
    ~SqliteHelper();

    // ============ 数据库连接 ============
    // 打开数据库
    bool openDatabase(const QString& dbPath = "./account_book.db");
    // 关闭数据库
    void closeDatabase();
    // 获取数据库实例
    QSqlDatabase getDatabase();

    // ============ SQL执行（支持参数化查询防止SQL注入） ============
    // 执行SQL语句（增删改）
    bool executeSql(const QString& sql);
    // 执行参数化SQL语句（防止SQL注入）
    bool executeSqlWithParams(const QString& sql, const QVariantList& params);
    // 执行查询SQL
    QSqlQuery executeQuery(const QString& sql);
    // 执行参数化查询（防止SQL注入）
    QSqlQuery executeQueryWithParams(const QString& sql, const QVariantList& params);

    // ============ 事务管理 ============
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // ============ 数据库备份与恢复 ============
    // 创建数据库备份（带时间戳）
    bool createBackup(const QString& backupDir = "./backups");
    // 恢复数据库备份
    bool restoreBackup(const QString& backupFilePath);
    // 列出所有备份文件
    QStringList listBackups(const QString& backupDir = "./backups");
    // 删除指定备份
    bool deleteBackup(const QString& backupFilePath);

    // ============ 数据库维护与优化 ============
    // 优化数据库（VACUUM + ANALYZE）
    bool optimizeDatabase();
    // 检查数据库完整性
    bool checkIntegrity();
    // 启用外键约束
    bool enableForeignKeys();
    // 修复孤立记录
    bool fixOrphanedRecords();
    // 获取数据库统计信息
    QString getDatabaseStatistics();

    // ============ 版本管理 ============
    // 初始化版本管理
    bool initializeVersion();
    // 获取当前数据库版本
    int getCurrentVersion();
    // 设置数据库版本
    bool setVersion(int version);

    // ============ 错误处理 ============
    QString getLastError() const;
    void clearError();

private:
    // ============ 私有构造 ============
    SqliteHelper();
    SqliteHelper(const SqliteHelper&) = delete;
    SqliteHelper& operator=(const SqliteHelper&) = delete;

    // ============ 私有成员 ============
    static SqliteHelper* m_instance;
    static QMutex m_mutex;
    QSqlDatabase m_db;
    QString m_lastError;
    QString m_dbPath;

    // ============ 私有方法 ============
    // 创建数据库索引
    bool createIndexes();
    // 创建版本表
    bool createVersionTable();
};

#endif // SQLITE_HELPER_H

