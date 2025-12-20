#include "AccountBookMainWidget.h"
#include <QFont>

AccountBookMainWidget::AccountBookMainWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("AccountBookMainWidget");
    // 启用样式表背景绘制，避免在嵌入到 MainWindow 时仍然显示为黑色
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(450, 650); // 和登录注册页尺寸一致
    initUI();
    initStyleSheet();
}

void AccountBookMainWidget::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 顶部：账本切换 + 搜索框
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    m_bookSwitchCombo = new QComboBox();
    m_bookSwitchCombo->addItems({"默认账本", "生活账本", "工作账本"});
    topBarLayout->addWidget(m_bookSwitchCombo);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("输入关键词");
    m_searchEdit->setFixedHeight(35);
    topBarLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(topBarLayout);

    // 月份切换栏
    QHBoxLayout *monthBarLayout = new QHBoxLayout();
    m_prevMonthBtn = new QPushButton("<");
    m_monthLabel = new QLabel("2025年12月");
    QFont monthFont;
    monthFont.setPointSize(14);
    monthFont.setBold(true);
    m_monthLabel->setFont(monthFont);
    m_nextMonthBtn = new QPushButton(">");
    m_calendarBtn = new QPushButton("收支日历");
    m_calendarBtn->setFixedHeight(30);

    monthBarLayout->addWidget(m_prevMonthBtn);
    monthBarLayout->addWidget(m_monthLabel);
    monthBarLayout->addWidget(m_nextMonthBtn);
    monthBarLayout->addStretch();
    monthBarLayout->addWidget(m_calendarBtn);
    mainLayout->addLayout(monthBarLayout);

    // 收支统计卡片（轻玻璃质感）
    m_statCard = new QFrame();
    QVBoxLayout *statLayout = new QVBoxLayout(m_statCard);
    statLayout->setContentsMargins(20, 20, 20, 20);

    m_totalExpenseLabel = new QLabel("总支出 ¥922.50");
    QFont expenseFont;
    expenseFont.setPointSize(20);
    expenseFont.setBold(true);
    m_totalExpenseLabel->setFont(expenseFont);
    m_totalExpenseLabel->setStyleSheet("color: #FF6B6B;");
    statLayout->addWidget(m_totalExpenseLabel);

    QHBoxLayout *subStatLayout = new QHBoxLayout();
    m_totalIncomeLabel = new QLabel("总收入 ¥1,900.00");
    m_monthSurplusLabel = new QLabel("月结余 ¥977.50");
    subStatLayout->addWidget(m_totalIncomeLabel);
    subStatLayout->addStretch();
    subStatLayout->addWidget(m_monthSurplusLabel);
    statLayout->addLayout(subStatLayout);
    mainLayout->addWidget(m_statCard);

    // 账单列表（示例项，实际需动态生成）
    m_billListWidget = new QListWidget();
    m_billListWidget->setSpacing(10);

    // 12/18 账单项示例
    QListWidgetItem *item1 = new QListWidgetItem(m_billListWidget);
    QWidget *itemWidget1 = new QWidget();
    QVBoxLayout *itemLayout1 = new QVBoxLayout(itemWidget1);

    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("12/18 星期四"));
    dateLayout->addStretch();
    dateLayout->addWidget(new QLabel("支出:¥24.00 收入:¥1,000.00"));
    itemLayout1->addLayout(dateLayout);

    // 奖金收入项
    QHBoxLayout *bonusItem = new QHBoxLayout();
    QLabel *bonusIcon = new QLabel("奖");
    bonusIcon->setFixedSize(30, 30);
    bonusIcon->setStyleSheet("background-color: #4CAF50; border-radius: 15px; color: white; text-align: center;");
    bonusItem->addWidget(bonusIcon);
    bonusItem->addWidget(new QLabel("奖金\n18:44 · 奖学金"));
    bonusItem->addStretch();
    bonusItem->addWidget(new QLabel("+¥1,000.00"));
    itemLayout1->addLayout(bonusItem);

    // 餐饮支出项
    QHBoxLayout *foodItem = new QHBoxLayout();
    QLabel *foodIcon = new QLabel("餐");
    foodIcon->setFixedSize(30, 30);
    foodIcon->setStyleSheet("background-color: #FF6B6B; border-radius: 15px; color: white; text-align: center;");
    foodItem->addWidget(foodIcon);
    foodItem->addWidget(new QLabel("餐饮-三餐\n18:07 · 晚"));
    foodItem->addStretch();
    foodItem->addWidget(new QLabel("-¥13.00"));
    itemLayout1->addLayout(foodItem);

    item1->setSizeHint(itemWidget1->sizeHint());
    m_billListWidget->setItemWidget(item1, itemWidget1);
    mainLayout->addWidget(m_billListWidget);

    // 底部导航
    QHBoxLayout *navLayout = new QHBoxLayout();
    m_bookNavBtn = new QPushButton("账本");
    m_assetNavBtn = new QPushButton("资产");
    m_statNavBtn = new QPushButton("统计");
    navLayout->addWidget(m_bookNavBtn);
    navLayout->addWidget(m_assetNavBtn);
    navLayout->addWidget(m_statNavBtn);
    mainLayout->addLayout(navLayout);

    // 右下角加号按钮
    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedSize(60, 60);
    QFont addFont;
    addFont.setPointSize(24);
    m_addBtn->setFont(addFont);
    m_addBtn->setParent(this);
    m_addBtn->move(width() - 80, height() - 100);
}

void AccountBookMainWidget::initStyleSheet()
{
    setStyleSheet(R"(
        QWidget#AccountBookMainWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #E0F7FA);
        }
        QComboBox {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 15px;
            padding: 5px 10px;
            border: none;
            font-size: 14px;
        }
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 15px;
            padding: 0 10px;
            border: none;
            font-size: 14px;
        }
        QPushButton {
            background-color: transparent;
            border: none;
            font-size: 14px;
            color: #5D5D5D;
        }
        QPushButton#m_calendarBtn {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 10px;
            padding: 0 10px;
        }
        QFrame#m_statCard {
            background-color: rgba(255, 255, 255, 0.85);
            border-radius: 25px;
            border: 1px solid rgba(255, 255, 255, 0.6);
            box-shadow: 0 8px 20px rgba(0, 0, 0, 0.05);
        }
        QListWidget {
            background-color: transparent;
            border: none;
        }
        QListWidget::item {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 15px;
            padding: 10px;
        }
        QPushButton#m_bookNavBtn:checked {
            color: #FFB6C1;
            font-weight: bold;
        }
        QPushButton#m_addBtn {
            background-color: #FFD700;
            color: white;
            border-radius: 30px;
            box-shadow: 0 4px 12px rgba(255, 215, 0, 0.3);
        }
    )");
    m_bookNavBtn->setCheckable(true);
    m_bookNavBtn->setChecked(true); // 默认选中账本
}
