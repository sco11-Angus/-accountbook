#include "database_example.h"
#include "db_models.h"
#include <QDebug>
#include <QDateTime>
#include <QDate>

void DatabaseExample::exampleCreateUserAndBills() {
    qDebug() << "\n========== 示例1：创建用户并添加账单 ==========";
    
    DBManager* db = DBManager::getInstance();
    
    // 初始化数据库
    if (!db->initialize("./account_book.db")) {
        qCritical() << "数据库初始化失败:" << db->getLastError();
        return;
    }
    
    // 1. 创建用户
    UserData user;
    user.account = "user@example.com";
    user.password = "encrypted_password";
    user.nickname = "张三";
    user.gender = 1;  // 1=男
    user.payMethod = "支付宝";
    
    int userId = db->addUser(user);
    qDebug() << "新增用户成功，ID:" << userId;
    
    // 2. 创建账本
    AccountBookData book;
    book.userId = userId;
    book.name = "日常开支";
    book.description = "记录日常生活中的收支";
    book.icon = "💰";
    book.sortOrder = 1;
    
    int bookId = db->addAccountBook(book);
    qDebug() << "新增账本成功，ID:" << bookId;
    
    // 3. 创建分类
    BillCategoryData category;
    category.userId = userId;
    category.name = "餐饮";
    category.type = 0;  // 支出
    category.icon = "🍽️";
    category.color = "#FF6B6B";
    category.sortOrder = 1;
    
    int categoryId = db->addBillCategory(category);
    qDebug() << "新增分类成功，ID:" << categoryId;
    
    // 4. 添加多条账单
    for (int i = 0; i < 5; ++i) {
        BillData bill;
        bill.userId = userId;
        bill.bookId = bookId;
        bill.categoryId = categoryId;
        bill.date = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        bill.amount = 30.0 + i * 10;
        bill.type = 0;  // 支出
        bill.description = QString("餐饮消费 #%1").arg(i + 1);
        bill.paymentMethod = "微信";
        bill.merchant = "餐厅名称";
        
        int billId = db->addBill(bill);
        qDebug() << "新增账单" << (i + 1) << "成功，ID:" << billId;
    }
}

void DatabaseExample::exampleMonthlyStatistics() {
    qDebug() << "\n========== 示例2：获取月度统计 ==========";
    
    DBManager* db = DBManager::getInstance();
    int userId = 1;  // 假设用户ID为1
    
    // 获取当前月份
    QDate today = QDate::currentDate();
    int year = today.year();
    int month = today.month();
    
    // 获取本月账单
    QList<BillData> bills = db->getBillByMonth(userId, year, month);
    qDebug() << "本月账单总数:" << bills.size();
    
    // 获取分类统计
    BillQueryResult result = db->getBillByCategoryStats(userId, year, month);
    
    qDebug() << "本月收入:" << result.totalIncome;
    qDebug() << "本月支出:" << result.totalExpense;
    qDebug() << "净金额:" << result.netAmount;
    
    // 按分类统计
    qDebug() << "分类统计:";
    for (auto it = result.categoryStats.begin(); it != result.categoryStats.end(); ++it) {
        qDebug() << "  " << it.key() << ":" << it.value();
    }
    
    // 按支付方式统计
    qDebug() << "支付方式统计:";
    for (auto it = result.methodStats.begin(); it != result.methodStats.end(); ++it) {
        qDebug() << "  " << it.key() << ":" << it.value();
    }
    
    // 显示具体账单
    qDebug() << "\n详细账单:";
    for (const BillData& bill : bills) {
        QString type = bill.type == 0 ? "支出" : "收入";
        qDebug() << bill.date << "|" << type << "|" 
                 << bill.amount << "|" << bill.description;
    }
}

void DatabaseExample::exampleCategoryManagement() {
    qDebug() << "\n========== 示例3：分类管理 ==========";
    
    DBManager* db = DBManager::getInstance();
    int userId = 1;
    
    // 1. 添加多个分类
    QStringList categoryNames = {"餐饮", "交通", "购物", "娱乐", "工作", "投资"};
    QStringList categoryIcons = {"🍽️", "🚗", "🛍️", "🎬", "💼", "💹"};
    QStringList categoryColors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8", "#F7DC6F"};
    
    for (int i = 0; i < categoryNames.size(); ++i) {
        BillCategoryData category;
        category.userId = userId;
        category.name = categoryNames[i];
        category.type = 0;  // 支出
        category.icon = categoryIcons[i];
        category.color = categoryColors[i];
        category.sortOrder = i;
        
        int categoryId = db->addBillCategory(category);
        qDebug() << "添加分类:" << categoryNames[i] << "ID:" << categoryId;
    }
    
    // 2. 查询所有支出分类
    QList<BillCategoryData> expenseCategories = db->getBillCategories(userId, 0);
    qDebug() << "\n支出分类总数:" << expenseCategories.size();
    
    // 3. 查询所有收入分类
    QList<BillCategoryData> incomeCategories = db->getBillCategories(userId, 1);
    qDebug() << "收入分类总数:" << incomeCategories.size();
    
    // 4. 更新分类（修改颜色）
    if (!expenseCategories.isEmpty()) {
        BillCategoryData& category = expenseCategories.first();
        category.color = "#9B59B6";  // 改为紫色
        db->updateBillCategory(category);
        qDebug() << "更新分类颜色成功";
    }
    
    // 5. 删除分类
    if (!expenseCategories.isEmpty()) {
        // db->deleteBillCategory(expenseCategories.first().id);
        qDebug() << "删除分类操作已注释";
    }
}

void DatabaseExample::exampleAccountBookManagement() {
    qDebug() << "\n========== 示例4：账本管理 ==========";
    
    DBManager* db = DBManager::getInstance();
    int userId = 1;
    
    // 1. 创建多个账本
    QStringList bookNames = {"日常开支", "投资账本", "家庭预算", "旅游开支"};
    QStringList bookIcons = {"💰", "💹", "🏠", "✈️"};
    
    for (int i = 0; i < bookNames.size(); ++i) {
        AccountBookData book;
        book.userId = userId;
        book.name = bookNames[i];
        book.description = QString("%1 - 详细描述").arg(bookNames[i]);
        book.icon = bookIcons[i];
        book.sortOrder = i;
        
        int bookId = db->addAccountBook(book);
        qDebug() << "添加账本:" << bookNames[i] << "ID:" << bookId;
    }
    
    // 2. 查询所有账本
    QList<AccountBookData> books = db->getAccountBooks(userId);
    qDebug() << "\n用户账本总数:" << books.size();
    
    for (const AccountBookData& book : books) {
        qDebug() << "  " << book.icon << book.name << "(" << book.id << ")";
    }
    
    // 3. 查询单个账本
    if (!books.isEmpty()) {
        AccountBookData book = db->getAccountBookById(books.first().id);
        qDebug() << "\n查询到账本:" << book.name;
    }
}

void DatabaseExample::exampleDateRangeQuery() {
    qDebug() << "\n========== 示例5：日期范围查询 ==========";
    
    DBManager* db = DBManager::getInstance();
    int userId = 1;
    
    // 查询12月份的账单
    QString startDate = "2024-12-01";
    QString endDate = "2024-12-31";
    
    QList<BillData> bills = db->getBillByDateRange(userId, startDate, endDate);
    qDebug() << "查询范围:" << startDate << "到" << endDate;
    qDebug() << "符合条件的账单数:" << bills.size();
    
    // 按日期排序并统计
    QMap<QString, double> dailyStats;
    double totalAmount = 0;
    
    for (const BillData& bill : bills) {
        QString date = bill.date.left(10);  // 提取日期部分
        dailyStats[date] += bill.amount;
        totalAmount += bill.amount;
    }
    
    qDebug() << "日期统计:";
    for (auto it = dailyStats.begin(); it != dailyStats.end(); ++it) {
        qDebug() << "  " << it.key() << ":" << it.value();
    }
    
    qDebug() << "总计:" << totalAmount;
}

void DatabaseExample::exampleDeleteAndRecovery() {
    qDebug() << "\n========== 示例6：删除与恢复 ==========";
    
    DBManager* db = DBManager::getInstance();
    
    // 获取一条账单
    BillData bill = db->getBillById(1);
    if (bill.id > 0) {
        qDebug() << "原始账单:" << bill.description << "金额:" << bill.amount;
        
        // 软删除（标记为已删除，但不删除数据）
        if (db->deleteBill(bill.id)) {
            qDebug() << "软删除成功 - 账单被标记为已删除";
        }
        
        // 重新查询，已删除的账单不会显示
        BillData deletedBill = db->getBillById(bill.id);
        if (deletedBill.id == 0) {
            qDebug() << "已删除的账单无法查询";
        }
        
        // 硬删除（永久删除）
        // db->permanentlyDeleteBill(bill.id);
        qDebug() << "硬删除操作已注释（使用时需谨慎）";
    }
}

void DatabaseExample::exampleSyncQueue() {
    qDebug() << "\n========== 示例7：同步队列 ==========";
    
    DBManager* db = DBManager::getInstance();
    int userId = 1;
    
    // 检查是否连接到远程数据库
    if (!db->isRemoteConnected()) {
        qWarning() << "未连接到远程数据库，无法演示同步";
        
        // 连接到远程数据库
        if (db->connectRemoteDatabase("localhost", 3306, "root", "password", "account_book")) {
            qDebug() << "远程数据库已连接";
        } else {
            qWarning() << "连接失败:" << db->getLastError();
            return;
        }
    }
    
    // 获取待同步项
    QList<SyncQueueItem> pendingItems = db->getPendingSyncItems(userId, 50);
    qDebug() << "待同步项总数:" << pendingItems.size();
    
    for (const SyncQueueItem& item : pendingItems) {
        QString statusStr;
        switch (item.status) {
            case 0: statusStr = "待同步"; break;
            case 1: statusStr = "同步中"; break;
            case 2: statusStr = "已同步"; break;
            case 3: statusStr = "同步失败"; break;
            default: statusStr = "未知";
        }
        
        qDebug() << "  类型:" << item.entityType 
                 << "操作:" << item.operation 
                 << "状态:" << statusStr 
                 << "重试:" << item.retryCount;
    }
    
    // 获取同步统计
    SyncStatistics stats = db->getSyncStatistics(userId);
    qDebug() << "\n同步统计:";
    qDebug() << "  待同步:" << stats.pendingCount;
    qDebug() << "  已同步:" << stats.successCount;
    qDebug() << "  失败:" << stats.failureCount;
    qDebug() << "  最后同步时间:" << stats.lastSyncTime;
    qDebug() << "  正在同步:" << (stats.isSyncing ? "是" : "否");
}

void DatabaseExample::exampleErrorHandling() {
    qDebug() << "\n========== 示例8：错误处理 ==========";
    
    DBManager* db = DBManager::getInstance();
    
    // 尝试查询不存在的用户
    UserData user = db->getUserById(9999);
    if (user.id == 0) {
        qWarning() << "用户不存在，查询结果为空";
    }
    
    // 尝试添加重复的账户
    UserData duplicateUser;
    duplicateUser.account = "existing@example.com";
    duplicateUser.password = "password";
    duplicateUser.nickname = "测试用户";
    
    int userId = db->addUser(duplicateUser);
    if (userId <= 0) {
        qWarning() << "添加用户失败:" << db->getLastError();
    }
    
    // 检查返回值
    BillData invalidBill;
    invalidBill.userId = -1;  // 无效的用户ID
    invalidBill.amount = 100;
    
    int billId = db->addBill(invalidBill);
    if (billId <= 0) {
        QString error = db->getLastError();
        qWarning() << "添加账单失败，错误信息:" << error;
    }
    
    qDebug() << "错误处理演示完成";
}
