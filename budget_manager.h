#ifndef BUDGET_MANAGER_H
#define BUDGET_MANAGER_H

#include <QObject>
#include <QMutex>
#include "sqlite_helper.h"

struct BudgetInfo {
    double daily = 0;
    double monthly = 0;
    double yearly = 0;
};

class BudgetManager : public QObject {
    Q_OBJECT
public:
    static BudgetManager* getInstance();
    
    // 设置预算
    bool setBudget(int userId, double daily, double monthly, double yearly);
    // 获取预算
    BudgetInfo getBudget(int userId);
    
    // 检查预算是否超额
    // 返回超额的类型描述，如果不超额则返回空字符串
    QString checkBudgetExceeded(int userId, double currentAmount);

private:
    BudgetManager();
    static BudgetManager* m_instance;
    static QMutex m_mutex;
    
    SqliteHelper* m_dbHelper;
};

#endif // BUDGET_MANAGER_H
