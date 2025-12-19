#include "LoginWidget.h"
#include "RegisterWidget.h"
#include <QMessageBox>
#include <QCursor>
#include <QGradient>
#include <QPalette>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , m_userManager(new UserManager())
{
    setObjectName("LoginWidget");
    setWindowTitle("红果记账 - 登录");
    setFixedSize(450, 600); // 匹配XML尺寸
    initUI();
    initStyleSheet();
}

LoginWidget::~LoginWidget() {
    delete m_userManager;
}

void LoginWidget::initUI() {
    // ========== 主布局 ==========
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(40, 40, 40, 40); // 匹配XML边距

    // 顶部垂直间隔器
    QSpacerItem* verticalSpacer_t = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Fixed);
    mainLayout->addItem(verticalSpacer_t);

    // ========== 标题标签 ==========
    m_titleLabel = new QLabel("红果记账");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // ========== 登录卡片 ==========
    m_loginCard = new QFrame();
    QVBoxLayout* cardLayout = new QVBoxLayout(m_loginCard);
    cardLayout->setSpacing(20);
    cardLayout->setContentsMargins(30, 30, 30, 30); // 卡片内边距

    // 账号输入框
    m_accountEdit = new QLineEdit();
    m_accountEdit->setPlaceholderText("手机号 / 邮箱");
    cardLayout->addWidget(m_accountEdit);

    // 密码输入框
    m_pwdEdit = new QLineEdit();
    m_pwdEdit->setEchoMode(QLineEdit::Password);
    m_pwdEdit->setPlaceholderText("密码");
    cardLayout->addWidget(m_pwdEdit);

    // 记住密码/自动登录复选框布局
    QHBoxLayout* checkLayout = new QHBoxLayout();
    m_rememberPwdCheck = new QCheckBox("记住密码");
    m_autoLoginCheck = new QCheckBox("自动登录");
    checkLayout->addWidget(m_rememberPwdCheck);
    checkLayout->addWidget(m_autoLoginCheck);
    cardLayout->addLayout(checkLayout);

    // 登录按钮
    m_loginBtn = new QPushButton("登 录");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_loginBtn);

    // 注册/忘记密码按钮布局
    QHBoxLayout* subBtnLayout = new QHBoxLayout();
    m_registerBtn = new QPushButton("新用户注册");
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_forgotPwdBtn = new QPushButton("忘记密码？");
    subBtnLayout->addWidget(m_registerBtn);
    subBtnLayout->addItem(horizontalSpacer);
    subBtnLayout->addWidget(m_forgotPwdBtn);
    cardLayout->addLayout(subBtnLayout);

    mainLayout->addWidget(m_loginCard);

    // 提示标签（错误/锁定提示）
    m_tipLabel = new QLabel("");
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setStyleSheet("color: #FF6B6B; font-size: 12px; margin-top: 10px;");
    mainLayout->addWidget(m_tipLabel);

    // 底部垂直间隔器
    QSpacerItem* verticalSpacer_b = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Fixed);
    mainLayout->addItem(verticalSpacer_b);

    // ========== 信号槽绑定 ==========
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginBtnClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginWidget::onRegisterBtnClicked);
    connect(m_forgotPwdBtn, &QPushButton::clicked, this, &LoginWidget::onForgotPwdBtnClicked);
}

void LoginWidget::initStyleSheet() {
    // 主窗口渐变背景
    setStyleSheet(R"(
        QWidget#LoginWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #E0F7FA);
        }

        /* 标题样式 */
        QLabel#m_titleLabel {
            font-family: "Microsoft YaHei";
            font-size: 32px;
            font-weight: bold;
            color: #5D5D5D;
            letter-spacing: 4px;
            margin-bottom: 20px;
        }

        /* 登录卡片样式（轻玻璃模糊+悬浮感） */
        QFrame#m_loginCard {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 25px;
            border: 1px solid rgba(255, 255, 255, 0.5);
            box-shadow: 0 8px 20px rgba(0, 0, 0, 0.05);
        }

        /* 输入框样式 */
        QLineEdit {
            background-color: #F8F9FA;
            border: 1px solid #E9ECEF;
            border-radius: 12px;
            padding: 10px 15px;
            font-size: 14px;
            color: #495057;
            font-family: "Microsoft YaHei";
        }
        QLineEdit:focus {
            border: 1px solid #A8E6CF;
            background-color: #FFFFFF;
            outline: none;
        }

        /* 登录按钮样式 */
        QPushButton#m_loginBtn {
            background-color: #A8E6CF;
            color: #4A4A4A;
            border-radius: 15px;
            font-weight: bold;
            font-size: 16px;
            padding: 12px;
            border: none;
            font-family: "Microsoft YaHei";
        }
        QPushButton#m_loginBtn:hover {
            background-color: #98D8BF;
            box-shadow: 0 4px 12px rgba(168, 230, 207, 0.3);
        }
        QPushButton#m_loginBtn:pressed {
            background-color: #87C7AF;
        }

        /* 注册/忘记密码按钮样式 */
        QPushButton#m_registerBtn, QPushButton#m_forgotPwdBtn {
            border: none;
            color: #888888;
            font-size: 12px;
            background: transparent;
            font-family: "Microsoft YaHei";
        }
        QPushButton#m_registerBtn:hover, QPushButton#m_forgotPwdBtn:hover {
            color: #A8E6CF;
        }

        /* 复选框样式 */
        QCheckBox {
            color: #777777;
            font-size: 12px;
            font-family: "Microsoft YaHei";
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border-radius: 3px;
            border: 1px solid #E9ECEF;
        }
        QCheckBox::indicator:checked {
            background-color: #A8E6CF;
            border-color: #A8E6CF;
            image: url(:/icons/check.png); /* 可选：添加对勾图标，若无则注释 */
        }
    )");

    // 给控件设置objectName（匹配样式表选择器）
    m_titleLabel->setObjectName("m_titleLabel");
    m_loginCard->setObjectName("m_loginCard");
    m_loginBtn->setObjectName("m_loginBtn");
    m_registerBtn->setObjectName("m_registerBtn");
    m_forgotPwdBtn->setObjectName("m_forgotPwdBtn");
}

void LoginWidget::onLoginBtnClicked() {
    QString account = m_accountEdit->text().trimmed();
    QString password = m_pwdEdit->text().trimmed();

    // 空值校验
    if (account.isEmpty() || password.isEmpty()) {
        m_tipLabel->setText("账号/密码不能为空！");
        return;
    }

    // 检查账号是否锁定
    if (m_userManager->isAccountLocked(account)) {
        m_tipLabel->setText("账号已锁定！1小时后自动解锁");
        return;
    }

    // 执行登录逻辑
    User user = m_userManager->login(account, password, m_rememberPwdCheck->isChecked());
    if (user.getId() > 0) {
        m_tipLabel->setText("");
        emit loginSuccess(user);
        close();
    } else {
        m_tipLabel->setText("账号/密码错误！");
        // 清空密码框
        m_pwdEdit->clear();
    }
}

void LoginWidget::onRegisterBtnClicked() {
    // 1. 创建注册窗口，设置为独立窗口（无父窗口，避免重叠）
    RegisterWidget* regWidget = new RegisterWidget(nullptr);

    // 2. 设置窗口属性：独立窗口、模态（阻塞登录窗口操作）
    regWidget->setWindowModality(Qt::ApplicationModal); // 应用级模态，整个程序只有注册窗口可操作
    regWidget->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放内存，避免内存泄漏

    // 3. 调整注册窗口位置：居中屏幕（或偏移登录窗口，避免重叠）
    QRect screenRect = QApplication::primaryScreen()->geometry();
    int x = (screenRect.width() - regWidget->width()) / 2;
    int y = (screenRect.height() - regWidget->height()) / 2;
    regWidget->move(x, y); // 居中屏幕显示，彻底避免和登录窗口重叠

    // 4. 绑定信号：注册成功后自动填充账号到登录界面
    connect(regWidget, &RegisterWidget::registerSuccess, this, [=](const QString& account) {
        m_accountEdit->setText(account); // 填充账号
        this->activateWindow(); // 激活登录窗口（回到前台）
    });

    // 5. 绑定信号：返回登录时激活登录窗口
    connect(regWidget, &RegisterWidget::backToLogin, this, [=]() {
        this->activateWindow(); // 登录窗口回到前台
    });

    // 6. 显示注册窗口
    regWidget->show();
}

void LoginWidget::onForgotPwdBtnClicked() {
    QMessageBox::information(this, "提示", "忘记密码功能待开发！");
}
