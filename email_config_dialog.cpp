#include "email_config_dialog.h"
#include "user_manager.h"
#include "email_sender.h"
#include <QRegularExpression>

EmailConfigDialog::EmailConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("配置邮件发送服务");
    setFixedSize(450, 350);
    initUI();
}

void EmailConfigDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 标题
    QLabel *titleLabel = new QLabel("配置QQ邮箱发送服务");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);

    // 说明文字
    QLabel *descLabel = new QLabel(
        "请配置用于发送验证码的QQ邮箱账号和授权码。\n"
        "授权码获取方法：\n"
        "1. 登录QQ邮箱网页版\n"
        "2. 设置 -> 账户 -> 开启SMTP服务\n"
        "3. 生成授权码（不是登录密码）"
        );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #666; font-size: 12px; padding: 10px; background: #F5F5F5; border-radius: 5px;");
    mainLayout->addWidget(descLabel);

    // 表单
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);

    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText("例如: your_email@qq.com");
    m_emailEdit->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #DDD; border-radius: 5px; }");
    formLayout->addRow("发送邮箱:", m_emailEdit);

    m_authCodeEdit = new QLineEdit();
    m_authCodeEdit->setPlaceholderText("请输入QQ邮箱授权码");
    m_authCodeEdit->setEchoMode(QLineEdit::Password);
    m_authCodeEdit->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #DDD; border-radius: 5px; }");
    formLayout->addRow("授权码:", m_authCodeEdit);

    mainLayout->addLayout(formLayout);

    // 提示标签
    m_tipLabel = new QLabel("");
    m_tipLabel->setStyleSheet("color: #FF6B6B; font-size: 12px;");
    m_tipLabel->setWordWrap(true);
    mainLayout->addWidget(m_tipLabel);

    mainLayout->addStretch();

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    QPushButton *testBtn = new QPushButton("测试连接");
    testBtn->setStyleSheet(
        "QPushButton { background: #E8F5E9; color: #4A4A4A; border: 1px solid #A8E6CF; "
        "border-radius: 5px; padding: 8px 20px; font-size: 14px; }"
        "QPushButton:hover { background: #C8E6C9; }"
        );
    connect(testBtn, &QPushButton::clicked, this, &EmailConfigDialog::onTestClicked);
    btnLayout->addWidget(testBtn);

    btnLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setStyleSheet(
        "QPushButton { background: #F5F5F5; color: #333; border: 1px solid #DDD; "
        "border-radius: 5px; padding: 8px 20px; font-size: 14px; }"
        "QPushButton:hover { background: #E0E0E0; }"
        );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton *confirmBtn = new QPushButton("保存配置");
    confirmBtn->setStyleSheet(
        "QPushButton { background: #A8E6CF; color: #4A4A4A; border: none; "
        "border-radius: 5px; padding: 8px 20px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #98D8BF; }"
        );
    connect(confirmBtn, &QPushButton::clicked, this, &EmailConfigDialog::onConfirmClicked);
    btnLayout->addWidget(confirmBtn);

    mainLayout->addLayout(btnLayout);
}

void EmailConfigDialog::onConfirmClicked() {
    QString email = m_emailEdit->text().trimmed();
    QString authCode = m_authCodeEdit->text().trimmed();

    if (email.isEmpty()) {
        m_tipLabel->setText("请输入发送邮箱");
        return;
    }

    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!emailRegex.match(email).hasMatch()) {
        m_tipLabel->setText("邮箱格式不正确");
        return;
    }

    if (authCode.isEmpty()) {
        m_tipLabel->setText("请输入授权码");
        return;
    }

    // 保存配置
    UserManager::getInstance()->configureEmailSender(email, authCode);
    accept();
}

void EmailConfigDialog::onTestClicked() {
    QString email = m_emailEdit->text().trimmed();
    QString authCode = m_authCodeEdit->text().trimmed();

    if (email.isEmpty() || authCode.isEmpty()) {
        m_tipLabel->setText("请先填写邮箱和授权码");
        return;
    }

    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!emailRegex.match(email).hasMatch()) {
        m_tipLabel->setText("邮箱格式不正确");
        return;
    }

    m_tipLabel->setText("正在测试连接...");

    // 创建临时邮件发送器进行测试
    EmailSender *testSender = new EmailSender(this);
    testSender->setSmtpServer("smtp.qq.com", 465, true);
    testSender->setCredentials(email, authCode);

    // 发送测试邮件到自己的邮箱
    connect(testSender, &EmailSender::sendFinished, this, [this, testSender](bool success, const QString& message) {
        if (success) {
            m_tipLabel->setText("✓ 测试成功！配置正确");
            m_tipLabel->setStyleSheet("color: #4CAF50; font-size: 12px;");
        } else {
            m_tipLabel->setText("✗ 测试失败：" + message);
            m_tipLabel->setStyleSheet("color: #FF6B6B; font-size: 12px;");
        }
        testSender->deleteLater();
    });

    testSender->sendVerificationCode(email, "123456"); // 发送测试验证码
}

