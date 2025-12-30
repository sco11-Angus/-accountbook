#include "AccountBookMainWidget.h"
#include "AccountBookRecordWidget.h"
<<<<<<< HEAD
#include "monthpickerdialog.h"
#include "sqlite_helper.h"
=======
#include "account_manager.h"
#include "user_manager.h"
>>>>>>> origin/branch
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QRegularExpression>
#include <QString>
<<<<<<< HEAD
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
=======
#include <QDebug>
#include <QDate>
>>>>>>> origin/branch

AccountBookMainWidget::AccountBookMainWidget(QWidget *parent)
    : QWidget(parent), m_currentDate(QDate::currentDate())
{
    setObjectName("AccountBookMainWidget");
    // 启用样式表背景绘制，避免在嵌入到 MainWindow 时仍然显示为黑色
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedSize(450, 650); // 和登录注册页尺寸一致
    
    // 初始化当前日期为本月第一天
    m_currentDate.setDate(m_currentDate.year(), m_currentDate.month(), 1);
    
    initUI();
    initStyleSheet();
    
    // 初始化显示
    updateDateDisplay();
    loadBillsForMonth();

    // 右下角加号：打开独立记账窗口（单独的界面）
    connect(m_addBtn, &QPushButton::clicked, this, [this]() {
        // 不设置父对象，作为顶层窗口单独显示
        auto *recordWidget = new AccountBookRecordWidget(nullptr);
        recordWidget->setAttribute(Qt::WA_DeleteOnClose); // 关闭自动释放
        // 记账完成后刷新列表
        connect(recordWidget, &QWidget::destroyed, this, &AccountBookMainWidget::loadBillsForMonth);
        recordWidget->show();
        recordWidget->activateWindow();
        recordWidget->raise();
    });
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
    m_prevMonthBtn->setFixedSize(30, 30);
    m_prevMonthBtn->setCursor(Qt::PointingHandCursor);
    m_prevMonthBtn->setObjectName("m_prevMonthBtn");
    
    m_monthLabel = new QLabel(); // 我们将使用事件过滤器或直接响应点击
    m_monthLabel->setCursor(Qt::PointingHandCursor);
    m_monthLabel->setObjectName("m_monthLabel");
    m_monthLabel->installEventFilter(this); // 安装事件过滤器监听点击
    
    QFont monthFont;
    monthFont.setPointSize(14);
    monthFont.setBold(true);
    m_monthLabel->setFont(monthFont);
    
    m_nextMonthBtn = new QPushButton(">");
    m_nextMonthBtn->setFixedSize(30, 30);
    m_nextMonthBtn->setCursor(Qt::PointingHandCursor);
    m_nextMonthBtn->setObjectName("m_nextMonthBtn");
    
    m_calendarBtn = new QPushButton("收支日历");
    m_calendarBtn->setFixedHeight(30);
    m_calendarBtn->setObjectName("m_calendarBtn");

    monthBarLayout->addWidget(m_prevMonthBtn);
    monthBarLayout->addWidget(m_monthLabel);
    monthBarLayout->addWidget(m_nextMonthBtn);
    monthBarLayout->addStretch();
    monthBarLayout->addWidget(m_calendarBtn);
    mainLayout->addLayout(monthBarLayout);

    // 连接月份切换信号
    connect(m_prevMonthBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onPrevMonth);
    connect(m_nextMonthBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onNextMonth);

    // 收支统计卡片（轻玻璃质感）

    // 收支统计卡片（轻玻璃质感）
    m_statCard = new QFrame();
    m_statCard->setObjectName("m_statCard");
    QVBoxLayout *statLayout = new QVBoxLayout(m_statCard);
    statLayout->setContentsMargins(20, 20, 20, 20);

    m_totalExpenseLabel = new QLabel("总支出 ¥0.00"); // 初始0
    QFont expenseFont;
    expenseFont.setPointSize(20);
    expenseFont.setBold(true);
    m_totalExpenseLabel->setFont(expenseFont);
    m_totalExpenseLabel->setStyleSheet("color: #FF6B6B;");
    statLayout->addWidget(m_totalExpenseLabel);

    QHBoxLayout *subStatLayout = new QHBoxLayout();
    m_totalIncomeLabel = new QLabel("总收入 ¥0.00"); // 初始0
    m_monthSurplusLabel = new QLabel("月结余 ¥0.00"); // 初始0
    subStatLayout->addWidget(m_totalIncomeLabel);
    subStatLayout->addStretch();
    subStatLayout->addWidget(m_monthSurplusLabel);
    statLayout->addLayout(subStatLayout);
    mainLayout->addWidget(m_statCard);

    // ========== 2. 账单列表初始化为空（替换硬编码示例项） ==========
    m_billListWidget = new QListWidget();
    m_billListWidget->setSpacing(10);
    // 初始显示“暂无账单”提示项
    QListWidgetItem *emptyItem = new QListWidgetItem(m_billListWidget);
    QWidget *emptyWidget = new QWidget();
    QLabel *emptyLabel = new QLabel("暂无账单，点击右下角+开始记账吧～");
    emptyLabel->setStyleSheet("color: #999; padding: 20px 0;");
    emptyLabel->setAlignment(Qt::AlignCenter);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->addWidget(emptyLabel, 0, Qt::AlignCenter);
    // 为空状态设置一个固定高度，确保文字可见
    emptyItem->setSizeHint(QSize(0, 320));
    m_billListWidget->setItemWidget(emptyItem, emptyWidget);
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
    // 按钮整体缩小为原来的约 2/3，但保持加号字体大小不变
    m_addBtn->setFixedSize(40, 40);
    QFont addFont;
    addFont.setPointSize(24);
    m_addBtn->setFont(addFont);
    m_addBtn->setParent(this);
    m_addBtn->move(width() - 80, height() - 100);
    m_addBtn->setObjectName("m_addBtn");
}

// ========== 新增：更新收支统计（核心动态函数） ==========
void AccountBookMainWidget::updateStatistic(double totalExpense, double totalIncome)
{
    double surplus = totalIncome - totalExpense;
    QString expenseText = QString("总支出 ¥%1").arg(
        QString::number(totalExpense, 'f', 2).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,")
        );
    QString incomeText = QString("总收入 ¥%1").arg(
        QString::number(totalIncome, 'f', 2).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,")
        );
    QString surplusText = QString("月结余 ¥%1").arg(
        QString::number(surplus, 'f', 2).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,")
        );
    m_totalExpenseLabel->setText(expenseText);
    m_totalIncomeLabel->setText(incomeText);
    m_monthSurplusLabel->setText(surplusText);
}

// ========== 新增：创建单个账单项（通用工具函数） ==========
QWidget* AccountBookMainWidget::createBillItemWidget(const QString& date, const QString& cateIcon,
                                                     const QString& cateName, const QString& time,
                                                     const QString& amount, bool isExpense)
{
    QWidget *itemWidget = new QWidget();
    QVBoxLayout *itemLayout = new QVBoxLayout(itemWidget);

    // 日期行（如：12/18 星期四）
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel(date));
    dateLayout->addStretch();
    // 该日期的收支小计（需从账单数据汇总，这里先占位，实际可传参数）
    dateLayout->addWidget(new QLabel(QString("支出:¥%1 收入:¥%2").arg(isExpense ? amount : "0").arg(!isExpense ? amount : "0")));
    itemLayout->addLayout(dateLayout);

    // 账单详情行
    QHBoxLayout *billItemLayout = new QHBoxLayout();
    // 分类图标（如“餐”“奖”）
    QLabel *iconLabel = new QLabel(cateIcon);
    iconLabel->setFixedSize(30, 30);
    QString bgColor = isExpense ? "#FF6B6B" : "#4CAF50"; // 支出红、收入绿
    iconLabel->setStyleSheet(QString("background-color: %1; border-radius: 15px; color: white; text-align: center;").arg(bgColor));
    billItemLayout->addWidget(iconLabel);

    // 分类名称+时间（如“餐饮-三餐\n18:07 · 晚”）
    billItemLayout->addWidget(new QLabel(QString("%1\n%2").arg(cateName).arg(time)));
    billItemLayout->addStretch();

    // 金额（支出带-，收入带+）
    QString amountText = isExpense ? QString("-%1").arg(amount) : QString("+%1").arg(amount);
    billItemLayout->addWidget(new QLabel(amountText));
    itemLayout->addLayout(billItemLayout);

    return itemWidget;
}

// ========== 新增：批量更新账单列表（核心动态函数） ==========
void AccountBookMainWidget::updateBillData(const QList<QMap<QString, QString>>& billList)
{
    // 清空原有列表
    m_billListWidget->clear();

    // 如果无数据，显示空状态
    if (billList.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem(m_billListWidget);
        QWidget *emptyWidget = new QWidget();
        QLabel *emptyLabel = new QLabel("本月暂无数据");
        emptyLabel->setStyleSheet("color: #999; padding: 20px 0; font-size: 16px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
        emptyLayout->addWidget(emptyLabel, 0, Qt::AlignCenter);
        emptyItem->setSizeHint(QSize(0, 300));
        m_billListWidget->setItemWidget(emptyItem, emptyWidget);

        // 同时更新收支统计为0
        updateStatistic(0.0, 0.0);
        return;
    }

    // 动态生成账单项
    double totalExpense = 0.0;
    double totalIncome = 0.0;

    for (const QMap<QString, QString>& bill : billList) {
        double amount = bill["amount"].toDouble();
        bool isExpense  = (bill["isExpense"] == "true");

        QListWidgetItem *item = new QListWidgetItem(m_billListWidget);
        QWidget *itemWidget = new QWidget();
        QVBoxLayout *itemLayout = new QVBoxLayout(itemWidget);
        itemLayout->setContentsMargins(15, 10, 15, 10);

        QHBoxLayout *dateLayout = new QHBoxLayout();
        QLabel *dateLabel = new QLabel(bill["date"]);
        dateLabel->setStyleSheet("color: #666; font-size: 12px;");
        dateLayout->addWidget(dateLabel);
        dateLayout->addStretch();
        
        QLabel *summaryLabel = new QLabel(QString("%1¥%2").arg(isExpense ? "支出:" : "收入:", 
                                         QString::number(amount, 'f', 2)));
        summaryLabel->setStyleSheet("color: #999; font-size: 12px;");
        dateLayout->addWidget(summaryLabel);
        itemLayout->addLayout(dateLayout);

        QHBoxLayout *billLayout = new QHBoxLayout();
        QLabel *iconLabel = new QLabel(bill["cateIcon"]);
        iconLabel->setFixedSize(35, 35);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("background-color: %1; border-radius: 17px; color: white; font-weight: bold; font-size: 14px;")
                                     .arg(isExpense ? "#FF6B6B" : "#4CAF50"));
        billLayout->addWidget(iconLabel);
        
        QLabel *infoLabel = new QLabel(QString("<b>%1</b><br><font color='#999' size='2'>%2</font>").arg(bill["cateName"], bill["time"]));
        billLayout->addWidget(infoLabel);
        billLayout->addStretch();
        
        QLabel *amountLabel = new QLabel(QString("%1¥%2").arg(isExpense ? "-" : "+", 
                                         QString::number(amount, 'f', 2)));
        amountLabel->setStyleSheet(QString("font-weight: bold; font-size: 16px; color: %1;")
                                    .arg(isExpense ? "#333" : "#4CAF50"));
        billLayout->addWidget(amountLabel);
        itemLayout->addLayout(billLayout);

        item->setSizeHint(itemWidget->sizeHint());
        m_billListWidget->setItemWidget(item, itemWidget);

        if (isExpense) totalExpense += amount;
        else totalIncome += amount;
    }

    updateStatistic(totalExpense, totalIncome);
}

void AccountBookMainWidget::onPrevMonth()
{
    m_currentDate = m_currentDate.addMonths(-1);
    updateDateDisplay();
    loadBillsForMonth();
}

void AccountBookMainWidget::onNextMonth()
{
    m_currentDate = m_currentDate.addMonths(1);
    updateDateDisplay();
    loadBillsForMonth();
}

void AccountBookMainWidget::onMonthLabelClicked()
{
    MonthPickerDialog dialog(m_currentDate, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_currentDate = dialog.getSelectedDate();
        updateDateDisplay();
        loadBillsForMonth();
    }
}

void AccountBookMainWidget::updateDateDisplay()
{
    m_monthLabel->setText(m_currentDate.toString("yyyy年MM月"));
}

void AccountBookMainWidget::loadBillsForMonth()
{
    // 获取当月开始和结束时间
    QString startTime = m_currentDate.toString("yyyy-MM-01 00:00:00");
    QString endTime = m_currentDate.addMonths(1).addDays(-m_currentDate.addMonths(1).day() + 1).addDays(-1).toString("yyyy-MM-dd 23:59:59");
    
    // 获取当月最后一天
    QDate lastDayOfMonth = m_currentDate.addMonths(1).addDays(-1);
    endTime = lastDayOfMonth.toString("yyyy-MM-dd 23:59:59");

    QList<QMap<QString, QString>> billList;
    
    // 从数据库查询
    QString sql = QString("SELECT * FROM account_record WHERE create_time >= '%1' AND create_time <= '%2' AND is_deleted = 0 ORDER BY create_time DESC")
                  .arg(startTime).arg(endTime);
                  
    QSqlQuery query = SqliteHelper::getInstance()->executeQuery(sql);
    while (query.next()) {
        QMap<QString, QString> bill;
        QDateTime dt = QDateTime::fromString(query.value("create_time").toString(), "yyyy-MM-dd HH:mm:ss");
        
        bill["date"] = dt.toString("MM/dd ") + dt.date().toString("ddd");
        bill["time"] = dt.toString("HH:mm");
        bill["cateName"] = query.value("type").toString();
        bill["cateIcon"] = bill["cateName"].left(1); // 暂时取第一个字作为图标
        bill["amount"] = QString::number(query.value("amount").toDouble(), 'f', 2);
        bill["isExpense"] = (query.value("is_income").toInt() == 0) ? "true" : "false";
        
        billList.append(bill);
    }
    
    updateBillData(billList);
}

bool AccountBookMainWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_monthLabel && event->type() == QEvent::MouseButtonPress) {
        onMonthLabelClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
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
        QPushButton#m_prevMonthBtn, QPushButton#m_nextMonthBtn {
            font-size: 18px;
            font-weight: bold;
            color: #333;
        }
        QLabel#m_monthLabel {
            color: #333;
            padding: 0 5px;
        }
        QFrame#m_statCard {
            background-color: white;
            border-radius: 25px;
            border: 1px solid rgba(0, 0, 0, 0.1);
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
            border-radius: 20px;
            box-shadow: 0 4px 12px rgba(255, 215, 0, 0.3);
        }
    )");
    m_bookNavBtn->setCheckable(true);
    m_bookNavBtn->setChecked(true); // 默认选中账本
}
