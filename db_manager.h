#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <QString>
#include <QList>
#include <QMutex>
#include "db_models.h"

class SqliteHelper;

/**
 * @file db_manager.h
 * @brief 数据库管理接口（核心业务层）
 * @details 统一的数据库访问接口，负责：
 *   - 本地SQLite和服务端SQLite的双库管理
 *   - 增删改查账单、用户、账本的标准接口
 *   - 本地缓存和服务端同步的协调
 */

class DBManager {
public:
    // ============ 单例管理 ============
    static DBManager* getInstance();

    // ============ 初始化 ============
    /**
     * @brief 初始化数据库系统
     * @param localDbPath SQLite本地数据库路径
     * @return 是否初始化成功
     */
    bool initialize(const QString& localDbPath = "./account_book.db");

    /**
     * @brief 连接到远程数据库 (SQLite模式下仅做占位)
     */
    bool connectRemoteDatabase(const QString& host = "", int port = 0,
                               const QString& username = "", const QString& password = "",
                               const QString& database = "");

    /**
     * @brief 断开远程数据库连接
     */
    void disconnectRemoteDatabase();

    /**
     * @brief 检查远程数据库是否连接
     */
    bool isRemoteConnected() const;

    // ============ 账单管理 (Bill) ============

    /**
     * @brief 新增账单（同时写入本地库和待同步队列）
     * @param bill 账单数据
     * @return 生成的账单ID（本地ID）
     */
    int addBill(const BillData& bill);

    /**
     * @brief 获取指定月份的账单
     * @param userId 用户ID
     * @param year 年份
     * @param month 月份（1-12）
     * @return 账单列表
     */
    QList<BillData> getBillByMonth(int userId, int year, int month);

    /**
     * @brief 获取指定日期范围的账单
     * @param userId 用户ID
     * @param startDate 开始日期（YYYY-MM-DD）
     * @param endDate 结束日期（YYYY-MM-DD）
     * @return 账单列表
     */
    QList<BillData> getBillByDateRange(int userId, const QString& startDate, const QString& endDate);

    /**
     * @brief 按分类获取账单统计
     * @param userId 用户ID
     * @param year 年份
     * @param month 月份
     * @return 分类统计结果
     */
    BillQueryResult getBillByCategoryStats(int userId, int year, int month);

    /**
     * @brief 获取单个账单详情
     */
    BillData getBillById(int billId);

    /**
     * @brief 更新账单
     */
    bool updateBill(const BillData& bill);

    /**
     * @brief 删除账单（软删除）
     */
    bool deleteBill(int billId);

    /**
     * @brief 永久删除账单（硬删除）
     */
    bool permanentlyDeleteBill(int billId);

    /**
     * @brief 获取账单总数
     */
    int getBillCount(int userId, int year = 0, int month = 0);

    // ============ 账本管理 (AccountBook) ============

    /**
     * @brief 新增账本
     */
    int addAccountBook(const AccountBookData& book);

    /**
     * @brief 获取用户的所有账本
     */
    QList<AccountBookData> getAccountBooks(int userId);

    /**
     * @brief 获取单个账本
     */
    AccountBookData getAccountBookById(int bookId);

    /**
     * @brief 更新账本
     */
    bool updateAccountBook(const AccountBookData& book);

    /**
     * @brief 删除账本
     */
    bool deleteAccountBook(int bookId);

    // ============ 账单分类管理 (BillCategory) ============

    /**
     * @brief 新增分类
     */
    int addBillCategory(const BillCategoryData& category);

    /**
     * @brief 获取用户的所有分类
     */
    QList<BillCategoryData> getBillCategories(int userId, int type = -1);

    /**
     * @brief 获取单个分类
     */
    BillCategoryData getBillCategoryById(int categoryId);

    /**
     * @brief 更新分类
     */
    bool updateBillCategory(const BillCategoryData& category);

    /**
     * @brief 删除分类
     */
    bool deleteBillCategory(int categoryId);

    // ============ 用户管理 (User) ============

    /**
     * @brief 新增用户
     */
    int addUser(const UserData& user);

    /**
     * @brief 获取用户信息
     */
    UserData getUserById(int userId);

    /**
     * @brief 按账户查询用户
     */
    UserData getUserByAccount(const QString& account);

    /**
     * @brief 更新用户信息
     */
    bool updateUser(const UserData& user);

    /**
     * @brief 删除用户
     */
    bool deleteUser(int userId);

    // ============ 本地数据缓存 ============

    /**
     * @brief 将服务端账单数据同步到本地
     */
    bool syncBillsFromRemote(int userId, const QList<BillData>& bills);

    /**
     * @brief 从本地导出待同步的账单
     */
    QList<BillData> getUnsyncedBills(int userId);

    /**
     * @brief 标记账单为已同步
     */
    bool markBillAsSynced(int billId, int remoteBillId = 0);

    /**
     * @brief 标记账单同步失败
     */
    bool markBillSyncFailed(int billId, const QString& errorMsg);

    // ============ 同步队列管理 ============

    /**
     * @brief 获取待同步项列表
     */
    QList<SyncQueueItem> getPendingSyncItems(int userId, int limit = 100);

    /**
     * @brief 添加待同步项到队列
     */
    bool addToSyncQueue(const SyncQueueItem& item);

    /**
     * @brief 移除同步队列项
     */
    bool removeSyncQueueItem(int itemId);

    /**
     * @brief 获取同步统计信息
     */
    SyncStatistics getSyncStatistics(int userId);

    // ============ 错误处理 ============

    /**
     * @brief 获取最后的错误信息
     */
    QString getLastError() const { return m_lastError; }

private:
    static DBManager* m_instance;
    static QMutex m_mutex;

    SqliteHelper* m_localDb = nullptr;      // 本地数据库
    SqliteHelper* m_remoteDb = nullptr;      // SQLite模式下可能指向同一个或另一个SQLite实例
    QString m_lastError;
    bool m_isInitialized = false;

    DBManager();
    ~DBManager();

    // 辅助方法
    void setError(const QString& error);
};

#endif // DB_MANAGER_H
