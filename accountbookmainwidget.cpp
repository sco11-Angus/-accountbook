#include "AccountBookMainWidget.h"
#include "bill_service.h"
#include "AccountBookRecordWidget.h"
#include "monthpickerdialog.h"
#include "sqlite_helper.h"
#include "account_manager.h"
#include "user_manager.h"
#include "sync_manager.h"
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
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>

#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

// 自定义美化版 ActionSheet (底部弹窗)
class ActionSheet : public QDialog {
public:
    ActionSheet(QWidget *parent = nullptr) : QDialog(parent) {
        // 使用 Tool 提示，防止在任务栏显示，并移除边框和阴影提示
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::NoDropShadowWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        
        // 遮罩层：填满整个对话框
        m_mask = new QWidget(this);
        m_mask->setStyleSheet("background-color: rgba(0, 0, 0, 0.4);");
        
        // 内容层
        m_content = new QWidget(this);
        m_content->setObjectName("SheetContent");
        m_content->setStyleSheet(R"(
            QWidget#SheetContent {
                background-color: white;
                border-top-left-radius: 20px;
                border-top-right-radius: 20px;
                /* 使用边框代替阴影，防止渲染引擎在透明窗口边缘报错 */
                border-top: 1px solid #ddd;
                border-left: 1px solid #ddd;
                border-right: 1px solid #ddd;
            }
            QPushButton {
                border: none;
                border-bottom: 1px solid #f0f0f0;
                padding: 15px;
                font-size: 16px;
                background-color: white;
                text-align: center;
            }
            QPushButton:pressed {
                background-color: #f8f8f8;
            }
            QPushButton#DeleteBtn {
                color: #ff4d4f;
            }
            QPushButton#CancelBtn {
                border-top: 8px solid #f5f5f5;
                border-bottom: none;
                color: #666;
            }
        )");

        QVBoxLayout *contentLayout = new QVBoxLayout(m_content);
        contentLayout->setContentsMargins(0, 10, 0, 0);
        contentLayout->setSpacing(0);

        m_editBtn = new QPushButton("编辑", m_content);
        m_deleteBtn = new QPushButton("删除", m_content);
        m_deleteBtn->setObjectName("DeleteBtn");
        m_cancelBtn = new QPushButton("取消", m_content);
        m_cancelBtn->setObjectName("CancelBtn");

        contentLayout->addWidget(m_editBtn);
        contentLayout->addWidget(m_deleteBtn);
        contentLayout->addWidget(m_cancelBtn);

        connect(m_editBtn, &QPushButton::clicked, this, [this](){ m_result = Edit; accept(); });
        connect(m_deleteBtn, &QPushButton::clicked, this, [this](){ m_result = Delete; accept(); });
        connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        // 彻底移除 QGraphicsDropShadowEffect，这是导致 UpdateLayeredWindowIndirect 报错的主因
    }

    enum Result { Cancel, Edit, Delete };
    Result getResult() const { return m_result; }

protected:
    void showEvent(QShowEvent *event) override {
        updateGeometry();
        QDialog::showEvent(event);
        
        // 弹出动画
        int contentHeight = m_content->sizeHint().height();
        // 动画期间将内容宽度设置为与窗口一致，不再留边距以简化渲染区域
        m_content->resize(width(), contentHeight);
        m_content->move(0, height());
        
        QPropertyAnimation *animation = new QPropertyAnimation(m_content, "pos");
        animation->setDuration(250);
        animation->setStartValue(QPoint(0, height()));
        animation->setEndValue(QPoint(0, height() - contentHeight));
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void resizeEvent(QResizeEvent *event) override {
        QDialog::resizeEvent(event);
        m_mask->setGeometry(0, 0, width(), height());
        
        int contentHeight = m_content->height();
        if (contentHeight > 0) {
            // 确保内容层在调整大小时也保持在正确的底部位置
            m_content->setGeometry(0, height() - contentHeight, width(), contentHeight);
        }
    }

    void mousePressEvent(QMouseEvent *event) override {
        // 点击遮罩区域（非内容区域）关闭弹窗
        if (!m_content->geometry().contains(event->pos())) {
            reject();
        }
    }

private:
    void updateGeometry() {
        if (parentWidget()) {
            // 获取父窗口的全局坐标和大小
            QWidget *topLevel = parentWidget()->window();
            if (topLevel) {
                setGeometry(topLevel->geometry());
            }
        }
    }

    QWidget *m_mask;
    QWidget *m_content;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_cancelBtn;
    Result m_result = Cancel;
};

// 静态辅助函数：解析日期时间字符串，支持多种格式
static QDateTime parseDateTime(const QString& dateTimeStr) {
    QDateTime dt = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");
    if (!dt.isValid()) {
        dt = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm");
    }
    if (!dt.isValid()) {
        dt = QDateTime::fromString(dateTimeStr, Qt::ISODate);
    }
    if (!dt.isValid()) {
        // 如果还是无效，尝试只解析日期
        QDate date = QDate::fromString(dateTimeStr, "yyyy-MM-dd");
        if (date.isValid()) {
            dt = QDateTime(date, QTime(0, 0));
        }
    }
    return dt;
}

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
    
    // 确保在 UserManager 设置用户后再加载数据，或者延迟加载
    // 这里我们直接调用，但在 main.cpp 中 connect 确保 loginSuccess 后设置用户
    loadBillsForMonth();

    // 监听用户切换/登录成功信号以重新加载数据
    connect(UserManager::getInstance(), &UserManager::userChanged, this, [this](){
        qDebug() << "检测到用户切换，重新加载账单数据...";
        loadBillsForMonth();
    });

    // 右下角加号：打开独立记账窗口（单独的界面）
    connect(m_addBtn, &QPushButton::clicked, this, [this]() {
        // 使用 QDialog::exec() 以模态方式运行，确保流程同步
        AccountBookRecordWidget dialog(this);
        
        // 记账完成后刷新列表
        connect(&dialog, &AccountBookRecordWidget::billRecorded, this, [this](){
            qDebug() << "主界面接收到 billRecorded 信号，正在刷新列表...";
            this->loadBillsForMonth();
        });
        
        dialog.exec();
    });

    // 开启自动同步功能
    SyncManager::getInstance()->startAutoSync();
    
    // 连接同步更新信号，同步完成后自动刷新界面
    connect(SyncManager::getInstance(), &SyncManager::dataUpdated, this, [this](){
        loadBillsForMonth();
    });
}

void AccountBookMainWidget::initUI()
{
    // 主布局
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget();
    
    // --- 账本页面 ---
    m_bookPage = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_bookPage);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 顶部：用户头像按钮 + 搜索框
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    m_userBtn = new QPushButton();
    m_userBtn->setFixedSize(35, 35);
    m_userBtn->setCursor(Qt::PointingHandCursor);
    m_userBtn->setObjectName("m_userBtn");
    
    // 绘制一个白底白边蓝图标的用户头像样式
    m_userBtn->setStyleSheet(R"(
        QPushButton#m_userBtn {
            background-color: white;
            border-radius: 17px;
            color: #007AFF;
            font-size: 22px;
            font-weight: bold;
            border: 2px solid white;
        }
        QPushButton#m_userBtn:hover {
            background-color: #F8F9FA;
            color: #0063CC;
        }
    )");
    m_userBtn->setText("👤"); 
    topBarLayout->addWidget(m_userBtn);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索分类、备注...");
    m_searchEdit->setFixedHeight(35);
    topBarLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(topBarLayout);

    // 连接搜索框信号
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AccountBookMainWidget::onSearchTextChanged);
    connect(m_userBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onUserBtnClicked);

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
    statLayout->setSpacing(10);

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

    // 连接账单点击信号，用于编辑或删除
    connect(m_billListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        // 检查是否是抬头（不可选中项通常没有 UserRole 数据）
        QVariant idVar = item->data(Qt::UserRole);
        if (!idVar.isValid()) return;

        int recordId = idVar.toInt();

        // 使用美化的 ActionSheet 代替 QMessageBox
        ActionSheet sheet(this->window());
        if (sheet.exec() == QDialog::Accepted) {
            ActionSheet::Result res = sheet.getResult();
            if (res == ActionSheet::Edit) {
                // --- 编辑逻辑 ---
                QString jsonStr = item->data(Qt::UserRole + 1).toString();
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject obj = doc.object();

                AccountRecord record;
                record.setId(recordId);
                record.setUserId(obj["userId"].toInt());
                record.setAmount(obj["amount"].toDouble());
                record.setType(obj["type"].toString());
                record.setRemark(obj["remark"].toString());
                record.setCreateTime(obj["createTime"].toString());

                // 使用 QDialog::exec() 以模态方式运行
                AccountBookRecordWidget dialog(this);
                dialog.setRecord(record); // 设置为编辑模式

                connect(&dialog, &AccountBookRecordWidget::billRecorded, this, [this](){
                    this->loadBillsForMonth();
                });

                dialog.exec();
            } 
            else if (res == ActionSheet::Delete) {
                // --- 删除逻辑 ---
                if (QMessageBox::question(this, "确认删除", "确定要删除这条账单记录吗？", 
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    if (BillService::deleteBill(recordId)) {
                        this->loadBillsForMonth(); // 刷新列表
                    }
                }
            }
        }
    });

    m_stackedWidget->addWidget(m_bookPage);
    
        // --- 资产页面 (优化版) ---
    m_assetPage = new QWidget();
    m_assetPage->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F0F2F5, stop:1 #FFFFFF);");
    QVBoxLayout *assetLayout = new QVBoxLayout(m_assetPage);
    assetLayout->setContentsMargins(20, 40, 20, 20);
    assetLayout->setSpacing(20);

    // 资产总览卡片
    QFrame *assetCard = new QFrame();
    assetCard->setFixedHeight(150);
    assetCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4A90E2, stop:1 #357ABD);
            border-radius: 20px;
        }
    )");
    QVBoxLayout *cardLayout = new QVBoxLayout(assetCard);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel *assetTitle = new QLabel("净资产 (元)");
    assetTitle->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 14px;");
    
    QLabel *assetAmount = new QLabel("¥ 0.00");
    assetAmount->setStyleSheet("color: white; font-size: 32px; font-weight: bold;");
    
    cardLayout->addWidget(assetTitle);
    cardLayout->addWidget(assetAmount);
    cardLayout->addStretch();
    
    assetLayout->addWidget(assetCard);

    // 功能模块占位
    QLabel *assetMsg = new QLabel("资产管理模块建设中...");
    assetMsg->setAlignment(Qt::AlignCenter);
    assetMsg->setStyleSheet("color: #999; font-size: 16px; margin-top: 40px;");
    assetLayout->addWidget(assetMsg);

    QLabel *assetHint = new QLabel("即将支持：账户管理、理财统计、借贷管理等");
    assetHint->setAlignment(Qt::AlignCenter);
    assetHint->setStyleSheet("color: #BBB; font-size: 13px;");
    assetLayout->addWidget(assetHint);
    
    assetLayout->addStretch();

    m_stackedWidget->addWidget(m_assetPage);
    
        // --- 统计页面 ---
        m_statisticsPage = new StatisticsWidget();
        m_stackedWidget->addWidget(m_statisticsPage);

    outerLayout->addWidget(m_stackedWidget);

    // 底部导航
    QFrame *navBar = new QFrame();
    navBar->setFixedHeight(60);
    navBar->setStyleSheet("background-color: white; border-top: 1px solid rgba(0,0,0,0.1);");
    QHBoxLayout *navLayout = new QHBoxLayout(navBar);
    
    m_bookNavBtn = new QPushButton("账本");
    m_assetNavBtn = new QPushButton("资产");
    m_statNavBtn = new QPushButton("统计");
    
    m_bookNavBtn->setCheckable(true);
    m_assetNavBtn->setCheckable(true);
    m_statNavBtn->setCheckable(true);
    m_bookNavBtn->setChecked(true);
    
    m_bookNavBtn->setObjectName("navBtn");
    m_assetNavBtn->setObjectName("navBtn");
    m_statNavBtn->setObjectName("navBtn");

    navLayout->addWidget(m_bookNavBtn);
    navLayout->addWidget(m_assetNavBtn);
    navLayout->addWidget(m_statNavBtn);
    outerLayout->addWidget(navBar);

    connect(m_bookNavBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onNavButtonClicked);
    connect(m_assetNavBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onNavButtonClicked);
    connect(m_statNavBtn, &QPushButton::clicked, this, &AccountBookMainWidget::onNavButtonClicked);

    // 右下角加号按钮
    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedSize(40, 40);
    QFont addFont;
    addFont.setPointSize(24);
    m_addBtn->setFont(addFont);
    m_addBtn->setParent(this);
    m_addBtn->move(width() - 80, height() - 110);
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
void AccountBookMainWidget::updateBillData(const QList<AccountRecord>& records)
{
    m_billListWidget->clear();

    if (records.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem(m_billListWidget);
        QWidget *emptyWidget = new QWidget();
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
    QMap<QString, QList<AccountRecord>> groupedBills;
    QStringList dateOrder; // 保持日期顺序
    double totalExpense = 0.0;
    double totalIncome = 0.0;

    for (const AccountRecord& record : records) {
        QDateTime dt = parseDateTime(record.getCreateTime());
        QString dateKey = dt.isValid() ? (dt.toString("MM/dd ") + dt.date().toString("ddd")) : "未知日期";
        
        if (!groupedBills.contains(dateKey)) {
            dateOrder.append(dateKey);
        }
        groupedBills[dateKey].append(record);
        
        double amount = record.getAmount();
        if (amount < 0) totalExpense += qAbs(amount);
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

        // 计算该日支出
        double dayExpense = 0;
        for (const AccountRecord& r : groupedBills[date]) {
            if (r.getAmount() < 0) dayExpense += qAbs(r.getAmount());
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
        for (const AccountRecord& record : groupedBills[date]) {
            double amount = qAbs(record.getAmount());
            bool isExpense = (record.getAmount() < 0);
            QString cateName = record.getType();
            QDateTime dt = parseDateTime(record.getCreateTime());
            QString timeStr = dt.isValid() ? dt.toString("HH:mm") : "--:--";

            QListWidgetItem *item = new QListWidgetItem(m_billListWidget);
            // 存储记录ID，用于点击跳转编辑
            item->setData(Qt::UserRole, record.getId());
            // 存储完整记录 JSON，方便直接获取（如果数据量不大）
            QJsonObject obj;
            obj["id"] = record.getId();
            obj["userId"] = record.getUserId();
            obj["amount"] = record.getAmount();
            obj["type"] = record.getType();
            obj["remark"] = record.getRemark();
            obj["createTime"] = record.getCreateTime();
            item->setData(Qt::UserRole + 1, QJsonDocument(obj).toJson());
            
            // 外层容器，负责边距
            QWidget *container = new QWidget();
            container->setAttribute(Qt::WA_TransparentForMouseEvents); // 让鼠标事件穿透到 QListWidget
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
                iconLabel->setText(cateName.left(1));
                iconLabel->setAlignment(Qt::AlignCenter);
                iconLabel->setStyleSheet(QString("background-color: %1; border-radius: 20px; color: white; font-weight: bold;")
                                         .arg(isExpense ? "#FF6B6B" : "#4CAF50"));
            }
            itemLayout->addWidget(iconLabel);

            // 信息列
            QVBoxLayout *infoLayout = new QVBoxLayout();
            infoLayout->setSpacing(2);
            
            QHBoxLayout *nameRow = new QHBoxLayout();
            nameRow->setSpacing(8);
            
            QLabel *nameLabel = new QLabel(cateName);
            nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
            nameRow->addWidget(nameLabel);
            
            QString remark = record.getRemark();
            if (!remark.isEmpty()) {
                QLabel *remarkLabel = new QLabel("(" + remark + ")");
                remarkLabel->setStyleSheet("font-size: 12px; color: #666; font-style: italic;");
                nameRow->addWidget(remarkLabel);
            }
            nameRow->addStretch();
            
            infoLayout->addLayout(nameRow);

            QLabel *timeLabel = new QLabel(timeStr);
            timeLabel->setStyleSheet("font-size: 11px; color: #999;");
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
    updateBillData(records);
}

bool AccountBookMainWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_monthLabel && event->type() == QEvent::MouseButtonPress) {
        onMonthLabelClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void AccountBookMainWidget::onSearchTextChanged(const QString &text)
{
    int userId = UserManager::getInstance()->getCurrentUser().getId();
    if (userId <= 0) return;

    // 获取当月所有账单
    QList<AccountRecord> records = BillService::getMonthlyBills(userId, m_currentDate);
    QList<AccountRecord> filteredList;

    QString keyword = text.trimmed().toLower();

    for (const AccountRecord& record : records) {
        // 搜索分类名或备注
        QString category = record.getType().toLower();
        QString remark = record.getRemark().toLower();

        if (keyword.isEmpty() || category.contains(keyword) || remark.contains(keyword)) {
            filteredList.append(record);
        }
    }

    // 更新列表显示
    updateBillData(filteredList);
}

void AccountBookMainWidget::onNavButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    // 更新按钮选中状态
    m_bookNavBtn->setChecked(btn == m_bookNavBtn);
    m_assetNavBtn->setChecked(btn == m_assetNavBtn);
    m_statNavBtn->setChecked(btn == m_statNavBtn);

    if (btn == m_bookNavBtn) {
        m_stackedWidget->setCurrentWidget(m_bookPage);
        m_addBtn->show();
    } else if (btn == m_assetNavBtn) {
        m_stackedWidget->setCurrentWidget(m_assetPage);
        m_addBtn->hide();
    } else if (btn == m_statNavBtn) {
        m_stackedWidget->setCurrentWidget(m_statisticsPage);
        m_addBtn->hide();
        // 从 UserManager 获取当前用户 ID
        int userId = UserManager::getInstance()->getCurrentUser().getId();
        m_statisticsPage->updateData(userId, m_currentDate.year(), m_currentDate.month());
    }
}

void AccountBookMainWidget::onUserBtnClicked()
{
    SettingsWidget *settings = new SettingsWidget();
    settings->setAttribute(Qt::WA_DeleteOnClose);
    settings->show();
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
        QPushButton#navBtn {
            color: #666;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#navBtn:checked {
            color: #FFD700;
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
