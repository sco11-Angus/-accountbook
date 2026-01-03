#include "statistics_manager.h"
#include <algorithm>

StatisticsManager* StatisticsManager::m_instance = nullptr;

StatisticsManager* StatisticsManager::getInstance() {
    if (!m_instance) {
        m_instance = new StatisticsManager();
    }
    return m_instance;
}

StatisticsManager::StatisticsManager(QObject *parent) : QObject(parent) {}

MonthlyStat StatisticsManager::getMonthlyStat(int userId, int year, int month) {
    MonthlyStat stat;
    stat.totalIncome = 0;
    stat.totalExpense = 0;
    stat.balance = 0;

    QList<AccountRecord> records = m_accountManager.queryMonthlyRecords(userId, year, month);
    
    QMap<QString, double> expenseMap;
    QMap<QString, double> incomeMap;

    for (const auto& record : records) {
        double amount = record.getAmount();
        if (amount < 0) {
            stat.totalExpense += qAbs(amount);
            expenseMap[record.getType()] += qAbs(amount);
        } else {
            stat.totalIncome += amount;
            incomeMap[record.getType()] += amount;
        }
    }

    stat.balance = stat.totalIncome - stat.totalExpense;

    // Process Daily Stats
    int daysInMonth = QDate(year, month, 1).daysInMonth();
    QMap<int, DailyStat> dailyMap;
    for (int i = 1; i <= daysInMonth; ++i) {
        dailyMap[i] = {i, 0, 0};
    }

    for (const auto& record : records) {
        QDateTime dt = QDateTime::fromString(record.getCreateTime(), "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) {
            dt = QDateTime::fromString(record.getCreateTime(), "yyyy-MM-dd");
        }
        if (dt.isValid()) {
            int day = dt.date().day();
            double amount = record.getAmount();
            if (amount < 0) {
                dailyMap[day].expense += qAbs(amount);
            } else {
                dailyMap[day].income += amount;
            }
        }
    }
    for (int i = 1; i <= daysInMonth; ++i) {
        stat.dailyStats.append(dailyMap[i]);
    }

    // Process Expense Stats
    for (auto it = expenseMap.begin(); it != expenseMap.end(); ++it) {
        CategoryStat cs;
        cs.category = it.key();
        cs.amount = it.value();
        cs.percentage = (stat.totalExpense > 0) ? (cs.amount / stat.totalExpense * 100) : 0;
        cs.color = getCategoryColor(cs.category);
        stat.expenseStats.append(cs);
    }

    // Process Income Stats
    for (auto it = incomeMap.begin(); it != incomeMap.end(); ++it) {
        CategoryStat cs;
        cs.category = it.key();
        cs.amount = it.value();
        cs.percentage = (stat.totalIncome > 0) ? (cs.amount / stat.totalIncome * 100) : 0;
        cs.color = getCategoryColor(cs.category);
        stat.incomeStats.append(cs);
    }

    // Sort by amount descending
    auto sortFunc = [](const CategoryStat& a, const CategoryStat& b) {
        return a.amount > b.amount;
    };
    std::sort(stat.expenseStats.begin(), stat.expenseStats.end(), sortFunc);
    std::sort(stat.incomeStats.begin(), stat.incomeStats.end(), sortFunc);

    return stat;
}

QString StatisticsManager::getCategoryColor(const QString& category) {
    static QMap<QString, QString> colors = {
        {"餐饮", "#FF6B6B"}, {"服饰", "#4ECDC4"}, {"日用", "#45B7D1"},
        {"数码", "#96CEB4"}, {"美妆", "#FFEEAD"}, {"软件", "#D4A5A5"},
        {"住房", "#9A8C98"}, {"交通", "#C9ADA7"}, {"娱乐", "#F2CC8F"},
        {"医疗", "#E07A5F"}, {"通讯", "#3D405B"}, {"汽车", "#81B29A"},
        {"学习", "#F4F1DE"}, {"办公", "#A8DADC"}, {"运动", "#457B9D"},
        {"社交", "#1D3557"}, {"宠物", "#E63946"}, {"旅行", "#A2D2FF"},
        {"育儿", "#BDE0FE"}, {"其他", "#FFAFCC"},
        {"工资", "#4CAF50"}, {"兼职", "#8BC34A"}, {"投资", "#CDDC39"},
        {"副业", "#FFEB3B"}, {"红包", "#FFC107"}, {"意外收入", "#FF9800"}
    };
    return colors.value(category, "#CCCCCC");
}
