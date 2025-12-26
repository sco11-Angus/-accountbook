#ifndef DB_MODELS_H
#define DB_MODELS_H

#include <QString>
#include <QDateTime>
#include <QList>

/**
 * @file db_models.h
 * @brief 数据库数据模型定义
 * @details 定义账本系统中使用的所有数据结构体
 */

// ==================== 账户相关数据结构 ====================

/**
 * @struct UserData
 * @brief 用户数据结构
 */
struct UserData {
    int id = 0;                        // 用户ID（主键）
    QString account;                   // 账户（手机号/邮箱），唯一
    QString password;                  // 加密后的密码
    QString nickname = "默认昵称";      // 昵称
    QString avatar;                    // 头像路径
    int gender = 0;                    // 性别（0=未知，1=男，2=女）
    QString payMethod;                 // 常用支付方式
    int loginFailCount = 0;            // 登录失败次数
    QString lockTime;                  // 账户锁定时间
    QString createTime;                // 创建时间
    QString updateTime;                // 更新时间
    int isDeleted = 0;                 // 是否删除（0=否，1=是）
    
    // 用于同步状态追踪
    int syncStatus = 0;                // 同步状态（0=已同步，1=待同步，2=同步失败）
    QString lastSyncTime;              // 最后同步时间
};

// ==================== 账本相关数据结构 ====================

/**
 * @struct AccountBookData
 * @brief 账本（账户分类）数据结构
 */
struct AccountBookData {
    int id = 0;                        // 账本ID（主键）
    int userId = 0;                    // 所属用户ID（外键）
    QString name;                      // 账本名称
    QString description;               // 账本描述
    QString icon;                      // 账本图标/颜色
    int sortOrder = 0;                 // 排序顺序
    QString createTime;                // 创建时间
    QString updateTime;                // 更新时间
    int isDeleted = 0;                 // 是否删除
    
    // 同步状态
    int syncStatus = 0;                // 0=已同步，1=待同步，2=同步失败
    QString lastSyncTime;
};

/**
 * @struct BillCategoryData
 * @brief 账单分类数据结构
 */
struct BillCategoryData {
    int id = 0;                        // 分类ID
    int userId = 0;                    // 所属用户ID
    QString name;                      // 分类名称（如"餐饮"、"交通"）
    int type = 0;                      // 分类类型（0=支出，1=收入）
    QString icon;                      // 分类图标
    QString color;                     // 分类颜色
    int sortOrder = 0;                 // 排序
    QString createTime;
    QString updateTime;
    int isDeleted = 0;
    
    int syncStatus = 0;
    QString lastSyncTime;
};

// ==================== 账单相关数据结构 ====================

/**
 * @struct BillData
 * @brief 账单数据结构（核心实体）
 */
struct BillData {
    int id = 0;                        // 账单ID（主键）
    int userId = 0;                    // 所属用户ID（外键）
    int bookId = 0;                    // 所属账本ID（外键）
    int categoryId = 0;                // 分类ID（外键）
    
    // 基本信息
    QString date;                      // 账单日期（YYYY-MM-DD HH:mm:ss）
    double amount = 0.0;               // 金额
    int type = 0;                      // 类型（0=支出，1=收入）
    
    // 详细信息
    QString description;               // 账单描述/备注
    QString paymentMethod;             // 支付方式（现金、支付宝、微信等）
    QString merchant;                  // 商家名称
    QString tag;                       // 标签（逗号分隔）
    
    // 媒体数据
    QString voucherPath;               // 凭证图片路径（本地或服务器URL）
    QString voucherUrl;                // 凭证图片URL（服务端）
    
    // 状态信息
    int isDeleted = 0;                 // 是否删除（0=否，1=是）
    QString deleteTime;                // 删除时间
    QString createTime;                // 创建时间
    QString updateTime;                // 最后修改时间
    
    // 同步状态追踪（用于本地-服务端同步）
    int syncStatus = 0;                // 0=已同步，1=待同步，2=同步失败
    int localId = 0;                   // 本地临时ID（云端同步未成功时使用）
    QString lastSyncTime;              // 最后同步时间
    QString syncErrorMsg;              // 同步失败消息
};

/**
 * @struct BillQueryResult
 * @brief 账单查询结果（用于统计分析）
 */
struct BillQueryResult {
    int totalCount = 0;                // 查询结果总数
    double totalIncome = 0.0;          // 总收入
    double totalExpense = 0.0;         // 总支出
    double netAmount = 0.0;            // 净金额（收入-支出）
    QList<BillData> bills;             // 账单列表
    
    // 分类统计
    QMap<QString, double> categoryStats;  // 分类统计（分类名->金额）
    QMap<QString, double> methodStats;    // 支付方式统计
};

// ==================== 同步相关数据结构 ====================

/**
 * @struct SyncQueueItem
 * @brief 同步队列项（用于离线模式下存储待同步操作）
 */
struct SyncQueueItem {
    int id = 0;
    int userId = 0;
    int entityId = 0;                  // 对应的实体ID（BillData.id等）
    QString entityType;                // 实体类型（"bill"、"category"、"book"等）
    QString operation;                 // 操作类型（"insert"、"update"、"delete"）
    QString payload;                   // JSON格式的数据负载
    QString createTime;
    int status = 0;                    // 0=待同步，1=同步中，2=已同步，3=同步失败
    int retryCount = 0;                // 重试次数
    QString errorMsg;
};

/**
 * @struct SyncStatistics
 * @brief 同步统计信息
 */
struct SyncStatistics {
    int pendingCount = 0;              // 待同步项数
    int successCount = 0;              // 成功同步数
    int failureCount = 0;              // 失败同步数
    QString lastSyncTime;              // 最后同步时间
    bool isSyncing = false;            // 是否正在同步
};

// ==================== 分页相关数据结构 ====================

/**
 * @struct PaginationInfo
 * @brief 分页信息
 */
struct PaginationInfo {
    int pageNumber = 1;                // 页码（从1开始）
    int pageSize = 20;                 // 每页条数
    int totalCount = 0;                // 总条数
    int totalPages = 0;                // 总页数
    
    int getOffset() const {
        return (pageNumber - 1) * pageSize;
    }
};

#endif // DB_MODELS_H
