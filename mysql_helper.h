#ifndef MYSQL_HELPER_H
#define MYSQL_HELPER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantList>
#include <QMutex>
#include "db_models.h"

/**
 * @file mysql_helper.h
 * @brief MySQL服务端数据库助手
 * @details 负责与服务端MySQL数据库的所有交互操作
 */

class MySqlHelper {
public:
    // ============ 单例管理 ============
    static MySqlHelper* getInstance();
    ~MySqlHelper();

    // ============ 连接管理 ============
    /**
     * @brief 连接到MySQL数据库
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param username 用户名
     * @param password 密码
     * @param database 数据库名
     * @return 是否连接成功
     */
    bool connect(const QString& host, int port, const QString& username, 
                 const QString& password, const QString& database);
    
    /**
     * @brief 断开连接
     */
    void disconnect();
    
    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 测试连接
     */
    bool testConnection();

    // ============ 数据库初始化 ============
    /**
     * @brief 初始化数据库表结构
     */
    bool initializeTables();

    // ============ SQL执行 ============
    /**
     * @brief 执行SQL语句（增删改）
     */
    bool executeSql(const QString& sql);
    
    /**
     * @brief 执行参数化SQL语句（防止SQL注入）
     */
    bool executeSqlWithParams(const QString& sql, const QVariantList& params);
    
    /**
     * @brief 执行查询
     */
    QSqlQuery executeQuery(const QString& sql);
    
    /**
     * @brief 执行参数化查询
     */
    QSqlQuery executeQueryWithParams(const QString& sql, const QVariantList& params);

    // ============ 错误处理 ============
    /**
     * @brief 获取最后的错误信息
     */
    QString getLastError() const { return m_lastError; }
    
    /**
     * @brief 清空错误信息
     */
    void clearError() { m_lastError = ""; }

    // ============ 事务管理 ============
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    // 单例相关
    static MySqlHelper* m_instance;
    static QMutex m_mutex;
    
    // 数据库相关
    QSqlDatabase m_db;
    QString m_lastError;
    bool m_isConnected = false;
    
    // 私有构造函数
    MySqlHelper();
    
    // 创建表的辅助方法
    bool createUserTable();
    bool createAccountBookTable();
    bool createBillCategoryTable();
    bool createBillTable();
    bool createSyncQueueTable();
    bool createIndexes();

    bool creatdatebase(const QString& database);
};

#endif // MYSQL_HELPER_H
