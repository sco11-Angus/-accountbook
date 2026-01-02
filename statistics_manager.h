#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDate>
#include "account_record.h"
#include "account_manager.h"

struct CategoryStat {
    QString category;
    double amount;
    double percentage;
    QString color;
};

struct MonthlyStat {
    double totalIncome;
    double totalExpense;
    double balance;
    QList<CategoryStat> expenseStats;
    QList<CategoryStat> incomeStats;
};

class StatisticsManager : public QObject
{
    Q_OBJECT
public:
    static StatisticsManager* getInstance();
    
    MonthlyStat getMonthlyStat(int userId, int year, int month);

private:
    explicit StatisticsManager(QObject *parent = nullptr);
    static StatisticsManager* m_instance;
    AccountManager m_accountManager;
    
    QString getCategoryColor(const QString& category);
};

#endif // STATISTICS_MANAGER_H
