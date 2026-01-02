#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "User.h"
#include "sqlite_helper.h"
#include <QString>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMutex>

class UserManager {
public:
    // 单例获取方法
    static UserManager* getInstance();
    // 禁用拷贝构造和赋值（防止实例复制）
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    // 密码强度校验（大小写+数字+特殊字符）
    bool checkPasswordStrength(const QString& password);
    // 验证码校验（模拟，实际需对接短信/邮箱接口）
    bool verifyCode(const QString& account, const QString& code);
    // 用户注册
    bool registerUser(const QString& account, const QString& password, const QString& code);
    // 用户登录（返回登录用户，空则失败）
    User login(const QString& account, const QString& password, bool rememberPwd = false);
    // 检查账号是否锁定
    bool isAccountLocked(const QString& account);
    // 解锁账号
    bool unlockAccount(const QString& account);
    // 修改用户信息
    bool updateUserInfo(const User& user);
    // 修改密码（原密码验证）
    bool changePassword(int userId, const QString& oldPwd, const QString& newPwd);
    // 忘记密码（验证码找回）
    bool resetPassword(const QString& account, const QString& code, const QString& newPwd);
    // 安全退出（清除本地缓存）
    void logout();
    // 获取当前登录用户
    void setCurrentUser(const User& user) {  // 保存当前登录用户
        m_currentUser = user;
    }

    User getCurrentUser() { return m_currentUser; }
private:
    // 单例静态成员
    static UserManager* m_instance;
    static QMutex m_mutex; // 线程安全锁

    UserManager();

    // 密码加密（MD5）
    QString encryptPassword(const QString& password);

    User m_currentUser;  // 当前登录用户
    SqliteHelper* m_dbHelper;
};

#endif // USER_MANAGER_H
