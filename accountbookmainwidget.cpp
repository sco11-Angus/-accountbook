#include "AccountBookMainWidget.h"
#include "bill_service.h"
#include "AccountBookRecordWidget.h"
#include "monthpickerdialog.h"
#include "sqlite_helper.h"
#include "account_manager.h"
#include "user_manager.h"
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
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QFile>

// 静态辅助函数：获取分类名称到拼音的映射（与 AccountBookRecordWidget 保持一致）
static QMap<QString, QString> getCategoryPinyinMap() {
    QMap<QString, QString> cateMap;
    // 支出分类
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
    // 收入分类
    cateMap["副业"] = "fuye";
    cateMap["工资"] = "gongzi";
    cateMap["红包"] = "hongbao";
    cateMap["兼职"] = "jianzhi";
    cateMap["投资"] = "touzi";
    cateMap["意外收入"] = "yiwaishouru";
    return cateMap;
}

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
        
        // 记账完成后立即刷新列表（不需要等窗口销毁）
        connect(recordWidget, &AccountBookRecordWidget::billRecorded, this, &AccountBookMainWidget::loadBillsForMonth);
        
        // 窗口销毁时也刷新一次作为兜底
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

// 更新收支统计（总支出/总收入/结余）
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

// ========== 新增：批量更新账单列表（核心动态函数） ==========
void AccountBookMainWidget::updateBillData(const QList<QMap<QString, QString>>& billList)
{
    m_billListWidget->clear();

    if (billList.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem(m_billListWidget);
        QWidget *emptyWidget = new QWidget();
        // 动态计算高度：listWidget高度 - 顶部卡片和边距的估算高度
        // 这里设为一个较大的固定值或者让它拉伸
        emptyWidget->setMinimumHeight(350); 
        
        QLabel *emptyLabel = new QLabel("本月暂无数据");
        emptyLabel->setStyleSheet("color: #999; font-size: 16px; font-weight: 500;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
        emptyLayout->addStretch(1);
        emptyLayout->addWidget(emptyLabel);
        emptyLayout->addStretch(1);
        
        emptyItem->setSizeHint(QSize(0, 350));
        emptyItem->setFlags(Qt::NoItemFlags); // 不可交互
        m_billListWidget->setItemWidget(emptyItem, emptyWidget);
        updateStatistic(0, 0);
        return;
    }

    // 按日期分组
    QMap<QString, QList<QMap<QString, QString>>> groupedBills;
    QStringList dateOrder; // 保持日期顺序
    double totalExpense = 0.0;
    double totalIncome = 0.0;

    for (const QMap<QString, QString>& bill : billList) {
        QString date = bill["date"];
        if (!groupedBills.contains(date)) {
            dateOrder.append(date);
        }
        groupedBills[date].append(bill);
        
        double amount = bill["amount"].toDouble();
        if (bill["isExpense"] == "true") totalExpense += amount;
        else totalIncome += amount;
    }

    // 获取分类图标映射
    static QMap<QString, QString> pinyinMap = getCategoryPinyinMap();

    // 渲染分组数据
    for (const QString& date : dateOrder) {
        // 1. 添加日期抬头
        QListWidgetItem *headerItem = new QListWidgetItem(m_billListWidget);
        QWidget *headerWidget = new QWidget();
        headerWidget->setStyleSheet("background-color: transparent;");
        QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
        headerLayout->setContentsMargins(15, 10, 15, 5);

        QLabel *dateLabel = new QLabel(date);
        dateLabel->setStyleSheet("color: #999; font-size: 13px; font-weight: bold;");
        headerLayout->addWidget(dateLabel);
        headerLayout->addStretch();

        // 计算该日支出（可选，参考图中右上角）
        double dayExpense = 0;
        for (const auto& b : groupedBills[date]) {
            if (b["isExpense"] == "true") dayExpense += b["amount"].toDouble();
        }
        if (dayExpense > 0) {
            QLabel *dayStatLabel = new QLabel(QString("支出: ¥%1").arg(QString::number(dayExpense, 'f', 2)));
            dayStatLabel->setStyleSheet("color: #999; font-size: 12px;");
            headerLayout->addWidget(dayStatLabel);
        }

        headerItem->setSizeHint(headerWidget->sizeHint());
        headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsSelectable); // 抬头不可选中
        m_billListWidget->setItemWidget(headerItem, headerWidget);

        // 2. 添加该日期下的所有账单项
        for (const QMap<QString, QString>& bill : groupedBills[date]) {
            double amount = bill["amount"].toDouble();
            bool isExpense = (bill["isExpense"] == "true");
            QString cateName = bill["cateName"];
            QString time = bill["time"];

            QListWidgetItem *item = new QListWidgetItem(m_billListWidget);
            
            // 外层容器，负责边距
            QWidget *container = new QWidget();
            QVBoxLayout *containerLayout = new QVBoxLayout(container);
            containerLayout->setContentsMargins(15, 4, 15, 4);
            
            // 内层卡片，负责背景和样式
            QWidget *itemWidget = new QWidget();
            itemWidget->setObjectName("billItemWidget");
            QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
            itemLayout->setContentsMargins(12, 10, 12, 10);
            itemLayout->setSpacing(12);

            // 图标
            QLabel *iconLabel = new QLabel();
            iconLabel->setFixedSize(40, 40);
            QString pinyin = pinyinMap.value(cateName, "qita");
            QString imgDir = isExpense ? "classify1" : "classify2";
            QString iconPath = QString(":/%1/resources/%2/%3.jpg").arg(imgDir).arg(imgDir).arg(pinyin);
            
            if (QFile::exists(iconPath)) {
                QPixmap pix(iconPath);
                iconLabel->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                iconLabel->setStyleSheet("border-radius: 20px; overflow: hidden; background-color: #F8F8F8;");
            } else {
                // 兜底：显示文字首字母
                iconLabel->setText(cateName.left(1));
                iconLabel->setAlignment(Qt::AlignCenter);
                iconLabel->setStyleSheet(QString("background-color: %1; border-radius: 20px; color: white; font-weight: bold;")
                                         .arg(isExpense ? "#FF6B6B" : "#4CAF50"));
            }
            itemLayout->addWidget(iconLabel);

            // 信息列（分类名称 + 时间）
            QVBoxLayout *infoLayout = new QVBoxLayout();
            infoLayout->setSpacing(2);
            QLabel *nameLabel = new QLabel(cateName);
            nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
            QLabel *timeLabel = new QLabel(time);
            timeLabel->setStyleSheet("font-size: 11px; color: #999;");
            infoLayout->addWidget(nameLabel);
            infoLayout->addWidget(timeLabel);
            itemLayout->addLayout(infoLayout);

            itemLayout->addStretch();

            // 金额
            QLabel *amountLabel = new QLabel(QString("%1%2").arg(isExpense ? "-" : "+", 
                                             QString::number(amount, 'f', 2)));
            amountLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
                                        .arg(isExpense ? "#333" : "#4CAF50"));
            itemLayout->addWidget(amountLabel);

            containerLayout->addWidget(itemWidget);
            item->setSizeHint(container->sizeHint());
            m_billListWidget->setItemWidget(item, container);
        }
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
    int userId = UserManager::getInstance()->getCurrentUser().getId();
    if (userId <= 0) {
        qDebug() << "未登录用户，无法加载账单";
        return;
    }

    QList<AccountRecord> records = BillService::getMonthlyBills(userId, m_currentDate);
    QList<QMap<QString, QString>> billList;
    
    for (const AccountRecord& record : records) {
        QMap<QString, QString> bill;
        QDateTime dt = QDateTime::fromString(record.getCreateTime(), "yyyy-MM-dd HH:mm:ss");
        
        bill["date"] = dt.toString("MM/dd ") + dt.date().toString("ddd");
        bill["time"] = dt.toString("HH:mm");
        bill["cateName"] = record.getType();
        bill["cateIcon"] = bill["cateName"].left(1); 
        
        double amount = record.getAmount();
        bool isExpense = (amount < 0);
        
        bill["amount"] = QString::number(qAbs(amount), 'f', 2); // UI 显示绝对值
        bill["isExpense"] = isExpense ? "true" : "false";
        
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
            outline: none;
        }
        QListWidget::item {
            background-color: transparent;
            padding: 0px;
            margin: 0px;
        }
        QListWidget::item:selected {
            background-color: transparent;
        }
        QWidget#billItemWidget {
            background-color: white;
            border-radius: 12px;
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
