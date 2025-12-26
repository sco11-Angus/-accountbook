#include "RegisterWidget.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

RegisterWidget::RegisterWidget(QWidget *parent)
    : QWidget(parent)
    , m_userManager(UserManager::getInstance())
    , m_countdownTimer(new QTimer(this))
    , m_countdownSeconds(60) {
    setObjectName("RegisterWidget");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint); // 显示关闭按钮，且为独立窗口
    setWindowTitle("红果记账 - 注册");
    setFixedSize(450, 600);
    initUI();
    initStyleSheet();

    // 连接信号槽
    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterWidget::onRegisterBtnClicked);
    connect(m_getCodeBtn, &QPushButton::clicked, this, &RegisterWidget::onGetCodeBtnClicked);
    connect(m_countdownTimer, &QTimer::timeout, this, &RegisterWidget::updateCountdown);
}

void RegisterWidget::initUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(40, 40, 40, 40);

   // 标题标签
    QLabel *titleLabel = new QLabel("账号注册");
    titleLabel->setObjectName("m_titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 注册卡片
    QFrame *registerCard = new QFrame();
    registerCard->setObjectName("m_registerCard");
    QVBoxLayout *cardLayout = new QVBoxLayout(registerCard);
    cardLayout->setSpacing(20);
    cardLayout->setContentsMargins(30, 30, 30, 30);

    // 账号输入框
    m_accountEdit = new QLineEdit();
    m_accountEdit->setPlaceholderText("请输入手机号或邮箱");
    // 设置手机号验证器
    QRegularExpression regExp("^1[3-9]\\d{9}$|^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    m_accountEdit->setValidator(new QRegularExpressionValidator(regExp, this));
    cardLayout->addWidget(m_accountEdit);

    // 密码输入框
    m_pwdEdit = new QLineEdit();
    m_pwdEdit->setEchoMode(QLineEdit::Password);
    m_pwdEdit->setPlaceholderText("请输入密码（需包含大小写字母、数字和特殊字符，至少8位）");
    cardLayout->addWidget(m_pwdEdit);

    // 确认密码输入框
    m_confirmPwdEdit = new QLineEdit();
    m_confirmPwdEdit->setEchoMode(QLineEdit::Password);
    m_confirmPwdEdit->setPlaceholderText("请确认密码");
    cardLayout->addWidget(m_confirmPwdEdit);

    // 验证码布局
    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit();
    m_codeEdit->setPlaceholderText("请输入验证码");
    m_getCodeBtn = new QPushButton("获取验证码");
    m_getCodeBtn->setObjectName("m_getCodeBtn");
    m_getCodeBtn->setCursor(Qt::PointingHandCursor);
    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_getCodeBtn);
    cardLayout->addLayout(codeLayout);

    // 注册按钮
    m_registerBtn = new QPushButton("注 册");
    m_registerBtn->setObjectName("m_registerBtn");
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_registerBtn);

    mainLayout->addWidget(registerCard);

    // 注册按钮下方添加返回按钮
    m_backBtn = new QPushButton("返回登录");
    m_backBtn->setObjectName("m_backBtn");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_backBtn);  // 添加到卡片布局

    // 间隔器
    mainLayout->addStretch();

    // 连接返回按钮信号
    connect(m_backBtn, &QPushButton::clicked, this, &RegisterWidget::onBackBtnClicked);

    // 提示标签
    m_tipLabel = new QLabel("");
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setStyleSheet("color: #FF6B6B; font-size: 12px; margin-top: 10px;");
    mainLayout->addWidget(m_tipLabel);

    // 间隔器
    mainLayout->addStretch();
}

void RegisterWidget::initStyleSheet() {
    setStyleSheet(R"(
        QWidget#RegisterWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #E0F7FA);
        }
        QLabel#m_titleLabel {
            font-family: "Microsoft YaHei";
            font-size: 32px;
            font-weight: bold;
            color: #5D5D5D;
            letter-spacing: 4px;
            margin: 20px 0;
        }
        QFrame#m_registerCard {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 25px;
            border: 1px solid rgba(255, 255, 255, 0.5);
            box-shadow: 0 8px 20px rgba(0, 0, 0, 0.05);
        }
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
        QPushButton#m_registerBtn {
            background-color: #A8E6CF;
            color: #4A4A4A;
            border-radius: 15px;
            font-weight: bold;
            font-size: 16px;
            padding: 12px;
            border: none;
            font-family: "Microsoft YaHei";
        }
        QPushButton#m_registerBtn:hover {
            background-color: #98D8BF;
            box-shadow: 0 4px 12px rgba(168, 230, 207, 0.3);
        }
        QPushButton#m_registerBtn:pressed {
            background-color: #87C7AF;
        }
        QPushButton#m_getCodeBtn {
            background-color: #E8F5E9;
            color: #4A4A4A;
            border-radius: 12px;
            font-size: 14px;
            padding: 0 15px;
            margin-left: 10px;
            border: none;
        }
        QPushButton#m_getCodeBtn:disabled {
            background-color: #E0E0E0;
            color: #9E9E9E;
        }
        QPushButton#m_backBtn {
            background-color: #A8E6CF;
            color: #4A4A4A;
            border-radius: 15px;
            font-weight: bold;
            font-size: 16px;
            padding: 12px;
            border: none;
            font-family: "Microsoft YaHei";
        }
        QPushButton#m_backBtn:hover {
            background-color: #98D8BF;
            box-shadow: 0 4px 12px rgba(168, 230, 207, 0.3);
        }
    )");
}

bool RegisterWidget::checkInputValidity() {
    QString account = m_accountEdit->text().trimmed();
    QString password = m_pwdEdit->text().trimmed();
    QString confirmPwd = m_confirmPwdEdit->text().trimmed();
    QString code = m_codeEdit->text().trimmed();

    if (account.isEmpty()) {
        m_tipLabel->setText("请输入账号（手机号或邮箱）");
        return false;
    }

    if (password.isEmpty()) {
        m_tipLabel->setText("请输入密码");
        return false;
    }

    if (!m_userManager->checkPasswordStrength(password)) {
        m_tipLabel->setText("密码强度不足（需包含大小写字母、数字和特殊字符，至少8位）");
        return false;
    }

    if (password != confirmPwd) {
        m_tipLabel->setText("两次输入的密码不一致");
        return false;
    }

    if (code.isEmpty()) {
        m_tipLabel->setText("请输入验证码");
        return false;
    }

    return true;
}

void RegisterWidget::onGetCodeBtnClicked() {
    QString account = m_accountEdit->text().trimmed();
    if (account.isEmpty()) {
        m_tipLabel->setText("请先输入账号（手机号或邮箱）");
        return;
    }

    // 模拟发送验证码
    m_tipLabel->setText("验证码已发送，请注意查收（测试验证码：123456）");
    m_getCodeBtn->setDisabled(true);
    m_countdownSeconds = 60;
    m_countdownTimer->start(1000);
    updateCountdown();
}

void RegisterWidget::updateCountdown() {
    if (m_countdownSeconds > 0) {
        m_getCodeBtn->setText(QString("重新获取（%1s）").arg(m_countdownSeconds));
        m_countdownSeconds--;
    } else {
        m_countdownTimer->stop();
        m_getCodeBtn->setText("获取验证码");
        m_getCodeBtn->setDisabled(false);
    }
}

void RegisterWidget::onRegisterBtnClicked() {
    if (!checkInputValidity()) {
        return;
    }

    QString account = m_accountEdit->text().trimmed();
    QString password = m_pwdEdit->text().trimmed();
    QString code = m_codeEdit->text().trimmed();

    bool success = m_userManager->registerUser(account, password, code);
    if (success) {
        QMessageBox::information(this, "注册成功", "账号注册成功，请登录！");
        emit registerSuccess(account);  // 发射注册成功信号
        close();
    } else {
        m_tipLabel->setText("注册失败，可能是账号已存在或验证码错误");
    }
}

void RegisterWidget::onBackBtnClicked() {
    emit backToLogin(); // 发送返回信号
    close(); // 关闭注册窗口（触发WA_DeleteOnClose释放内存）
}
