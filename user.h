// user.h
#ifndef USER_H
#define USER_H

#include <QString>

class User {
public:
    User() = default;
    User(const QString& account, const QString& password, const QString& nickname = "默认昵称");

    // Getter & Setter
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString getAccount() const { return m_account; }
    void setAccount(const QString& account) { m_account = account; }

    QString getPassword() const { return m_password; }
    void setPassword(const QString& password) { m_password = password; }

    QString getNickname() const { return m_nickname; }
    void setNickname(const QString& nickname) { m_nickname = nickname; }

    QString getAvatar() const { return m_avatar; }
    void setAvatar(const QString& avatar) { m_avatar = avatar; }

    int getGender() const { return m_gender; }
    void setGender(int gender) { m_gender = gender; }

    QString getPayMethod() const { return m_payMethod; }
    void setPayMethod(const QString& payMethod) { m_payMethod = payMethod; }

    int getLoginFailCount() const { return m_loginFailCount; }
    void setLoginFailCount(int count) { m_loginFailCount = count; }

    QString getLockTime() const { return m_lockTime; }
    void setLockTime(const QString& lockTime) { m_lockTime = lockTime; }

    QString getCreateTime() const { return m_createTime; }
    void setCreateTime(const QString& createTime) { m_createTime = createTime; }

private:
    int m_id = 0;
    QString m_account;     // 手机号/邮箱
    QString m_password;    // 加密后的密码
    QString m_nickname;    // 昵称
    QString m_avatar;      // 头像路径
    int m_gender = 0;      // 0:未知 1:男 2:女
    QString m_payMethod;   // 常用支付方式
    int m_loginFailCount = 0; // 登录失败次数
    QString m_lockTime;    // 账号锁定时间
    QString m_createTime;  // 创建时间
};

#endif // USER_H
