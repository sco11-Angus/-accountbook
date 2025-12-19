#ifndef REGISTERWIDGET_H
#define REGISTERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "user_manager.h"

class RegisterWidget : public QWidget {
    Q_OBJECT
public:
    explicit RegisterWidget(QWidget *parent = nullptr);

private slots:
    void onRegisterBtnClicked();
    void onGetCodeBtnClicked();
    void updateCountdown();
    void onBackBtnClicked();

signals:
    void backToLogin();
    void registerSuccess(const QString& account);

private:
    void initUI();
    void initStyleSheet();
    bool checkInputValidity();

    UserManager *m_userManager;
    QLineEdit *m_accountEdit;      // 账号输入框
    QLineEdit *m_pwdEdit;          // 密码输入框
    QLineEdit *m_confirmPwdEdit;   // 确认密码输入框
    QLineEdit *m_codeEdit;         // 验证码输入框
    QPushButton *m_getCodeBtn;     // 获取验证码按钮
    QPushButton *m_registerBtn;    // 注册按钮
    QLabel *m_tipLabel;            // 提示标签
    QTimer *m_countdownTimer;      // 倒计时定时器
    int m_countdownSeconds;        // 倒计时秒数
    QPushButton *m_backBtn;        // 返回按钮
};

#endif // REGISTERWIDGET_H
