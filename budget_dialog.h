#ifndef BUDGET_DIALOG_H
#define BUDGET_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDoubleValidator>
#include "budget_manager.h"

class BudgetDialog : public QDialog {
    Q_OBJECT
public:
    explicit BudgetDialog(int userId, QWidget *parent = nullptr) : QDialog(parent), m_userId(userId) {
        setWindowTitle("预算设置");
        setFixedSize(300, 250);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        BudgetInfo info = BudgetManager::getInstance()->getBudget(m_userId);

        // 日预算
        QHBoxLayout *dailyLayout = new QHBoxLayout();
        dailyLayout->addWidget(new QLabel("日预算: ¥"));
        m_dailyEdit = new QLineEdit();
        m_dailyEdit->setValidator(new QDoubleValidator(0, 1000000, 2, this));
        m_dailyEdit->setText(QString::number(info.daily, 'f', 2));
        dailyLayout->addWidget(m_dailyEdit);
        mainLayout->addLayout(dailyLayout);

        // 月预算
        QHBoxLayout *monthlyLayout = new QHBoxLayout();
        monthlyLayout->addWidget(new QLabel("月预算: ¥"));
        m_monthlyEdit = new QLineEdit();
        m_monthlyEdit->setValidator(new QDoubleValidator(0, 10000000, 2, this));
        m_monthlyEdit->setText(QString::number(info.monthly, 'f', 2));
        monthlyLayout->addWidget(m_monthlyEdit);
        mainLayout->addLayout(monthlyLayout);

        // 年预算
        QHBoxLayout *yearlyLayout = new QHBoxLayout();
        yearlyLayout->addWidget(new QLabel("年预算: ¥"));
        m_yearlyEdit = new QLineEdit();
        m_yearlyEdit->setValidator(new QDoubleValidator(0, 100000000, 2, this));
        m_yearlyEdit->setText(QString::number(info.yearly, 'f', 2));
        yearlyLayout->addWidget(m_yearlyEdit);
        mainLayout->addLayout(yearlyLayout);

        mainLayout->addStretch();

        // 按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *cancelBtn = new QPushButton("取消");
        QPushButton *saveBtn = new QPushButton("保存");
        saveBtn->setStyleSheet("background-color: #007AFF; color: white; border-radius: 5px; padding: 5px 15px;");
        
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(saveBtn, &QPushButton::clicked, this, &BudgetDialog::onSaveClicked);
        
        btnLayout->addStretch();
        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(saveBtn);
        mainLayout->addLayout(btnLayout);
    }

private slots:
    void onSaveClicked() {
        double daily = m_dailyEdit->text().toDouble();
        double monthly = m_monthlyEdit->text().toDouble();
        double yearly = m_yearlyEdit->text().toDouble();
        
        if (BudgetManager::getInstance()->setBudget(m_userId, daily, monthly, yearly)) {
            accept();
        } else {
            // 这里可以加个错误提示
            reject();
        }
    }

private:
    int m_userId;
    QLineEdit *m_dailyEdit;
    QLineEdit *m_monthlyEdit;
    QLineEdit *m_yearlyEdit;
};

#endif // BUDGET_DIALOG_H
