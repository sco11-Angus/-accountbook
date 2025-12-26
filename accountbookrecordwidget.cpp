#include "accountbookrecordwidget.h"
#include "business_logic.h"
#include "account_record.h"
#include "account_manager.h"
#include "user_manager.h"
#include <QMessageBox>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QRegularExpressionMatch>
#include <cmath>


AccountBookRecordWidget::AccountBookRecordWidget(QWidget *parent)
    : QWidget(parent),
    m_plusMode(PlusMode::Add),
    m_minusMode(MinusMode::Sub),
    m_currentOp(Op::None),
    m_phase(InputPhase::EnteringFirst),
    m_firstOperandText(""),
    m_secondOperandText("")
{
    setFixedSize(450, 650);
    initUI();
    initStyleSheet();

    // 获取当前登录用户ID
    UserManager userManager;
    m_currentUserId = userManager.getCurrentUser().getId();
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

    // 绑定点击事件，记录选中分类
    connect(btn, &QPushButton::clicked, this, [=](){
        m_selectedCategory = text;
        // 标记收支类型（根据当前标签页）
        m_isExpense = (m_tabWidget->currentIndex() == 0);
    });
    return btn;
}

QLineEdit* AccountBookRecordWidget::getCurrentAmountEdit()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex == 0) return m_expenseAmountEdit;      // 支出页
    else if (currentIndex == 1) return m_incomeAmountEdit;  // 收入页
    else return nullptr;  // 移除转账金额编辑框返回
}

void AccountBookRecordWidget::onNumberClicked(const QString& number)
{
    QLineEdit *edit = getCurrentAmountEdit();
    if (!edit) return;

    // 根据输入阶段处理
    if (m_phase == InputPhase::ResultShown) {
        // 结果显示后输入新数字：重置为新的计算
        m_phase = InputPhase::EnteringFirst;
        m_firstOperandText = number;
        m_currentOp = Op::None;
    } else if (m_phase == InputPhase::EnteringFirst) {
        // 初始状态（0或空）直接替换，否则拼接
        if (m_firstOperandText == "0" || m_firstOperandText.isEmpty()) {
            m_firstOperandText = number;
        } else {
            m_firstOperandText += number;
        }
    } else if (m_phase == InputPhase::OperatorChosen) {
        // 选择运算符后：开始输入第二个数
        m_phase = InputPhase::EnteringSecond;
        m_secondOperandText = number;
    } else if (m_phase == InputPhase::EnteringSecond) {
        // 拼接第二个数
        m_secondOperandText += number;
    }

    updateAmountDisplay();
}

void AccountBookRecordWidget::onDotClicked()
{
    QLineEdit *edit = getCurrentAmountEdit();
    if (!edit) return;

    QString* currentText;
    if (m_phase == InputPhase::EnteringSecond) {
        currentText = &m_secondOperandText;
    } else {
        currentText = &m_firstOperandText;
    }

    if (currentText->contains(".")) return; // 已存在小数点则忽略

    // 空值时补0再加点，否则直接加点
    if (currentText->isEmpty() || *currentText == "0") {
        *currentText = "0.";
    } else {
        *currentText += ".";
    }

    updateAmountDisplay();
}

QString AccountBookRecordWidget::opToString(Op op) const {
    switch (op) {
    case Op::Add: return "+";
    case Op::Sub: return "-";
    case Op::Mul: return "×";
    case Op::Div: return "÷";
    default: return "";
    }
}

void AccountBookRecordWidget::onDeleteClicked()
{
    QLineEdit *edit = getCurrentAmountEdit();
    if (!edit) return;

    QString* currentText;
    bool isSecond = false;
    if (m_phase == InputPhase::EnteringSecond) {
        currentText = &m_secondOperandText;
        isSecond = true;
    } else {
        currentText = &m_firstOperandText;
    }

    if (currentText->isEmpty()) {
        // 空值时重置为0
        *currentText = "0";
        updateAmountDisplay();
        return;
    }

    // 删除最后一个字符
    currentText->chop(1);

    // 删空后重置为0
    if (currentText->isEmpty()) {
        *currentText = "0";
        // 第二个数删空后回到运算符选择阶段
        if (isSecond) {
            m_phase = InputPhase::OperatorChosen;
        }
    }

    updateAmountDisplay();
}

void AccountBookRecordWidget::onPlusGroupClicked()
{
    // 切换 + / × 模式
    m_plusMode = (m_plusMode == PlusMode::Add) ? PlusMode::Mul : PlusMode::Add;
    if (m_plusBtn) m_plusBtn->setText(m_plusMode == PlusMode::Add ? "+" : "×");

    // 更新当前运算符
    Op newOp = (m_plusMode == PlusMode::Add) ? Op::Add : Op::Mul;

    if (m_phase == InputPhase::EnteringFirst) {
        if (m_firstOperandText.isEmpty()) m_firstOperandText = "0";
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    } else if (m_phase == InputPhase::ResultShown) {
        // 用当前结果作为第一个操作数
        m_secondOperandText.clear();
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    } else if (m_phase == InputPhase::EnteringSecond) {
        // 先计算当前结果再切换运算符
        calculateIfPossible();
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    }

    updateAmountDisplay();
}

void AccountBookRecordWidget::onMinusGroupClicked()
{
    // 切换 - / ÷ 模式
    m_minusMode = (m_minusMode == MinusMode::Sub) ? MinusMode::Div : MinusMode::Sub;
    if (m_minusBtn) m_minusBtn->setText(m_minusMode == MinusMode::Sub ? "-" : "÷");

    // 更新当前运算符
    Op newOp = (m_minusMode == MinusMode::Sub) ? Op::Sub : Op::Div;

    if (m_phase == InputPhase::EnteringFirst) {
        if (m_firstOperandText.isEmpty()) m_firstOperandText = "0";
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    } else if (m_phase == InputPhase::ResultShown) {
        // 用当前结果作为第一个操作数
        m_secondOperandText.clear();
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    } else if (m_phase == InputPhase::EnteringSecond) {
        // 先计算当前结果再切换运算符
        calculateIfPossible();
        m_currentOp = newOp;
        m_phase = InputPhase::OperatorChosen;
    }

    updateAmountDisplay();
}

void AccountBookRecordWidget::updateAmountDisplay()
{
    QLineEdit *edit = getCurrentAmountEdit();
    if (!edit) return;

    QString current;
    if (m_phase == InputPhase::EnteringSecond) {
        current = m_secondOperandText;
    } else {
        current = m_firstOperandText;
    }

    // 空值时显示0
    if (current.isEmpty()) {
        current = "0";
    }

    edit->setText("¥" + current);
}

void AccountBookRecordWidget::calculateIfPossible()
{
    if (m_currentOp == Op::None) return;
    if (m_phase != InputPhase::EnteringSecond || m_secondOperandText.isEmpty()) return;

    bool ok1 = false, ok2 = false;
    double a = m_firstOperandText.toDouble(&ok1);
    double b = m_secondOperandText.toDouble(&ok2);
    if (!ok1 || !ok2) return;

    double result = 0.0;
    switch (m_currentOp) {
    case Op::Add: result = a + b; break;
    case Op::Sub: result = a - b; break;
    case Op::Mul: result = a * b; break;
    case Op::Div:
        if (b == 0.0) return;
        result = a / b;
        break;
    default: return;
    }

    // 格式化结果为两位小数
    m_firstOperandText = QString::number(result, 'f', 2);
    // 移除末尾多余的0
    if (m_firstOperandText.contains(".")) {
        m_firstOperandText.remove(QRegularExpression("0+$"));
        if (m_firstOperandText.endsWith(".")) {
            m_firstOperandText += "00";
        }
    }

    m_secondOperandText.clear();
    m_currentOp = Op::None;
    m_phase = InputPhase::ResultShown;

    updateAmountDisplay();
}

void AccountBookRecordWidget::onEqualClicked()
{
    // 按下等号时尝试计算，结果会显示在输入框中
    calculateIfPossible();
}

void AccountBookRecordWidget::createKeyboard()
{
    m_keyboardWidget = new QWidget();
    QGridLayout *keyLayout = new QGridLayout(m_keyboardWidget);
    keyLayout->setSpacing(5);

    // 创建数字键
    QPushButton *btn1 = new QPushButton("1");
    QPushButton *btn2 = new QPushButton("2");
    QPushButton *btn3 = new QPushButton("3");
    QPushButton *btn4 = new QPushButton("4");
    QPushButton *btn5 = new QPushButton("5");
    QPushButton *btn6 = new QPushButton("6");
    QPushButton *btn7 = new QPushButton("7");
    QPushButton *btn8 = new QPushButton("8");
    QPushButton *btn9 = new QPushButton("9");
    QPushButton *btn0 = new QPushButton("0");
    QPushButton *btnDot = new QPushButton(".");

    // 退位键：用退格符号
    m_deleteBtn = new QPushButton("⌫");

    // 使用成员函数连接信号槽（数字键、小数点、退位）
    connect(btn1, &QPushButton::clicked, this, [this]() { onNumberClicked("1"); });
    connect(btn2, &QPushButton::clicked, this, [this]() { onNumberClicked("2"); });
    connect(btn3, &QPushButton::clicked, this, [this]() { onNumberClicked("3"); });
    connect(btn4, &QPushButton::clicked, this, [this]() { onNumberClicked("4"); });
    connect(btn5, &QPushButton::clicked, this, [this]() { onNumberClicked("5"); });
    connect(btn6, &QPushButton::clicked, this, [this]() { onNumberClicked("6"); });
    connect(btn7, &QPushButton::clicked, this, [this]() { onNumberClicked("7"); });
    connect(btn8, &QPushButton::clicked, this, [this]() { onNumberClicked("8"); });
    connect(btn9, &QPushButton::clicked, this, [this]() { onNumberClicked("9"); });
    connect(btn0, &QPushButton::clicked, this, [this]() { onNumberClicked("0"); });
    connect(btnDot, &QPushButton::clicked, this, &AccountBookRecordWidget::onDotClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AccountBookRecordWidget::onDeleteClicked);

    // 布局数字键 + 运算键
    keyLayout->addWidget(btn1, 0, 0);
    keyLayout->addWidget(btn2, 0, 1);
    keyLayout->addWidget(btn3, 0, 2);

    // 加/乘按钮块（成员，用于切换 + / ×）
    m_plusBtn = new QPushButton("+");
    keyLayout->addWidget(m_plusBtn, 0, 3);

    keyLayout->addWidget(btn4, 1, 0);
    keyLayout->addWidget(btn5, 1, 1);
    keyLayout->addWidget(btn6, 1, 2);

    // 减/除按钮块（成员，用于切换 - / ÷）
    m_minusBtn = new QPushButton("-");
    keyLayout->addWidget(m_minusBtn, 1, 3);

    keyLayout->addWidget(btn7, 2, 0);
    keyLayout->addWidget(btn8, 2, 1);
    keyLayout->addWidget(btn9, 2, 2);

    // “保存再记”改成等号键 =
    m_equalBtn = new QPushButton("=");
    keyLayout->addWidget(m_equalBtn, 2, 3);

    keyLayout->addWidget(btnDot, 3, 0);
    keyLayout->addWidget(btn0, 3, 1);
    keyLayout->addWidget(m_deleteBtn, 3, 2);

    m_completeBtn = new QPushButton("完成");
    keyLayout->addWidget(m_completeBtn, 3, 3);

    // 运算按钮点击事件：切换 +/×、-/÷ 并设置当前运算符
    connect(m_plusBtn,  &QPushButton::clicked, this, &AccountBookRecordWidget::onPlusGroupClicked);
    connect(m_minusBtn, &QPushButton::clicked, this, &AccountBookRecordWidget::onMinusGroupClicked);

    // 等号按钮点击事件：只做运算，结果显示在输入框，不关闭窗口
    connect(m_equalBtn, &QPushButton::clicked, this, &AccountBookRecordWidget::onEqualClicked);

    // 键盘按钮样式（确保按钮可点击）
    for (QPushButton *btn : m_keyboardWidget->findChildren<QPushButton*>()) {
        btn->setFixedHeight(40);
        btn->setEnabled(true);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(255, 255, 255, 0.8);
                border-radius: 10px;
                border: none;
            }
            QPushButton:hover {
                background-color: rgba(255, 255, 255, 0.9);
            }
            QPushButton:pressed {
                background-color: rgba(200, 200, 200, 0.8);
            }
        )");
    }

    // 完成按钮点击事件
    connect(m_completeBtn, &QPushButton::clicked, this, [=](){
        // 1. 提取金额（去除“¥”，转换为double，支出为负）
        QLineEdit *edit = getCurrentAmountEdit();
        QString text = edit->text().remove("¥");
        bool ok;
        double amount = text.toDouble(&ok);
        if (!ok || amount == 0) {
            QMessageBox::warning(this, "错误", "请输入有效的金额（非0）");
            return;
        }
        // 支出金额转为负数
        if (m_isExpense) amount = -amount;

        // 2. 业务校验（分类、用户ID、金额）
        BusinessLogic logic;
        AccountRecord record;
        record.setUserId(m_currentUserId);
        record.setAmount(amount);
        record.setType(m_selectedCategory);
        record.setCreateTime(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        record.setModifyTime(record.getCreateTime());

        if (!logic.validateBillRecord(record)) {
            QMessageBox::warning(this, "错误", logic.getValidationError());
            return;
        }

        // 3. 保存到数据库（调用AccountManager）
        AccountManager accountManager;
        if (accountManager.addAccountRecord(record)) {
            QMessageBox::information(this, "成功", "记账记录已保存");
            emit billRecorded(); // 通知主界面更新
            this->close();
        } else {
            QMessageBox::critical(this, "失败", "保存失败：" + SqliteHelper::getInstance()->getLastError());
        }
    });
}

void AccountBookRecordWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // 顶部：取消
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(new QPushButton("取消"));
    topBar->addStretch();
    mainLayout->addLayout(topBar);

    // 支出/收入/转账标签页
    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(m_expensePage = new QWidget(), "支出");
    m_tabWidget->addTab(m_incomePage = new QWidget(), "收入");
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
    m_expenseAmountEdit = new QLineEdit("¥0");  // 原"¥0.00"
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
    m_incomeAmountEdit = new QLineEdit("¥0");
    m_incomeAmountEdit->setStyleSheet("color: #4CAF50; font-size: 24px; font-weight: bold;");
    incomeLayout->addWidget(m_incomeAmountEdit);

    // 功能按钮
    QHBoxLayout *incomeFunc = new QHBoxLayout();
    incomeFunc->addWidget(new QPushButton("选择账户"));
    incomeFunc->addWidget(new QPushButton("图片"));
    incomeLayout->addLayout(incomeFunc);

    // 金额
    m_incomeAmountEdit = new QLineEdit("¥0.00");
    m_incomeAmountEdit->setStyleSheet("color: #4CAF50; font-size: 24px; font-weight: bold;");
    incomeLayout->addWidget(m_incomeAmountEdit);

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
