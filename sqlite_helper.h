#ifndef SQLITE_HELPER_H
#define SQLITE_HELPER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QString>

// 单例模式封装SQLite操作
class SqliteHelper {
public:
    // 获取单例实例
    static SqliteHelper* getInstance();
    ~SqliteHelper();

    // 打开数据库
    bool openDatabase(const QString& dbPath = "./account_book.db");
    // 关闭数据库
    void closeDatabase();
    // 执行SQL语句（增删改）
    bool executeSql(const QString& sql);
    // 执行查询SQL
    QSqlQuery executeQuery(const QString& sql);
    // 获取数据库实例
    QSqlDatabase getDatabase();

private:
    SqliteHelper(); // 私有构造
    SqliteHelper(const SqliteHelper&) = delete;
    SqliteHelper& operator=(const SqliteHelper&) = delete;

    static SqliteHelper* m_instance;
    static QMutex m_mutex;
    QSqlDatabase m_db;
};

#endif // SQLITE_HELPER_H
