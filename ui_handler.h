#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include <QObject>
#include "business_logic.h"
#include "account_manager.h"
#include "user_manager.h"
#include "account_record.h"
#include "User.h"

class UIHandler : public QObject
{
    Q_OBJECT
public:
    explicit UIHandler(QObject *parent = nullptr);

    // 用户相关操作
    bool login(const QString& account, const QString& password, bool rememberPwd);
    bool registerUser(const QString& account, const QString& password, const QString& code);
    bool updateUserInfo(const User& user);
    bool changePassword(int userId, const QString& oldPwd, const QString& newPwd);
    bool resetPassword(const QString& account, const QString& code, const QString& newPwd);
    void logout();
    User getCurrentUser();

    // 记账相关操作
    bool addRecord(const AccountRecord& record);
    bool editRecord(const AccountRecord& record);
    bool deleteRecord(int recordId);
    bool restoreRecord(int recordId);
    bool permanentDeleteRecord(int recordId);

    // 查询相关操作
    QList<AccountRecord> getRecordsByDateRange(int userId, const QDate& start, const QDate& end);
    QList<AccountRecord> getMonthlyRecords(int userId, int year, int month);
    QList<AccountRecord> getRecordsByCategory(int userId, const QString& category);
    QList<AccountRecord> searchRecords(int userId, const QString& keyword);

    // 统计相关操作
    double getMonthlyBalance(int userId, int year, int month);
    QMap<QString, double> getMonthlyCategorySummary(int userId, int year, int month);

    // 分类管理
    QStringList getExpenseCategories();
    QStringList getIncomeCategories();
    bool addCustomCategory(const QString& category, bool isExpense);

signals:
    void loginSuccess(const User& user);
    void loginFailed(const QString& reason);
    void registerSuccess();
    void registerFailed(const QString& reason);
    void recordAdded(bool success, const QString& message);
    void recordUpdated(bool success, const QString& message);
    void recordDeleted(bool success, const QString& message);
    void dataSynced();

private:
    BusinessLogic* m_businessLogic;
    AccountManager* m_accountManager;
    UserManager* m_userManager;
};

#endif // UI_HANDLER_H
