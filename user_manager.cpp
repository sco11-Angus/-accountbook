#include "user_manager.h"

UserManager::UserManager() {
    m_dbHelper = SqliteHelper::getInstance();
    m_dbHelper->openDatabase();
}

bool UserManager::checkPasswordStrength(const QString& password) {
    bool hasUpper = false, hasLower = false, hasNumber = false, hasSpecial = false;
    for (QChar c : password) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasNumber = true;
        else if (!c.isSpace()) hasSpecial = true;
    }
    return hasUpper && hasLower && hasNumber && hasSpecial && password.length() >= 8;
}

bool UserManager::verifyCode(const QString& account, const QString& code) {
    // 模拟验证码校验（实际需对接短信/邮箱平台）
    Q_UNUSED(account);
    return code == "123456"; // 测试用固定验证码
}

QString UserManager::encryptPassword(const QString& password) {
    // MD5加密
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    return hash.result().toHex();
}

bool UserManager::registerUser(const QString& account, const QString& password, const QString& code) {
    // 1. 校验验证码
    if (!verifyCode(account, code)) return false;
    // 2. 校验密码强度
    if (!checkPasswordStrength(password)) return false;
    // 3. 检查账号是否已存在
    QString sql = QString("SELECT id FROM user WHERE account = '%1'").arg(account);
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (query.next()) return false; // 账号已存在

    // 4. 插入用户数据
    QString createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString encPwd = encryptPassword(password);
    sql = QString(R"(
        INSERT INTO user (account, password, nickname, create_time)
        VALUES ('%1', '%2', '用户%3', '%4')
    )").arg(account).arg(encPwd).arg(account.right(4)).arg(createTime);

    return m_dbHelper->executeSql(sql);
}

User UserManager::login(const QString& account, const QString& password, bool rememberPwd) {
    Q_UNUSED(rememberPwd);
    // 1. 检查账号是否锁定
    if (isAccountLocked(account)) return User();

    // 2. 验证账号密码
    QString encPwd = encryptPassword(password);
    QString sql = QString(R"(
        SELECT id, account, password, nickname, avatar, gender, pay_method, login_fail_count
        FROM user WHERE account = '%1'
    )").arg(account);
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (!query.next()) return User(); // 账号不存在

    // 3. 密码匹配
    if (query.value("password").toString() != encPwd) {
        // 登录失败次数+1
        int failCount = query.value("login_fail_count").toInt() + 1;
        QString updateSql = QString(R"(
            UPDATE user SET login_fail_count = %1 %2 WHERE account = '%3'
        )").arg(failCount)
                                .arg(failCount >=5 ? QString(", lock_time = '%1'").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")) : "")
                                .arg(account);
        m_dbHelper->executeSql(updateSql);
        return User();
    }

    // 4. 登录成功，重置失败次数
    QString resetSql = QString("UPDATE user SET login_fail_count = 0 WHERE account = '%1'").arg(account);
    m_dbHelper->executeSql(resetSql);

    // 5. 封装当前用户
    m_currentUser.setId(query.value("id").toInt());
    m_currentUser.setAccount(account);
    m_currentUser.setPassword(encPwd);
    m_currentUser.setNickname(query.value("nickname").toString());
    m_currentUser.setAvatar(query.value("avatar").toString());
    m_currentUser.setGender(query.value("gender").toInt());
    m_currentUser.setPayMethod(query.value("pay_method").toString());

    return m_currentUser;
}

bool UserManager::isAccountLocked(const QString& account) {
    QString sql = QString("SELECT lock_time FROM user WHERE account = '%1'").arg(account);
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (!query.next()) return false;

    QString lockTime = query.value("lock_time").toString();
    if (lockTime.isEmpty()) return false;

    // 锁定时间超过5秒则自动解锁
    QDateTime lockDt = QDateTime::fromString(lockTime, "yyyy-MM-dd HH:mm:ss");
    if (lockDt.secsTo(QDateTime::currentDateTime()) >= 5) {
        unlockAccount(account);
        return false;
    }
    return true;
}

bool UserManager::unlockAccount(const QString& account) {
    QString sql = QString("UPDATE user SET lock_time = '', login_fail_count = 0 WHERE account = '%1'").arg(account);
    return m_dbHelper->executeSql(sql);
}

bool UserManager::updateUserInfo(const User& user) {
    QString sql = QString(R"(
        UPDATE user SET nickname = '%1', avatar = '%2', gender = %3, pay_method = '%4'
        WHERE id = %5
    )").arg(user.getNickname())
                      .arg(user.getAvatar())
                      .arg(user.getGender())
                      .arg(user.getPayMethod())
                      .arg(user.getId());

    bool ret = m_dbHelper->executeSql(sql);
    if (ret) m_currentUser = user;
    return ret;
}

bool UserManager::changePassword(int userId, const QString& oldPwd, const QString& newPwd) {
    // 1. 验证原密码
    QString encOldPwd = encryptPassword(oldPwd);
    QString sql = QString("SELECT id FROM user WHERE id = %1 AND password = '%2'").arg(userId).arg(encOldPwd);
    QSqlQuery query = m_dbHelper->executeQuery(sql);
    if (!query.next()) return false;

    // 2. 校验新密码强度
    if (!checkPasswordStrength(newPwd)) return false;

    // 3. 修改密码
    QString encNewPwd = encryptPassword(newPwd);
    sql = QString("UPDATE user SET password = '%1' WHERE id = %2").arg(encNewPwd).arg(userId);
    return m_dbHelper->executeSql(sql);
}

bool UserManager::resetPassword(const QString& account, const QString& code, const QString& newPwd) {
    // 1. 验证验证码
    if (!verifyCode(account, code)) return false;
    // 2. 校验新密码强度
    if (!checkPasswordStrength(newPwd)) return false;

    // 3. 重置密码
    QString encNewPwd = encryptPassword(newPwd);
    QString sql = QString("UPDATE user SET password = '%1' WHERE account = '%2'").arg(encNewPwd).arg(account);
    return m_dbHelper->executeSql(sql);
}

void UserManager::logout() {
    // 清除当前用户缓存
    m_currentUser = User();
    // 实际项目中需清除本地加密存储的密码、登录凭证等
}
