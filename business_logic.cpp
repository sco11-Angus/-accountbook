#include "business_logic.h"
#include <QRegularExpression>
#include <QDateTime>

BusinessLogic::BusinessLogic(QObject *parent) : QObject(parent)
{
    initCategories();
}

void BusinessLogic::initCategories()
{
    // 初始化预设分类
    m_expenseCategories = {"餐饮", "服饰", "日用", "数码", "美妆护肤",
                           "应用软件", "住房", "交通", "娱乐", "医疗",
                           "学习", "办公", "运动", "社交", "宠物", "旅行", "其他"};

    m_incomeCategories = {"工资", "奖金", "福利", "红包", "兼职", "副业", "投资", "其他"};
}

double BusinessLogic::calculateBalance(const QList<AccountRecord>& records)
{
    return calculateIncome(records) + calculateExpense(records);
}

double BusinessLogic::calculateIncome(const QList<AccountRecord>& records)
{
    double total = 0;
    for(const auto& record : records)
    {
        if(record.getAmount() > 0)
            total += record.getAmount();
    }
    return total;
}

double BusinessLogic::calculateExpense(const QList<AccountRecord>& records)
{
    double total = 0;
    for(const auto& record : records)
    {
        if(record.getAmount() < 0)
            total += record.getAmount();
    }
    return total;
}

double BusinessLogic::calculateMonthlyBalance(int year, int month, const QList<AccountRecord>& records)
{
    double income = 0, expense = 0;

    for(const auto& record : records)
    {
        QDate date = stringToDate(record.getCreateTime());
        if(date.year() == year && date.month() == month)
        {
            if(record.getAmount() > 0)
                income += record.getAmount();
            else
                expense += record.getAmount();
        }
    }

    return income + expense;
}

QMap<QString, double> BusinessLogic::calculateMonthlyCategorySummary(int year, int month, const QList<AccountRecord>& records)
{
    QMap<QString, double> summary;

    for(const auto& record : records)
    {
        QDate date = stringToDate(record.getCreateTime());
        if(date.year() == year && date.month() == month)
        {
            summary[record.getType()] += record.getAmount();
        }
    }

    return summary;
}

bool BusinessLogic::isValidCategory(const QString& category, bool isExpense)
{
    if(isExpense)
        return m_expenseCategories.contains(category);
    else
        return m_incomeCategories.contains(category);
}

QStringList BusinessLogic::getValidCategories(bool isExpense)
{
    return isExpense ? m_expenseCategories : m_incomeCategories;
}

bool BusinessLogic::addCustomCategory(const QString& category, bool isExpense)
{
    if(category.isEmpty())
    {
        m_lastError = "分类名称不能为空";
        return false;
    }

    if(isExpense)
    {
        if(!m_expenseCategories.contains(category))
        {
            m_expenseCategories.append(category);
            return true;
        }
    }
    else
    {
        if(!m_incomeCategories.contains(category))
        {
            m_incomeCategories.append(category);
            return true;
        }
    }

    m_lastError = "分类已存在";
    return false;
}

bool BusinessLogic::validateBillRecord(const AccountRecord& record)
{
    // 验证用户ID
    if(record.getUserId() <= 0)
    {
        m_lastError = "无效的用户ID";
        return false;
    }

    // 验证金额（不能为0）
    if(record.getAmount() == 0)
    {
        m_lastError = "金额不能为0";
        return false;
    }

    // 验证分类
    bool isExpense = record.getAmount() < 0;
    if(!isValidCategory(record.getType(), isExpense))
    {
        m_lastError = "无效的收支分类";
        return false;
    }

    // 验证日期格式
    if(stringToDate(record.getCreateTime()).isNull())
    {
        m_lastError = "无效的日期格式";
        return false;
    }

    return true;
}

QString BusinessLogic::getValidationError()
{
    return m_lastError;
}

bool BusinessLogic::isSameMonth(const QString& dateString, int year, int month)
{
    QDate date = stringToDate(dateString);
    return date.year() == year && date.month() == month;
}

QDate BusinessLogic::stringToDate(const QString& dateString)
{
    // 支持多种日期格式
    QDateTime dt = QDateTime::fromString(dateString, "yyyy-MM-dd HH:mm:ss");
    if(dt.isValid())
        return dt.date();

    dt = QDateTime::fromString(dateString, "yyyy-MM-dd");
    if(dt.isValid())
        return dt.date();

    dt = QDateTime::fromString(dateString, "MM/dd HH:mm");
    if(dt.isValid())
        return dt.date();

    return QDate();
}

bool BusinessLogic::checkAbnormalConsumption(const QMap<QString, double>& currentMonth,
                                             const QMap<QString, double>& lastMonth) {
    // 环比激增50%则判定为异常
    for (auto it = currentMonth.begin(); it != currentMonth.end(); ++it) {
        double last = lastMonth.value(it.key(), 0);
        if (last != 0 && (it.value() - last) / last > 0.5) {
            return true;
        }
    }
    return false;
}
