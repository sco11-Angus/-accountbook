#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "user_manager.h"
#include "account_manager.h"
#include "AccountBookRecordWidget.h"
#include <QMessageBox>
#include <QDebug>
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
    QObject::connect(m_bookWidget->m_addBtn, &QPushButton::clicked, this, [this]() {
        // 1. 校验当前是否有登录用户
        UserManager* userManager = UserManager::getInstance();
        int currentUserId = userManager->getCurrentUser().getId();

        // 2. 未登录则提示并拦截
        if (currentUserId <= 0) {
            QMessageBox::warning(this, "操作提示", "请先登录账号，再进行记账操作~");
            return; // 终止后续流程，不显示记账界面
        }

        // 3. 已登录则显示记账界面
        AccountBookRecordWidget recordWidget;
        // 设置窗口为“应用级模态”（整个程序都等这个窗口关闭）
        recordWidget.setWindowModality(Qt::ApplicationModal);
        recordWidget.show();
    });

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
