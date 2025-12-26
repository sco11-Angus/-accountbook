#ifndef DATABASE_EXAMPLE_H
#define DATABASE_EXAMPLE_H

#include "db_manager.h"
#include <QString>
#include <QList>

/**
 * @file database_example.h
 * @brief 数据库使用示例和集成建议
 * @details 展示DBManager在实际项目中的使用方式
 */

class DatabaseExample {
public:
    /**
     * @brief 完整流程示例：创建用户并添加账单
     */
    static void exampleCreateUserAndBills();
    
    /**
     * @brief 查询示例：获取月度统计
     */
    static void exampleMonthlyStatistics();
    
    /**
     * @brief 分类管理示例
     */
    static void exampleCategoryManagement();
    
    /**
     * @brief 账本管理示例
     */
    static void exampleAccountBookManagement();
    
    /**
     * @brief 日期范围查询示例
     */
    static void exampleDateRangeQuery();
    
    /**
     * @brief 删除与恢复示例
     */
    static void exampleDeleteAndRecovery();
    
    /**
     * @brief 同步队列示例
     */
    static void exampleSyncQueue();
    
    /**
     * @brief 错误处理示例
     */
    static void exampleErrorHandling();
};

#endif // DATABASE_EXAMPLE_H
