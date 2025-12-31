#include "accountbookrecordwidget.h"
#include "bill_service.h"
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QRegularExpressionMatch>
#include <cmath>
#include <QFile>
#include <QButtonGroup>
#include <QLabel>
#include <QDateTime>
#include <QCalendarWidget>
#include <QTimeEdit>
#include <QSpinBox>
#include <QDialog>
#include <QPushButton>
#include <QMessageBox>

#include <QIcon>


AccountBookRecordWidget::AccountBookRecordWidget(QWidget *parent)
    : QWidget(parent),
    m_plusMode(PlusMode::Add),
    m_minusMode(MinusMode::Sub),
    m_currentOp(Op::None),
    m_phase(InputPhase::EnteringFirst),
    m_firstOperandText(""),
    m_secondOperandText(""),
    m_currentDateTime(QDateTime::currentDateTime())
{
    setFixedSize(450, 650);
    initUI();
    initStyleSheet();
    updateTimeDisplay();
}

QMap<QString, QString> getCateNameMap() {
    QMap<QString, QString> cateMap;
    // 一一对应：中文分类名 → 拼音文件名（无后缀）
    cateMap["餐饮"] = "canyin";
    cateMap["服饰"] = "fushi";
    cateMap["日用"] = "riyong";
    cateMap["数码"] = "shuma";
    cateMap["美妆"] = "meizhuang";
    cateMap["软件"] = "ruanjian";
    cateMap["住房"] = "zhufang";
    cateMap["交通"] = "jiaotong";
    cateMap["娱乐"] = "yule";
    cateMap["医疗"] = "yiliao";
    cateMap["通讯"] = "tongxun";
    cateMap["汽车"] = "qiche";
    cateMap["学习"] = "xuexi";
    cateMap["办公"] = "bangong";
    cateMap["运动"] = "yundong";
    cateMap["社交"] = "shejiao";
    cateMap["宠物"] = "chongwu";
    cateMap["旅行"] = "lvxing";
    cateMap["育儿"] = "yuer";
    cateMap["其他"] = "qita";

    cateMap["副业"] = "fuye";
    cateMap["工资"] = "gongzi";
    cateMap["红包"] = "hongbao";
    cateMap["兼职"] = "jianzhi";
    cateMap["其他"] = "qita";
    cateMap["投资"] = "touzi";
    cateMap["意外收入"] = "yiwaishouru";
    return cateMap;
}


QWidget* AccountBookRecordWidget::createCateBtn(const QString& text, const QString& imgDir)
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0); // 图标和文字的间距
    layout->setAlignment(Qt::AlignCenter); // 整体居中

    QPushButton *btn = new QPushButton();
    btn->setCheckable(true);
    btn->setFixedSize(50, 50);
    btn->setObjectName("cateButton");
    
    // ===== 核心修改：通过映射表获取拼音文件名 =====
    QMap<QString, QString> cateMap = getCateNameMap();
    // 取拼音名，若没找到则默认用"qita（其他）"
    QString pinyinName = cateMap.value(text, "qita");
    // 拼接路径（用拼音名替代原来的中文）
    // 替换原来的 basePath 行，用绝对路径（注意路径里用 / 或 \\）
    QString basePath = QString(":/%1/resources/%2/%3").arg(imgDir).arg(imgDir).arg(pinyinName);
    QString normalPath = basePath + ".jpg";
    QString activePath = basePath + "1.jpg";

    qDebug() << "当前查找的正常图片路径：" << normalPath;
    qDebug() << "该文件是否存在：" << QFile::exists(normalPath);
    qDebug() << "当前查找的选中图片路径：" << activePath;
    qDebug() << "该文件是否存在：" << QFile::exists(activePath);

    // 如果没有1.jpg，则选中时也用原图
    if (!QFile::exists(activePath)) {
        activePath = normalPath;
    }
    
    // 使用 QIcon 管理图片状态和缩放，解决 background-size 不生效导致的显示不全问题
    QIcon icon;
    icon.addFile(normalPath, QSize(), QIcon::Normal, QIcon::Off); // 未选中状态
    icon.addFile(activePath, QSize(), QIcon::Normal, QIcon::On);  // 选中状态
    
    btn->setIcon(icon);
    
    // 根据是支出还是收入，设置不同的图标大小
    // classify1 是支出（按钮多，图标需更小）；classify2 是收入（按钮少，图标可大些）
    if (imgDir == "classify1") {
        btn->setIconSize(QSize(70, 70));
        // 支出：增加上边距让图片上移（相对视觉），同时给文字留出空间
        btn->setStyleSheet(btn->styleSheet() + "QPushButton { padding-bottom: 0px; }");
    } else {
        btn->setIconSize(QSize(70, 70));
    }

    btn->setStyleSheet(QString(R"(
        QPushButton {
            border-radius: 25px; /* 50x50的一半，做成圆形 */
            background-color: #f5f5f5; /* 图2的浅灰背景 */
            border: none;
        }
    )"));
    
    QLabel *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    
    // 支出：文字下移（增加顶部边距）；收入：维持原样
    QString labelStyle = "font-size: 12px; color: #666;";
    if (imgDir == "classify1") {
        // 支出：文字下移，同时减少底部内边距防止被截断
        labelStyle += " margin-top: 2px; margin-bottom: 2px;";
    } else {
        labelStyle += " margin-top: 2px;"; // 收入：维持较小间距
    }
    label->setStyleSheet(labelStyle);
    label->setFixedWidth(60);

    layout->addWidget(btn, 0, Qt::AlignCenter);
    layout->addWidget(label, 0, Qt::AlignCenter);
    
    // 如果是支出界面，强制增加容器高度以容纳下移的文字
    if (imgDir == "classify1") {
        container->setMinimumHeight(80); 
    }
    
    return container;
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
        // 1. 获取当前金额编辑框
        QLineEdit *amountEdit = getCurrentAmountEdit();
        QString text = amountEdit->text().remove("¥");
        bool ok;
        double amount = text.toDouble(&ok);
        
        if (!ok || amount == 0.0) {
            QMessageBox::warning(this, "提示", "请输入有效的金额");
            return;
        }

        // 2. 格式化金额为两位小数
        amountEdit->setText(QString("¥%1").arg(amount, 0, 'f', 2));

        // 3. 获取当前页面（支出0，收入1）
        int currentPage = m_tabWidget->currentIndex();
        
        // 4. 获取选中的分类
        QString category;
        if (currentPage == 0) {
            // 支出页
            int checkedId = m_expenseGroup->checkedId();
            if (checkedId < 0) {
                QMessageBox::warning(this, "提示", "请选择消费分类");
                return;
            }
            QStringList expenseCates = {
                "餐饮", "服饰", "日用", "数码", "美妆",
                "软件", "住房", "交通", "娱乐", "医疗",
                "通讯", "汽车", "学习", "办公", "运动",
                "社交", "宠物", "旅行", "育儿", "其他"
            };
            category = expenseCates[checkedId];
            amount = -amount;  // 支出为负数
            qDebug() << "记账：支出类型" << category << "金额" << amount;
        } else if (currentPage == 1) {
            // 收入页
            int checkedId = m_incomeGroup->checkedId();
            if (checkedId < 0) {
                QMessageBox::warning(this, "提示", "请选择收入分类");
                return;
            }
            QStringList incomeCates = {
                "副业", "工资", "红包", "兼职", "投资",
                "意外收入", "其他"
            };
            category = incomeCates[checkedId];
            // 收入为正数，amount 保持原样
            qDebug() << "记账：收入类型" << category << "金额" << amount;
        } else {
            QMessageBox::warning(this, "提示", "无效的页面");
            return;
        }

        // 5. 获取备注
        QString remark;
        if (currentPage == 0) {
            remark = m_expenseNoteEdit->text();
        } else {
            remark = m_incomeNoteEdit->text();
        }

        // 6. 获取当前登录用户ID
        UserManager* userManager = UserManager::getInstance();
        User currentUser = userManager->getCurrentUser();
        int userId = currentUser.getId();
        
        qDebug() << "当前用户ID：" << userId;
        
        if (userId <= 0) {
            QMessageBox::warning(this, "错误", "获取用户信息失败，请重新登录");
            return;
        }

        // 7. 创建 AccountRecord 对象
        AccountRecord record;
        record.setUserId(userId);
        record.setAmount(amount);
        record.setType(category);
        record.setRemark(remark);
        record.setCreateTime(m_currentDateTime.toString("yyyy-MM-dd HH:mm:ss"));
        record.setModifyTime(m_currentDateTime.toString("yyyy-MM-dd HH:mm:ss"));
        record.setIsDeleted(0);  // 0 表示正常记录

        // 调试信息：打印即将保存的数据
        qDebug() << "====== 准备保存账单 ======";
        qDebug() << "用户ID:" << record.getUserId();
        qDebug() << "金额:" << record.getAmount();
        qDebug() << "分类:" << record.getType();
        qDebug() << "备注:" << record.getRemark();
        qDebug() << "创建时间:" << record.getCreateTime();
        qDebug() << "================";

        // 8. 调用 BillService 保存到数据库（本地+同步）
        // 禁用保存按钮，防止重复提交
        sender()->setProperty("disabled", true);
        if (QPushButton* btn = qobject_cast<QPushButton*>(sender())) {
            btn->setEnabled(false);
        }

        // 监听保存结果信号
        // 注意：这里使用 context 对象 'this'，当 widget 销毁时连接会自动断开
        connect(BillService::getInstance(), &BillService::billSaved, this, [=](bool success, const QString& message) {
            if (success) {
                QMessageBox::information(this, "成功", message);
                emit billRecorded();
                this->close();
            } else {
                QMessageBox::critical(this, "错误", message);
                // 失败了重新启用按钮
                if (QPushButton* btn = qobject_cast<QPushButton*>(sender())) {
                    btn->setEnabled(true);
                }
            }
        });

        bool success = BillService::saveBill(record);
        if (!success) {
            // 如果 saveBill 返回 false，说明本地保存就失败了，上面的 lambda 会处理信号
            // 但为了保险，这里也可以做简单处理
            return;
        }
    });
}

void AccountBookRecordWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // 顶部：取消按钮已移除
    // QHBoxLayout *topBar = new QHBoxLayout();
    // topBar->addWidget(new QPushButton("取消"));
    // topBar->addStretch();
    // mainLayout->addLayout(topBar);

    // 支出/收入/转账标签页
    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(m_expensePage = new QWidget(), "支出");
    m_tabWidget->addTab(m_incomePage = new QWidget(), "收入");
    mainLayout->addWidget(m_tabWidget);

    // ========== 支出页 ==========
    QVBoxLayout *expenseLayout = new QVBoxLayout(m_expensePage);
    m_expenseCateLayout = new QGridLayout();
    // 增加垂直间距，防止文字被下一行遮挡
    m_expenseCateLayout->setVerticalSpacing(20); 
    m_expenseCateLayout->setHorizontalSpacing(15);
    m_expenseCateLayout->setContentsMargins(10, 10, 10, 10);
    
    m_expenseGroup = new QButtonGroup(this);
    m_expenseGroup->setExclusive(true);

    // 支出分类：20个，排成4行5列 (对应 resources/classify1 目录下的图片)
    QStringList expenseCates = {
        "餐饮", "服饰", "日用", "数码", "美妆",
        "软件", "住房", "交通", "娱乐", "医疗",
        "通讯", "汽车", "学习", "办公", "运动",
        "社交", "宠物", "旅行", "育儿", "其他"
    };

    for (int i=0; i<expenseCates.size(); i++) {
        QWidget *cateWidget = createCateBtn(expenseCates[i],"classify1");
        QPushButton *btn = cateWidget->findChild<QPushButton*>("cateButton");
        if (btn) {
            m_expenseGroup->addButton(btn, i);
        }
        m_expenseCateLayout->addWidget(cateWidget, i/5, i%5);
    }
    expenseLayout->addLayout(m_expenseCateLayout);

    // 功能按钮已移除
    // QHBoxLayout *expenseFunc = new QHBoxLayout();
    // expenseFunc->addWidget(new QPushButton("选择账户"));
    // expenseFunc->addWidget(new QPushButton("报销"));
    // expenseFunc->addWidget(new QPushButton("优惠"));
    // expenseLayout->addLayout(expenseFunc);

    // 金额
    m_expenseAmountEdit = new QLineEdit("¥0");  // 原"¥0.00"
    m_expenseAmountEdit->setStyleSheet("color: #FF6B6B; font-size: 24px; font-weight: bold;");
    expenseLayout->addWidget(m_expenseAmountEdit);
    
    // 时间和备注行
    QHBoxLayout *expenseTimeNoteLayout = new QHBoxLayout();
    // 时间显示（可点击）
    m_expenseTimeLabel = new QLabel();
    m_expenseTimeLabel->setStyleSheet("color: #666; font-size: 12px; padding: 5px; background-color: transparent;");
    m_expenseTimeLabel->setCursor(Qt::PointingHandCursor);
    m_expenseTimeLabel->installEventFilter(this);
    // 使用鼠标点击事件
    m_expenseTimeLabel->installEventFilter(this);
    expenseTimeNoteLayout->addWidget(m_expenseTimeLabel);
    
    // 备注输入框
    m_expenseNoteEdit = new QLineEdit();
    m_expenseNoteEdit->setPlaceholderText("点击填写备注");
    m_expenseNoteEdit->setStyleSheet("color: #999; font-size: 12px; background-color: transparent; border: none;");
    expenseTimeNoteLayout->addWidget(m_expenseNoteEdit, 1);
    expenseLayout->addLayout(expenseTimeNoteLayout);

    // ========== 收入页 ==========
    QVBoxLayout *incomeLayout = new QVBoxLayout(m_incomePage);
    m_incomeCateLayout = new QGridLayout();
    m_incomeCateLayout->setSpacing(10);

    m_incomeGroup = new QButtonGroup(this);
    m_incomeGroup->setExclusive(true);

    // 收入分类（使用有图片的类别）
    QStringList incomeCates = {
        "副业", "工资", "红包", "兼职", "投资",
        "意外收入", "其他"
    };
    for (int i=0; i<incomeCates.size(); i++) {
        QWidget *cateWidget = createCateBtn(incomeCates[i],"classify2");
        QPushButton *btn = cateWidget->findChild<QPushButton*>("cateButton");
        if (btn) m_incomeGroup->addButton(btn, i);
        m_incomeCateLayout->addWidget(cateWidget, i/5, i%5);
    }
    incomeLayout->addLayout(m_incomeCateLayout);

    // 功能按钮已移除
    // QHBoxLayout *incomeFunc = new QHBoxLayout();
    // incomeFunc->addWidget(new QPushButton("选择账户"));
    // incomeFunc->addWidget(new QPushButton("图片"));
    // incomeLayout->addLayout(incomeFunc);

    // 金额
    m_incomeAmountEdit = new QLineEdit("¥0");
    m_incomeAmountEdit->setStyleSheet("color: #4CAF50; font-size: 24px; font-weight: bold;");
    incomeLayout->addWidget(m_incomeAmountEdit);
    
    // 时间和备注行
    QHBoxLayout *incomeTimeNoteLayout = new QHBoxLayout();
    // 时间显示（可点击）
    m_incomeTimeLabel = new QLabel();
    m_incomeTimeLabel->setStyleSheet("color: #666; font-size: 12px; padding: 5px; background-color: transparent;");
    m_incomeTimeLabel->setCursor(Qt::PointingHandCursor);
    m_incomeTimeLabel->installEventFilter(this);
    incomeTimeNoteLayout->addWidget(m_incomeTimeLabel);
    
    // 备注输入框
    m_incomeNoteEdit = new QLineEdit();
    m_incomeNoteEdit->setPlaceholderText("点击填写备注");
    m_incomeNoteEdit->setStyleSheet("color: #999; font-size: 12px; background-color: transparent; border: none;");
    incomeTimeNoteLayout->addWidget(m_incomeNoteEdit, 1);
    incomeLayout->addLayout(incomeTimeNoteLayout);

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

bool AccountBookRecordWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == m_expenseTimeLabel || obj == m_incomeTimeLabel) {
            onTimeClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AccountBookRecordWidget::updateTimeDisplay()
{
    QString timeStr = m_currentDateTime.toString("HH:mm");
    
    // 设置时间显示，使用时钟图标（Unicode字符）
    QString displayText = QString("🕐 %1").arg(timeStr);
    if (m_expenseTimeLabel) {
        m_expenseTimeLabel->setText(displayText);
    }
    if (m_incomeTimeLabel) {
        m_incomeTimeLabel->setText(displayText);
    }
}

void AccountBookRecordWidget::onTimeClicked()
{
    qDebug() << "时间选择按钮被点击";
    showDateTimePicker();
}

void AccountBookRecordWidget::showDateTimePicker()
{
    DateTimePickerDialog dialog(this);
    dialog.setDateTime(m_currentDateTime);
    if (dialog.exec() == QDialog::Accepted) {
        m_currentDateTime = dialog.getDateTime();
        updateTimeDisplay();
    }
}

QLabel* AccountBookRecordWidget::getCurrentTimeLabel()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex == 0) return m_expenseTimeLabel;
    else if (currentIndex == 1) return m_incomeTimeLabel;
    return nullptr;
}

QLineEdit* AccountBookRecordWidget::getCurrentNoteEdit()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex == 0) return m_expenseNoteEdit;
    else if (currentIndex == 1) return m_incomeNoteEdit;
    return nullptr;
}

// ========== DateTimePickerDialog 实现 ==========

DateTimePickerDialog::DateTimePickerDialog(QWidget *parent)
    : QDialog(parent), m_dateTime(QDateTime::currentDateTime())
{
    setWindowTitle("选择日期时间");
    setFixedSize(400, 550);
    setStyleSheet(R"(
        QDialog {
            background-color: white;
        }
        QPushButton {
            background-color: #f0f0f0;
            border: none;
            border-radius: 5px;
            padding: 5px 10px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
        }
        QPushButton#yearMonthBtn {
            background-color: transparent;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton#timeBtn {
            background-color: #4CAF50;
            color: white;
        }
        QCalendarWidget {
            background-color: white;
        }
        QCalendarWidget QTableView {
            selection-background-color: #4CAF50;
        }
        QSpinBox {
            padding: 5px;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 14px;
        }
        QTimeEdit {
            padding: 5px;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 16px;
        }
    )");
    initUI();
}

void DateTimePickerDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 年月选择行
    QHBoxLayout *yearMonthLayout = new QHBoxLayout();
    m_prevMonthBtn = new QPushButton("◀");
    m_yearMonthBtn = new QPushButton();
    m_yearMonthBtn->setObjectName("yearMonthBtn");
    m_nextMonthBtn = new QPushButton("▶");
    
    yearMonthLayout->addWidget(m_prevMonthBtn);
    yearMonthLayout->addWidget(m_yearMonthBtn, 1);
    yearMonthLayout->addWidget(m_nextMonthBtn);
    
    connect(m_prevMonthBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onPrevMonth);
    connect(m_nextMonthBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onNextMonth);
    connect(m_yearMonthBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onYearMonthClicked);
    
    mainLayout->addLayout(yearMonthLayout);
    
    // 日历
    m_calendar = new QCalendarWidget();
    m_calendar->setGridVisible(true);
    // 连接日历选择日期信号，更新内部日期时间
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, [this]() {
        QDate selectedDate = m_calendar->selectedDate();
        if (selectedDate.isValid()) {
            m_dateTime = QDateTime(selectedDate, m_dateTime.time());
            updateDisplay();
        }
    });
    mainLayout->addWidget(m_calendar);
    
    // 时间选择行
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->addWidget(new QLabel("时间:"));
    m_timeLabel = new QLabel();
    m_timeBtn = new QPushButton();
    m_timeBtn->setText("选择时间");
    m_timeBtn->setObjectName("timeBtn");
    timeLayout->addWidget(m_timeLabel, 1);
    timeLayout->addWidget(m_timeBtn);
    
    connect(m_timeBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onTimeClicked);
    
    mainLayout->addLayout(timeLayout);
    
    // 按钮行
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消");
    QPushButton *okBtn = new QPushButton("确定");
    okBtn->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px 20px; border-radius: 5px;");
    cancelBtn->setStyleSheet("background-color: #ccc; color: white; padding: 8px 20px; border-radius: 5px;");
    
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);
    
    // 初始化年月选择器对话框
    m_yearMonthDialog = new QDialog(this);
    m_yearMonthDialog->setWindowTitle("选择年月");
    m_yearMonthDialog->setFixedSize(300, 200);
    QVBoxLayout *ymLayout = new QVBoxLayout(m_yearMonthDialog);
    QHBoxLayout *ymInputLayout = new QHBoxLayout();
    m_yearSpinBox = new QSpinBox();
    m_yearSpinBox->setRange(2000, 2100);
    m_yearSpinBox->setSuffix("年");
    m_monthSpinBox = new QSpinBox();
    m_monthSpinBox->setRange(1, 12);
    m_monthSpinBox->setSuffix("月");
    ymInputLayout->addWidget(m_yearSpinBox);
    ymInputLayout->addWidget(m_monthSpinBox);
    ymLayout->addLayout(ymInputLayout);
    QHBoxLayout *ymBtnLayout = new QHBoxLayout();
    QPushButton *ymCancelBtn = new QPushButton("取消");
    QPushButton *ymOkBtn = new QPushButton("确定");
    connect(ymCancelBtn, &QPushButton::clicked, m_yearMonthDialog, &QDialog::reject);
    connect(ymOkBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onYearMonthSelected);
    ymBtnLayout->addWidget(ymCancelBtn);
    ymBtnLayout->addWidget(ymOkBtn);
    ymLayout->addLayout(ymBtnLayout);
    
    // 初始化时间选择器对话框
    m_timeDialog = new QDialog(this);
    m_timeDialog->setWindowTitle("选择时间");
    m_timeDialog->setFixedSize(250, 150);
    QVBoxLayout *timeDialogLayout = new QVBoxLayout(m_timeDialog);
    m_timeEdit = new QTimeEdit();
    m_timeEdit->setDisplayFormat("HH:mm");
    timeDialogLayout->addWidget(m_timeEdit);
    QHBoxLayout *timeBtnLayout = new QHBoxLayout();
    QPushButton *timeCancelBtn = new QPushButton("取消");
    QPushButton *timeOkBtn = new QPushButton("确定");
    connect(timeCancelBtn, &QPushButton::clicked, m_timeDialog, &QDialog::reject);
    connect(timeOkBtn, &QPushButton::clicked, this, &DateTimePickerDialog::onTimeSelected);
    timeBtnLayout->addWidget(timeCancelBtn);
    timeBtnLayout->addWidget(timeOkBtn);
    timeDialogLayout->addLayout(timeBtnLayout);
    
    updateDisplay();
}

void DateTimePickerDialog::setDateTime(const QDateTime &dateTime)
{
    m_dateTime = dateTime;
    updateDisplay();
}

QDateTime DateTimePickerDialog::getDateTime() const
{
    QDate selectedDate = m_calendar->selectedDate();
    if (!selectedDate.isValid()) {
        selectedDate = m_dateTime.date();
    }
    QTime selectedTime = m_dateTime.time();
    return QDateTime(selectedDate, selectedTime);
}

void DateTimePickerDialog::updateDisplay()
{
    // 更新年月按钮文本
    QString yearMonthText = m_dateTime.toString("yyyy年MM月");
    m_yearMonthBtn->setText(yearMonthText);
    
    // 更新日历显示
    m_calendar->setSelectedDate(m_dateTime.date());
    m_calendar->setCurrentPage(m_dateTime.date().year(), m_dateTime.date().month());
    
    // 更新时间显示
    QString timeText = m_dateTime.toString("HH:mm");
    m_timeLabel->setText(timeText);
}

void DateTimePickerDialog::onPrevMonth()
{
    m_dateTime = m_dateTime.addMonths(-1);
    updateDisplay();
}

void DateTimePickerDialog::onNextMonth()
{
    m_dateTime = m_dateTime.addMonths(1);
    updateDisplay();
}

void DateTimePickerDialog::onYearMonthClicked()
{
    m_yearSpinBox->setValue(m_dateTime.date().year());
    m_monthSpinBox->setValue(m_dateTime.date().month());
    if (m_yearMonthDialog->exec() == QDialog::Accepted) {
        onYearMonthSelected();
    }
}

void DateTimePickerDialog::onYearMonthSelected()
{
    int year = m_yearSpinBox->value();
    int month = m_monthSpinBox->value();
    QDate newDate(year, month, qMin(m_dateTime.date().day(), QDate(year, month, 1).daysInMonth()));
    m_dateTime = QDateTime(newDate, m_dateTime.time());
    updateDisplay();
    m_yearMonthDialog->accept();
}

void DateTimePickerDialog::onTimeClicked()
{
    m_timeEdit->setTime(m_dateTime.time());
    if (m_timeDialog->exec() == QDialog::Accepted) {
        onTimeSelected();
    }
}

void DateTimePickerDialog::onTimeSelected()
{
    QTime selectedTime = m_timeEdit->time();
    m_dateTime = QDateTime(m_dateTime.date(), selectedTime);
    updateDisplay();
    m_timeDialog->accept();
}

