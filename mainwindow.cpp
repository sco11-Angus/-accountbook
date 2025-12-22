#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("红果记账系统"); // 窗口标题
    this->setStyleSheet("MainWindow { background: transparent; }");

    // 1. 创建账本主界面实例
    m_bookWidget = new AccountBookMainWidget(this);

    // 2. 布局适配：将账本界面设为中心部件，自动填充窗口
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0); // 无间距，完全适配
    centralLayout->addWidget(m_bookWidget);
    this->setCentralWidget(centralWidget);

    // 3. 固定窗口大小（匹配账本界面的450×650，避免拉伸变形）
    this->setFixedSize(m_bookWidget->size());
}

MainWindow::~MainWindow()
{
    delete ui;
}
