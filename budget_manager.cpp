#include "budget_manager.h"
#include <QSqlQuery>
#include <QDateTime>
#include <QDebug>

BudgetManager* BudgetManager::m_instance = nullptr;
QMutex BudgetManager::m_mutex;

BudgetManager::BudgetManager() {
    m_dbHelper = SqliteHelper::getInstance();
}

BudgetManager* BudgetManager::getInstance() {
    if (m_instance == nullptr) {
        m_mutex.lock();
        if (m_instance == nullptr) {
            m_instance = new BudgetManager();
        }
        m_mutex.unlock();
    }
    return m_instance;
}

bool BudgetManager::setBudget(int userId, double daily, double monthly, double yearly) {
    QString sql = QString(R"(
        INSERT INTO budget (user_id, daily_budget, monthly_budget, yearly_budget)
        VALUES (%1, %2, %3, %4)
        ON CONFLICT(user_id) DO UPDATE SET
        daily_budget = excluded.daily_budget,
        monthly_budget = excluded.monthly_budget,
        yearly_budget = excluded.yearly_budget
    )").arg(userId).arg(daily).arg(monthly).arg(yearly);
    
    return m_dbHelper->executeSql(sql);
}

BudgetInfo BudgetManager::getBudget(int userId) {
    BudgetInfo info;
    QString sql = QString("SELECT daily_budget, monthly_budget, yearly_budget FROM budget WHERE user_id = %1").arg(userId);
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (query.next()) {
        info.daily = query.value(0).toDouble();
        info.monthly = query.value(1).toDouble();
        info.yearly = query.value(2).toDouble();
    }
    return info;
}

QString BudgetManager::checkBudgetExceeded(int userId, double newAmount, const QDateTime& checkDate) {
    if (newAmount >= 0) return ""; // 收入不触发预算检查

    BudgetInfo budget = getBudget(userId);
    if (budget.daily <= 0 && budget.monthly <= 0 && budget.yearly <= 0) return "";

    double absNewAmount = qAbs(newAmount);

    // 1. 检查日预算
    if (budget.daily > 0) {
        QString dateStr = checkDate.toString("yyyy-MM-dd");
        QString sql = QString(R"(
            SELECT SUM(ABS(amount)) FROM account_record 
            WHERE user_id = %1 AND amount < 0 AND is_deleted = 0 
            AND create_time LIKE '%2%'
        )").arg(userId).arg(dateStr);
        QSqlQuery query = m_dbHelper->executeQuery(sql);
        double currentDaily = 0;
        if (query.next()) currentDaily = query.value(0).toDouble();
        
        if (currentDaily + absNewAmount > budget.daily) {
            return QString("【%1】日预算超额！\n该日已支出: ¥%2\n当前记账: ¥%3\n日预算限额: ¥%4")
                .arg(dateStr).arg(currentDaily, 0, 'f', 2).arg(absNewAmount, 0, 'f', 2).arg(budget.daily, 0, 'f', 2);
        }
    }

    // 2. 检查月预算
    if (budget.monthly > 0) {
        QString monthStr = checkDate.toString("yyyy-MM");
        QString sql = QString(R"(
            SELECT SUM(ABS(amount)) FROM account_record 
            WHERE user_id = %1 AND amount < 0 AND is_deleted = 0 
            AND create_time LIKE '%2%'
        )").arg(userId).arg(monthStr);
        QSqlQuery query = m_dbHelper->executeQuery(sql);
        double currentMonthly = 0;
        if (query.next()) currentMonthly = query.value(0).toDouble();
        
        if (currentMonthly + absNewAmount > budget.monthly) {
            return QString("【%1】月预算超额！\n该月已支出: ¥%2\n当前记账: ¥%3\n月预算限额: ¥%4")
                .arg(monthStr).arg(currentMonthly, 0, 'f', 2).arg(absNewAmount, 0, 'f', 2).arg(budget.monthly, 0, 'f', 2);
        }
    }

    // 3. 检查年预算
    if (budget.yearly > 0) {
        QString yearStr = checkDate.toString("yyyy");
        QString sql = QString(R"(
            SELECT SUM(ABS(amount)) FROM account_record 
            WHERE user_id = %1 AND amount < 0 AND is_deleted = 0 
            AND create_time LIKE '%2%'
        )").arg(userId).arg(yearStr);
        QSqlQuery query = m_dbHelper->executeQuery(sql);
        double currentYearly = 0;
        if (query.next()) currentYearly = query.value(0).toDouble();
        
        if (currentYearly + absNewAmount > budget.yearly) {
            return QString("【%1】年预算超额！\n该年已支出: ¥%2\n当前记账: ¥%3\n年预算限额: ¥%4")
                .arg(yearStr).arg(currentYearly, 0, 'f', 2).arg(absNewAmount, 0, 'f', 2).arg(budget.yearly, 0, 'f', 2);
        }
    }

    return "";
}
