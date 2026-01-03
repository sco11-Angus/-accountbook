#include "statistics_widget.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>

StatisticsWidget::StatisticsWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("StatisticsWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    
    initUI();
    initStyleSheet();
}

void StatisticsWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 20);
    mainLayout->setSpacing(15);

    // Header with Month Switcher
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("收支统计");
    m_titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_prevMonthBtn = new QPushButton("<");
    m_monthLabel = new QLabel("2024-01");
    m_nextMonthBtn = new QPushButton(">");
    
    QString monthBtnStyle = "QPushButton { border: none; background: #eee; border-radius: 12px; width: 24px; height: 24px; font-weight: bold; } "
                           "QPushButton:hover { background: #ddd; }";
    m_prevMonthBtn->setStyleSheet(monthBtnStyle);
    m_nextMonthBtn->setStyleSheet(monthBtnStyle);
    m_monthLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #555; margin: 0 10px;");

    headerLayout->addWidget(m_prevMonthBtn);
    headerLayout->addWidget(m_monthLabel);
    headerLayout->addWidget(m_nextMonthBtn);
    mainLayout->addLayout(headerLayout);

    connect(m_prevMonthBtn, &QPushButton::clicked, this, &StatisticsWidget::onPrevMonth);
    connect(m_nextMonthBtn, &QPushButton::clicked, this, &StatisticsWidget::onNextMonth);

    // Summary Card
    QFrame *summaryCard = new QFrame();
    summaryCard->setObjectName("summaryCard");
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(20, 20, 20, 20);
    summaryLayout->setSpacing(10);

    m_totalExpenseLabel = new QLabel("总支出 ¥0.00");
    m_totalExpenseLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FF6B6B;");
    summaryLayout->addWidget(m_totalExpenseLabel);

    QHBoxLayout *subSummaryLayout = new QHBoxLayout();
    m_totalIncomeLabel = new QLabel("总收入 ¥0.00");
    m_balanceLabel = new QLabel("月结余 ¥0.00");
    m_totalIncomeLabel->setStyleSheet("color: #5D5D5D;");
    m_balanceLabel->setStyleSheet("color: #5D5D5D;");
    subSummaryLayout->addWidget(m_totalIncomeLabel);
    subSummaryLayout->addStretch();
    subSummaryLayout->addWidget(m_balanceLabel);
    summaryLayout->addLayout(subSummaryLayout);
    
    mainLayout->addWidget(summaryCard);

    // Daily Chart
    m_chartWidget = new ChartWidget();
    m_chartWidget->setFixedHeight(150);
    m_chartWidget->setStyleSheet("background: white; border-radius: 15px;");
    mainLayout->addWidget(m_chartWidget);

    // Tab Buttons
    QHBoxLayout *tabLayout = new QHBoxLayout();
    m_expenseTabBtn = new QPushButton("支出排行榜");
    m_incomeTabBtn = new QPushButton("收入排行榜");
    m_expenseTabBtn->setCheckable(true);
    m_incomeTabBtn->setCheckable(true);
    m_expenseTabBtn->setChecked(true);
    m_expenseTabBtn->setObjectName("tabBtn");
    m_incomeTabBtn->setObjectName("tabBtn");
    
    tabLayout->addWidget(m_expenseTabBtn);
    tabLayout->addWidget(m_incomeTabBtn);
    mainLayout->addLayout(tabLayout);

    connect(m_expenseTabBtn, &QPushButton::clicked, this, &StatisticsWidget::onExpenseTabClicked);
    connect(m_incomeTabBtn, &QPushButton::clicked, this, &StatisticsWidget::onIncomeTabClicked);

    // List Area
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");
    
    m_scrollContent = new QWidget();
    m_scrollContent->setObjectName("scrollContent");
    m_expenseListLayout = new QVBoxLayout(m_scrollContent);
    m_expenseListLayout->setContentsMargins(0, 0, 0, 0);
    m_expenseListLayout->setSpacing(12);
    m_expenseListLayout->addStretch();
    
    m_scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(m_scrollArea);
}

void StatisticsWidget::initStyleSheet()
{
    setStyleSheet(R"(
        QWidget#StatisticsWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #E0F7FA);
        }
        QFrame#summaryCard {
            background-color: white;
            border-radius: 20px;
            border: 1px solid rgba(0, 0, 0, 0.05);
        }
        QPushButton#tabBtn {
            background-color: rgba(255, 255, 255, 0.5);
            border: none;
            border-radius: 15px;
            padding: 8px 20px;
            font-weight: bold;
            color: #666;
        }
        QPushButton#tabBtn:checked {
            background-color: #FFD700;
            color: white;
        }
        QWidget#scrollContent {
            background: transparent;
        }
    )");
}

void StatisticsWidget::updateData(int userId, int year, int month)
{
    m_currentUserId = userId;
    m_currentYear = year;
    m_currentMonth = month;
    m_monthLabel->setText(QString("%1-%2").arg(year).arg(month, 2, 10, QChar('0')));

    MonthlyStat stat = StatisticsManager::getInstance()->getMonthlyStat(userId, year, month);
    
    m_totalExpenseLabel->setText(QString("总支出 ¥%1").arg(QString::number(stat.totalExpense, 'f', 2)));
    m_totalIncomeLabel->setText(QString("总收入 ¥%1").arg(QString::number(stat.totalIncome, 'f', 2)));
    m_balanceLabel->setText(QString("月结余 ¥%1").arg(QString::number(stat.balance, 'f', 2)));

    updateChart(stat.dailyStats);
    refreshList(stat);
}

void StatisticsWidget::updateChart(const QList<DailyStat>& dailyStats)
{
    m_dailyStats = dailyStats;
    m_maxDailyAmount = 0;
    for (const auto& ds : dailyStats) {
        m_maxDailyAmount = qMax(m_maxDailyAmount, qMax(ds.income, ds.expense));
    }
    m_chartWidget->setDailyStats(dailyStats, m_maxDailyAmount);
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    
    if (m_dailyStats.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int padding = 15;
    int chartContentWidth = width() - 2 * padding;
    int chartContentHeight = height() - 2 * padding - 15;
    int xStart = padding;
    int yBottom = height() - padding - 15;

    int dayCount = m_dailyStats.size();
    float barWidth = (float)chartContentWidth / dayCount;
    float gap = barWidth * 0.2f;
    float actualBarWidth = barWidth - gap;

    for (int i = 0; i < dayCount; ++i) {
        const auto& ds = m_dailyStats[i];
        float x = xStart + i * barWidth;

        // Draw Expense Bar (Red)
        if (ds.expense > 0 && m_maxDailyAmount > 0) {
            float h = (ds.expense / m_maxDailyAmount) * chartContentHeight;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#FF6B6B"));
            painter.drawRoundedRect(QRectF(x, yBottom - h, actualBarWidth, h), 2, 2);
        }

        // Draw Income Bar (Green)
        if (ds.income > 0 && m_maxDailyAmount > 0) {
            float h = (ds.income / m_maxDailyAmount) * chartContentHeight;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#4CAF50"));
            // Thinner and slightly offset
            painter.drawRoundedRect(QRectF(x + actualBarWidth * 0.4f, yBottom - h, actualBarWidth * 0.5f, h), 1, 1);
        }
        
        // Draw day labels every 5 days
        if (ds.day == 1 || ds.day == 10 || ds.day == 20 || ds.day == dayCount) {
            painter.setPen(QColor("#999"));
            painter.setFont(QFont("Microsoft YaHei", 7));
            painter.drawText(QRectF(x - 5, yBottom + 2, barWidth + 10, 15), Qt::AlignCenter, QString::number(ds.day));
        }
    }
}

void StatisticsWidget::onPrevMonth()
{
    m_currentMonth--;
    if (m_currentMonth < 1) {
        m_currentMonth = 12;
        m_currentYear--;
    }
    updateData(m_currentUserId, m_currentYear, m_currentMonth);
}

void StatisticsWidget::onNextMonth()
{
    m_currentMonth++;
    if (m_currentMonth > 12) {
        m_currentMonth = 1;
        m_currentYear++;
    }
    updateData(m_currentUserId, m_currentYear, m_currentMonth);
}

void StatisticsWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

void StatisticsWidget::refreshList(const MonthlyStat& stat)
{
    // Clear existing items
    QLayoutItem *child;
    while ((child = m_expenseListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    const QList<CategoryStat>& currentStats = m_isShowingExpense ? stat.expenseStats : stat.incomeStats;

    if (currentStats.isEmpty()) {
        QLabel *emptyLabel = new QLabel("本月暂无数据");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; padding: 40px;");
        m_expenseListLayout->addWidget(emptyLabel);
    } else {
        for (const auto& cs : currentStats) {
            m_expenseListLayout->addWidget(createCategoryItem(cs));
        }
    }
    m_expenseListLayout->addStretch();
}

QWidget* StatisticsWidget::createCategoryItem(const CategoryStat& stat)
{
    QWidget *item = new QWidget();
    item->setFixedHeight(60);
    item->setStyleSheet("background-color: white; border-radius: 15px;");
    
    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(15, 10, 15, 10);
    
    // Category Icon Placeholder (First char of name)
    QLabel *iconLabel = new QLabel(stat.category.left(1));
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(QString("background-color: %1; color: white; border-radius: 20px; font-weight: bold;")
                             .arg(stat.color));
    layout->addWidget(iconLabel);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    
    QHBoxLayout *textLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel(stat.category);
    nameLabel->setStyleSheet("font-weight: bold; color: #333;");
    QLabel *percentLabel = new QLabel(QString::number(stat.percentage, 'f', 1) + "%");
    percentLabel->setStyleSheet("color: #999; font-size: 12px;");
    textLayout->addWidget(nameLabel);
    textLayout->addStretch();
    textLayout->addWidget(percentLabel);
    infoLayout->addLayout(textLayout);

    QProgressBar *bar = new QProgressBar();
    bar->setFixedHeight(6);
    bar->setTextVisible(false);
    bar->setRange(0, 100);
    bar->setValue(static_cast<int>(stat.percentage));
    bar->setStyleSheet(QString(
        "QProgressBar { background-color: #F0F0F0; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 3px; }"
    ).arg(stat.color));
    infoLayout->addWidget(bar);

    layout->addLayout(infoLayout);
    layout->setStretch(1, 1);

    QLabel *amountLabel = new QLabel(QString("¥%1").arg(QString::number(stat.amount, 'f', 2)));
    amountLabel->setStyleSheet("font-weight: bold; color: #333;");
    layout->addWidget(amountLabel);

    return item;
}

void StatisticsWidget::onExpenseTabClicked()
{
    m_isShowingExpense = true;
    m_expenseTabBtn->setChecked(true);
    m_incomeTabBtn->setChecked(false);
    updateData(m_currentUserId, m_currentYear, m_currentMonth);
}

void StatisticsWidget::onIncomeTabClicked()
{
    m_isShowingExpense = false;
    m_expenseTabBtn->setChecked(false);
    m_incomeTabBtn->setChecked(true);
    updateData(m_currentUserId, m_currentYear, m_currentMonth);
}
