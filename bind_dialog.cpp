#include "bind_dialog.h"
#include <QMessageBox>

BindDialog::BindDialog(const QString& title, const QString& currentVal, QWidget *parent)
    : QDialog(parent), m_title(title)
{
    setWindowTitle("修改" + title);
    setFixedSize(350, 300);
    setStyleSheet("background-color: white;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);

    m_currentLabel = new QLabel("当前" + title + ": " + currentVal);
    m_currentLabel->setStyleSheet("color: #666; font-size: 13px;");

    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("请输入新" + title);
    m_inputEdit->setStyleSheet("QLineEdit { border: 1px solid #DDD; border-radius: 5px; padding: 8px; }");

    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit();
    m_codeEdit->setPlaceholderText("验证码");
    m_codeEdit->setStyleSheet("QLineEdit { border: 1px solid #DDD; border-radius: 5px; padding: 8px; }");
    
    m_sendBtn = new QPushButton("获取验证码");
    m_sendBtn->setFixedSize(100, 35);
    m_sendBtn->setStyleSheet("QPushButton { background: #F0F0F0; border: 1px solid #DDD; border-radius: 5px; font-size: 12px; }");
    connect(m_sendBtn, &QPushButton::clicked, this, &BindDialog::onSendCode);

    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_sendBtn);

    QPushButton *confirmBtn = new QPushButton("确定修改");
    confirmBtn->setFixedHeight(40);
    confirmBtn->setStyleSheet("QPushButton { background: #007AFF; color: white; border-radius: 5px; font-weight: bold; }");
    connect(confirmBtn, &QPushButton::clicked, this, &BindDialog::onConfirm);

    layout->addWidget(m_currentLabel);
    layout->addWidget(m_inputEdit);
    layout->addLayout(codeLayout);
    layout->addStretch();
    layout->addWidget(confirmBtn);
}

void BindDialog::onSendCode() {
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入新" + m_title);
        return;
    }
    QMessageBox::information(this, "提示", "验证码已发送（测试码：123456）");
    m_sendBtn->setEnabled(false);
    m_sendBtn->setText("已发送");
}

void BindDialog::onConfirm() {
    if (m_inputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "内容不能为空");
        return;
    }
    if (m_codeEdit->text() != "123456") {
        QMessageBox::warning(this, "提示", "验证码错误");
        return;
    }
    accept();
}
