#include "ui_handler.h"

UIHandler::UIHandler(QObject *parent) : QObject(parent)
{
    m_businessLogic = new BusinessLogic(this);
    m_accountManager = new AccountManager();
    m_userManager = UserManager::getInstance();
}

bool UIHandler::login(const QString& account, const QString& password, bool rememberPwd)
{
    User user = m_userManager->login(account, password, rememberPwd);
    if(user.getId() > 0)
    {
        emit loginSuccess(user);
        return true;
    }
    else
    {
        emit loginFailed("账号或密码错误");
        return false;
    }
}

bool UIHandler::registerUser(const QString& account, const QString& password, const QString& code)
{
    if(m_userManager->registerUser(account, password, code))
    {
        emit registerSuccess();
        return true;
    }
    else
    {
        emit registerFailed("注册失败，请检查信息");
        return false;
    }
}

bool UIHandler::updateUserInfo(const User& user)
{
    return m_userManager->updateUserInfo(user);
}

bool UIHandler::changePassword(int userId, const QString& oldPwd, const QString& newPwd)
{
    return m_userManager->changePassword(userId, oldPwd, newPwd);
}

bool UIHandler::resetPassword(const QString& account, const QString& code, const QString& newPwd)
{
    return m_userManager->resetPassword(account, code, newPwd);
}

void UIHandler::logout()
{
    m_userManager->logout();
}

User UIHandler::getCurrentUser()
{
    return m_userManager->getCurrentUser();
}

bool UIHandler::addRecord(const AccountRecord& record)
{
    if(!m_businessLogic->validateBillRecord(record))
    {
        emit recordAdded(false, m_businessLogic->getValidationError());
        return false;
    }

    int newId = m_accountManager->addAccountRecord(record);
    bool success = (newId > 0);
    emit recordAdded(success, success ? "记录添加成功" : "记录添加失败");
    return success;
}

bool UIHandler::editRecord(const AccountRecord& record)
{
    if(!m_businessLogic->validateBillRecord(record))
    {
        emit recordUpdated(false, m_businessLogic->getValidationError());
        return false;
    }

    bool success = m_accountManager->editAccountRecord(record);
    emit recordUpdated(success, success ? "记录更新成功" : "记录更新失败");
    return success;
}

bool UIHandler::deleteRecord(int recordId)
{
    bool success = m_accountManager->deleteAccountRecord(recordId);
    emit recordDeleted(success, success ? "记录已移至回收站" : "删除失败");
    return success;
}

bool UIHandler::restoreRecord(int recordId)
{
    bool success = m_accountManager->restoreAccountRecord(recordId);
    emit recordUpdated(success, success ? "记录已恢复" : "恢复失败");
    return success;
}

bool UIHandler::permanentDeleteRecord(int recordId)
{
    bool success = m_accountManager->permanentDeleteAccountRecord(recordId);
    emit recordDeleted(success, success ? "记录已永久删除" : "删除失败");
    return success;
}

QList<AccountRecord> UIHandler::getRecordsByDateRange(int userId, const QDate& start, const QDate& end)
{
    return m_accountManager->queryRecordsByDateRange(userId, start, end);
}

QList<AccountRecord> UIHandler::getMonthlyRecords(int userId, int year, int month)
{
    return m_accountManager->queryMonthlyRecords(userId, year, month);
}

QList<AccountRecord> UIHandler::getRecordsByCategory(int userId, const QString& category)
{
    return m_accountManager->queryRecordsByType(userId, category);
}

QList<AccountRecord> UIHandler::searchRecords(int userId, const QString& keyword)
{
    // 这里简化实现，实际应实现更复杂的搜索逻辑
    QList<AccountRecord> allRecords = m_accountManager->queryAccountRecord(userId);
    QList<AccountRecord> results;

    for(const auto& record : allRecords)
    {
        if(record.getRemark().contains(keyword, Qt::CaseInsensitive) ||
            record.getType().contains(keyword, Qt::CaseInsensitive))
        {
            results.append(record);
        }
    }

    return results;
}

double UIHandler::getMonthlyBalance(int userId, int year, int month)
{
    QList<AccountRecord> records = getMonthlyRecords(userId, year, month);
    return m_businessLogic->calculateMonthlyBalance(year, month, records);
}

QMap<QString, double> UIHandler::getMonthlyCategorySummary(int userId, int year, int month)
{
    QList<AccountRecord> records = getMonthlyRecords(userId, year, month);
    return m_businessLogic->calculateMonthlyCategorySummary(year, month, records);
}

QStringList UIHandler::getExpenseCategories()
{
    return m_businessLogic->getValidCategories(true);
}

QStringList UIHandler::getIncomeCategories()
{
    return m_businessLogic->getValidCategories(false);
}

bool UIHandler::addCustomCategory(const QString& category, bool isExpense)
{
    return m_businessLogic->addCustomCategory(category, isExpense);
}
