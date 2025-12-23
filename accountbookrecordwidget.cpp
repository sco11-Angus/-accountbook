#include "accountbookrecordWidget.h"
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>

AccountBookRecordWidget::AccountBookRecordWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(450, 650);
    initUI();
    initStyleSheet();
}

QPushButton* AccountBookRecordWidget::createCateBtn(const QString& text, const QString& color)
{
    QPushButton *btn = new QPushButton(text);
    btn->setFixedSize(60, 60);
    btn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 30px;
            border: none;
            font-size: 12px;
        }
        QPushButton:checked {
            background-color: %1;
            color: white;
        }
    )").arg(color));
    btn->setCheckable(true);
    return btn;
}

void AccountBookRecordWidget::createKeyboard()
{
    m_keyboardWidget = new QWidget();
    QGridLayout *keyLayout = new QGridLayout(m_keyboardWidget);
    keyLayout->setSpacing(5);

    // 数字键
    keyLayout->addWidget(new QPushButton("1"), 0, 0);
    keyLayout->addWidget(new QPushButton("2"), 0, 1);
    keyLayout->addWidget(new QPushButton("3"), 0, 2);
    keyLayout->addWidget(new QPushButton("+"), 0, 3);

    keyLayout->addWidget(new QPushButton("4"), 1, 0);
    keyLayout->addWidget(new QPushButton("5"), 1, 1);
    keyLayout->addWidget(new QPushButton("6"), 1, 2);
    keyLayout->addWidget(new QPushButton("-"), 1, 3);

    keyLayout->addWidget(new QPushButton("7"), 2, 0);
    keyLayout->addWidget(new QPushButton("8"), 2, 1);
    keyLayout->addWidget(new QPushButton("9"), 2, 2);
    keyLayout->addWidget(new QPushButton("保存再记"), 2, 3);

    keyLayout->addWidget(new QPushButton("."), 3, 0);
    keyLayout->addWidget(new QPushButton("0"), 3, 1);
    keyLayout->addWidget(new QPushButton("×"), 3, 2);
    keyLayout->addWidget(m_completeBtn = new QPushButton("完成"), 3, 3);

    // 键盘按钮样式
    for (QPushButton *btn : m_keyboardWidget->findChildren<QPushButton*>()) {
        btn->setFixedHeight(40);
        btn->setStyleSheet("background-color: rgba(255, 255, 255, 0.8); border-radius: 10px;");
    }
    // AccountBookRecordWidget.cpp 中，createKeyboard函数里绑定
    connect(m_completeBtn, &QPushButton::clicked, this, [=](){
        // 1. 先将记账数据写入数据库（省略数据库操作代码）
        // 2. 发送记账完成信号
        emit billRecorded();
        // 3. 关闭记账界面
        this->close();
    });
}

void AccountBookRecordWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // 顶部：取消 + 账本
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(new QPushButton("取消"));
    topBar->addStretch();
    topBar->addWidget(new QLabel("默认账本"));
    mainLayout->addLayout(topBar);

    // 支出/收入/转账标签页
    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(m_expensePage = new QWidget(), "支出");
    m_tabWidget->addTab(m_incomePage = new QWidget(), "收入");
    m_tabWidget->addTab(m_transferPage = new QWidget(), "转账");
    mainLayout->addWidget(m_tabWidget);

    // ========== 支出页 ==========
    QVBoxLayout *expenseLayout = new QVBoxLayout(m_expensePage);
    m_expenseCateLayout = new QGridLayout();
    // 支出分类（你指定的列表）
    QStringList expenseCates = {"餐饮", "服饰", "日用", "数码", "美妆护肤",
                                "应用软件", "住房", "交通", "娱乐", "医疗",
                                "学习", "办公", "运动", "社交", "宠物", "旅行", "其他"};
    for (int i=0; i<expenseCates.size(); i++) {
        m_expenseCateLayout->addWidget(createCateBtn(expenseCates[i], "#FF6B6B"), i/5, i%5);
    }
    expenseLayout->addLayout(m_expenseCateLayout);

    // 功能按钮
    QHBoxLayout *expenseFunc = new QHBoxLayout();
    expenseFunc->addWidget(new QPushButton("选择账户"));
    expenseFunc->addWidget(new QPushButton("报销"));
    expenseFunc->addWidget(new QPushButton("优惠"));
    expenseLayout->addLayout(expenseFunc);

    // 金额
    m_expenseAmountEdit = new QLineEdit("¥0.00");
    m_expenseAmountEdit->setStyleSheet("color: #FF6B6B; font-size: 24px; font-weight: bold;");
    expenseLayout->addWidget(m_expenseAmountEdit);

    // ========== 收入页 ==========
    QVBoxLayout *incomeLayout = new QVBoxLayout(m_incomePage);
    m_incomeCateLayout = new QGridLayout();
    // 收入分类（你指定的列表）
    QStringList incomeCates = {"工资", "奖金", "福利", "红包", "兼职", "副业", "投资", "其他"};
    for (int i=0; i<incomeCates.size(); i++) {
        m_incomeCateLayout->addWidget(createCateBtn(incomeCates[i], "#4CAF50"), i/5, i%5);
    }
    incomeLayout->addLayout(m_incomeCateLayout);

    // 功能按钮
    QHBoxLayout *incomeFunc = new QHBoxLayout();
    incomeFunc->addWidget(new QPushButton("选择账户"));
    incomeFunc->addWidget(new QPushButton("图片"));
    incomeLayout->addLayout(incomeFunc);

    // 金额
    m_incomeAmountEdit = new QLineEdit("¥0.00");
    m_incomeAmountEdit->setStyleSheet("color: #4CAF50; font-size: 24px; font-weight: bold;");
    incomeLayout->addWidget(m_incomeAmountEdit);

    // ========== 转账页 ==========
    QVBoxLayout *transferLayout = new QVBoxLayout(m_transferPage);
    m_outAccountCombo = new QComboBox();
    m_outAccountCombo->addItems({"请选择转出账户", "微信", "支付宝", "银行卡"});
    transferLayout->addWidget(m_outAccountCombo);
    transferLayout->addWidget(new QLabel("↘")); // 转账箭头
    m_inAccountCombo = new QComboBox();
    m_inAccountCombo->addItems({"请选择转入账户", "微信", "支付宝", "银行卡"});
    transferLayout->addWidget(m_inAccountCombo);

    // 功能按钮
    QHBoxLayout *transferFunc = new QHBoxLayout();
    transferFunc->addWidget(new QPushButton("优惠"));
    transferFunc->addWidget(new QPushButton("手续费"));
    transferLayout->addLayout(transferFunc);

    // 金额
    m_transferAmountEdit = new QLineEdit("¥0.00");
    m_transferAmountEdit->setStyleSheet("color: #FFD700; font-size: 24px; font-weight: bold;");
    transferLayout->addWidget(m_transferAmountEdit);

    // 数字键盘
    createKeyboard();
    mainLayout->addWidget(m_keyboardWidget);
}

void AccountBookRecordWidget::initStyleSheet()
{
    setStyleSheet(R"(
        AccountBookRecordWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #FFE4E1);
        }
        QTabWidget::tab-bar { alignment: center; }
        QTabBar::tab {
            width: 80px;
            height: 30px;
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 15px;
            margin: 0 5px;
        }
        QTabBar::tab:selected {
            background-color: #FFB6C1;
            color: white;
        }
        QLineEdit { background-color: transparent; border: none; }
        QComboBox {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 15px;
            padding: 5px 10px;
            border: none;
        }
        QPushButton#m_completeBtn {
            background-color: #FFB6C1;
            color: white;
            font-weight: bold;
        }
    )");
}
