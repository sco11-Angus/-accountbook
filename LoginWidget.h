#ifndef LOGIN_WIDGET_H
#define LOGIN_WIDGET_H

#include "user_manager.h"
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QFrame>

class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget() override;

signals:
    // 登录成功信号
    void loginSuccess(const User& user);

private slots:
    void onLoginBtnClicked();
    void onRegisterBtnClicked();
    void onForgotPwdBtnClicked();

private:
    void initUI();          // 初始化界面布局
    void initStyleSheet();  // 初始化样式表

    // 核心业务逻辑
    UserManager* m_userManager;

    // UI控件
    QLabel* m_titleLabel;          // 红果记账标题
    QFrame* m_loginCard;           // 登录卡片
    QLineEdit* m_accountEdit;      // 账号输入框
    QLineEdit* m_pwdEdit;          // 密码输入框
    QCheckBox* m_rememberPwdCheck; // 记住密码
    QCheckBox* m_autoLoginCheck;   // 自动登录
    QPushButton* m_loginBtn;       // 登录按钮
    QPushButton* m_registerBtn;    // 注册按钮
    QPushButton* m_forgotPwdBtn;   // 忘记密码按钮
    QLabel* m_tipLabel;            // 提示标签（错误/锁定提示）
};

#endif // LOGIN_WIDGET_H
