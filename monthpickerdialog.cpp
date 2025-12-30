#include "monthpickerdialog.h"
#include <QScrollBar>

MonthPickerDialog::MonthPickerDialog(const QDate &currentDate, QWidget *parent)
    : QDialog(parent), m_selectedDate(currentDate), m_initialDate(currentDate)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 600);
    initUI();
}

void MonthPickerDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *container = new QFrame();
    container->setObjectName("container");
    container->setStyleSheet(R"(
        QFrame#container {
            background-color: white;
            border-top-left-radius: 20px;
            border-top-right-radius: 20px;
            border-bottom-left-radius: 20px;
            border-bottom-right-radius: 20px;
        }
    )");
    
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // 顶部栏
    QFrame *topBar = new QFrame();
    topBar->setFixedHeight(50);
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    
    QPushButton *closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(40, 40);
    closeBtn->setStyleSheet("border: none; font-size: 18px; color: #333;");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    QLabel *titleLabel = new QLabel("选择时间");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");
    
    QPushButton *currentMonthBtn = new QPushButton("本月");
    currentMonthBtn->setFixedSize(60, 40);
    currentMonthBtn->setStyleSheet("border: none; font-size: 14px; color: #007AFF;");
    connect(currentMonthBtn, &QPushButton::clicked, this, &MonthPickerDialog::onCurrentMonthClicked);
    
    topLayout->addWidget(closeBtn);
    topLayout->addWidget(titleLabel, 1);
    topLayout->addWidget(currentMonthBtn);
    
    containerLayout->addWidget(topBar);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent;");
    scrollArea->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical { width: 0px; }
    )");

    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 10, 20, 20);
    scrollLayout->setSpacing(20);

    // 添加 2024, 2025, 2026 年份（可以根据需要动态调整）
    int currentYear = QDate::currentDate().year();
    for (int year = currentYear - 1; year <= currentYear + 1; ++year) {
        addYearSection(year, scrollLayout);
    }
    
    scrollArea->setWidget(scrollContent);
    containerLayout->addWidget(scrollArea);

    mainLayout->addWidget(container);
}

void MonthPickerDialog::addYearSection(int year, QVBoxLayout *layout)
{
    QLabel *yearLabel = new QLabel(QString("%1年").arg(year));
    yearLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; margin-top: 10px;");
    layout->addWidget(yearLabel);

    QGridLayout *monthGrid = new QGridLayout();
    monthGrid->setSpacing(10);
    monthGrid->setContentsMargins(0, 5, 0, 5);

    for (int i = 0; i < 12; ++i) {
        int month = i + 1;
        QPushButton *monthBtn = new QPushButton(QString("%1月").arg(month));
        monthBtn->setFixedSize(65, 40);
        
        bool isSelected = (m_selectedDate.year() == year && m_selectedDate.month() == month);
        
        QString style = R"(
            QPushButton {
                background-color: #F5F5F7;
                border: none;
                border-radius: 10px;
                font-size: 14px;
                color: #333;
            }
            QPushButton:hover {
                background-color: #E5E5EA;
            }
        )";
        
        if (isSelected) {
            style += "QPushButton { color: #007AFF; font-weight: bold; }";
        }
        
        monthBtn->setStyleSheet(style);
        monthBtn->setProperty("year", year);
        monthBtn->setProperty("month", month);
        
        connect(monthBtn, &QPushButton::clicked, this, &MonthPickerDialog::onMonthSelected);
        
        monthGrid->addWidget(monthBtn, i / 5, i % 5);
    }

    layout->addLayout(monthGrid);
}

void MonthPickerDialog::onMonthSelected()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        int year = btn->property("year").toInt();
        int month = btn->property("month").toInt();
        m_selectedDate = QDate(year, month, 1);
        accept();
    }
}

void MonthPickerDialog::onCurrentMonthClicked()
{
    m_selectedDate = QDate::currentDate();
    m_selectedDate.setDate(m_selectedDate.year(), m_selectedDate.month(), 1);
    accept();
}
