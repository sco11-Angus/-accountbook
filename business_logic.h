#ifndef BUSINESS_LOGIC_H
#define BUSINESS_LOGIC_H

#include <QObject>
#include <QList>
#include <QDate>
#include "account_record.h"
#include "user.h"

class BusinessLogic : public QObject
{
    Q_OBJECT
public:
    explicit BusinessLogic(QObject *parent = nullptr);

    // 收支计算
    double calculateBalance(const QList<AccountRecord>& records);
    double calculateIncome(const QList<AccountRecord>& records);
    double calculateExpense(const QList<AccountRecord>& records);

    // 月结余统计
    double calculateMonthlyBalance(int year, int month, const QList<AccountRecord>& records);
    QMap<QString, double> calculateMonthlyCategorySummary(int year, int month, const QList<AccountRecord>& records);

    // 分类校验
    bool isValidCategory(const QString& category, bool isExpense);
    QStringList getValidCategories(bool isExpense);
    bool addCustomCategory(const QString& category, bool isExpense);

    // 账单格式校验
    bool validateBillRecord(const AccountRecord& record);
    QString getValidationError();

    // 日期工具函数
    bool isSameMonth(const QString& dateString, int year, int month);
    QDate stringToDate(const QString& dateString);

    // 异常消费检测
    bool checkAbnormalConsumption(const QMap<QString, double>& currentMonth,
                                  const QMap<QString, double>& lastMonth);

signals:

private:
    QString m_lastError;
    QStringList m_expenseCategories;
    QStringList m_incomeCategories;
    void initCategories();
};

#endif // BUSINESS_LOGIC_H
