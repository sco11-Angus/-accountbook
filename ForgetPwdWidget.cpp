#include "ForgetPwdWidget.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFrame>

ForgetPwdWidget::ForgetPwdWidget(QWidget *parent)
    : QWidget(parent)
    , m_userManager(UserManager::getInstance())
    , m_countdownTimer(new QTimer(this))
    , m_countdownSeconds(60) {
    setObjectName("ForgetPwdWidget");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setWindowTitle("红果记账 - 找回密码");
    setFixedSize(450, 600);
    initUI();
    initStyleSheet();

    connect(m_resetBtn, &QPushButton::clicked, this, &ForgetPwdWidget::onResetBtnClicked);
    connect(m_getCodeBtn, &QPushButton::clicked, this, &ForgetPwdWidget::onGetCodeBtnClicked);
    connect(m_countdownTimer, &QTimer::timeout, this, &ForgetPwdWidget::updateCountdown);
    connect(m_backBtn, &QPushButton::clicked, this, &ForgetPwdWidget::onBackBtnClicked);
}

void ForgetPwdWidget::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    QLabel *titleLabel = new QLabel("找回密码");
    titleLabel->setObjectName("m_titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QFrame *resetCard = new QFrame();
    resetCard->setObjectName("m_resetCard");
    QVBoxLayout *cardLayout = new QVBoxLayout(resetCard);
    cardLayout->setSpacing(20);
    cardLayout->setContentsMargins(30, 30, 30, 30);

    m_accountEdit = new QLineEdit();
    m_accountEdit->setPlaceholderText("请输入手机号或邮箱");
    QRegularExpression regExp("^1[3-9]\\d{9}$|^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    m_accountEdit->setValidator(new QRegularExpressionValidator(regExp, this));
    cardLayout->addWidget(m_accountEdit);

    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit();
    m_codeEdit->setPlaceholderText("验证码");
    m_getCodeBtn = new QPushButton("获取验证码");
    m_getCodeBtn->setObjectName("m_getCodeBtn");
    m_getCodeBtn->setCursor(Qt::PointingHandCursor);
    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_getCodeBtn);
    cardLayout->addLayout(codeLayout);

    m_newPwdEdit = new QLineEdit();
    m_newPwdEdit->setEchoMode(QLineEdit::Password);
    m_newPwdEdit->setPlaceholderText("新密码");
    cardLayout->addWidget(m_newPwdEdit);

    m_confirmPwdEdit = new QLineEdit();
    m_confirmPwdEdit->setEchoMode(QLineEdit::Password);
    m_confirmPwdEdit->setPlaceholderText("确认新密码");
    cardLayout->addWidget(m_confirmPwdEdit);

    m_resetBtn = new QPushButton("重置密码");
    m_resetBtn->setObjectName("m_resetBtn");
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_resetBtn);

    m_backBtn = new QPushButton("返回登录");
    m_backBtn->setObjectName("m_backBtn");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_backBtn);

    mainLayout->addWidget(resetCard);

    m_tipLabel = new QLabel("");
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setStyleSheet("color: #FF6B6B; font-size: 12px; margin-top: 10px;");
    mainLayout->addWidget(m_tipLabel);

    mainLayout->addStretch();
}

void ForgetPwdWidget::initStyleSheet() {
    setStyleSheet(R"(
        #ForgetPwdWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, 
                stop:0 #A8E6CF, stop:1 #DCEDC1);
        }
        #m_titleLabel {
            font-size: 32px;
            font-weight: bold;
            color: white;
            margin-bottom: 30px;
            font-family: "Microsoft YaHei";
        }
        #m_resetCard {
            background-color: white;
            border-radius: 20px;
        }
        QLineEdit {
            border: none;
            border-bottom: 2px solid #E9ECEF;
            padding: 8px 5px;
            font-size: 14px;
            color: #495057;
        }
        QLineEdit:focus {
            border-bottom-color: #A8E6CF;
        }
        QPushButton#m_resetBtn {
            background-color: #A8E6CF;
            color: white;
            border: none;
            border-radius: 10px;
            height: 45px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton#m_resetBtn:hover {
            background-color: #98D8BF;
        }
        QPushButton#m_getCodeBtn {
            background-color: transparent;
            color: #A8E6CF;
            border: 1px solid #A8E6CF;
            border-radius: 5px;
            padding: 5px 10px;
            font-size: 12px;
        }
        QPushButton#m_getCodeBtn:hover {
            background-color: #F0FFF4;
        }
        QPushButton#m_backBtn {
            border: none;
            color: #888888;
            font-size: 12px;
            background: transparent;
        }
        QPushButton#m_backBtn:hover {
            color: #A8E6CF;
        }
    )");
}

void ForgetPwdWidget::onResetBtnClicked() {
    QString account = m_accountEdit->text().trimmed();
    QString code = m_codeEdit->text().trimmed();
    QString newPwd = m_newPwdEdit->text().trimmed();
    QString confirmPwd = m_confirmPwdEdit->text().trimmed();

    if (account.isEmpty() || code.isEmpty() || newPwd.isEmpty() || confirmPwd.isEmpty()) {
        m_tipLabel->setText("请填写完整信息！");
        return;
    }

    if (newPwd != confirmPwd) {
        m_tipLabel->setText("两次输入的密码不一致！");
        return;
    }

    if (!m_userManager->checkPasswordStrength(newPwd)) {
        m_tipLabel->setText("密码强度不够！");
        return;
    }

    if (m_userManager->resetPassword(account, code, newPwd)) {
        QMessageBox::information(this, "成功", "密码重置成功，请重新登录！");
        emit backToLogin();
        close();
    } else {
        m_tipLabel->setText("重置失败，验证码错误或账号不存在！");
    }
}

void ForgetPwdWidget::onGetCodeBtnClicked() {
    QString account = m_accountEdit->text().trimmed();
    if (account.isEmpty()) {
        m_tipLabel->setText("请输入手机号或邮箱！");
        return;
    }

    // 模拟发送验证码
    QMessageBox::information(this, "提示", "验证码已发送（模拟）：123456");
    
    m_getCodeBtn->setEnabled(false);
    m_countdownSeconds = 60;
    m_countdownTimer->start(1000);
}

void ForgetPwdWidget::updateCountdown() {
    m_countdownSeconds--;
    if (m_countdownSeconds <= 0) {
        m_countdownTimer->stop();
        m_getCodeBtn->setEnabled(true);
        m_getCodeBtn->setText("获取验证码");
    } else {
        m_getCodeBtn->setText(QString("%1s后重新获取").arg(m_countdownSeconds));
    }
}

void ForgetPwdWidget::onBackBtnClicked() {
    emit backToLogin();
    close();
}
