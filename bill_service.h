#ifndef BILL_SERVICE_H
#define BILL_SERVICE_H

#include <QObject>
#include <QList>
#include "account_record.h"

class BillService : public QObject {
    Q_OBJECT

public:
    // 保存账单（本地+服务端同步）
    static bool saveBill(const AccountRecord& record);

    // 更新账单（本地+服务端同步）
    static bool updateBill(const AccountRecord& record);

    // 删除账单（本地+服务端同步）
    static bool deleteBill(int recordId);

    // 获取指定月份账单列表
    static QList<AccountRecord> getMonthlyBills(int userId, const QDate& date);

    // 获取当前月份账单列表
    static QList<AccountRecord> getCurrentMonthBills(int userId);

    // 获取单例实例（用于信号发射）
    static BillService* getInstance();
signals:
    // 账单保存结果信号
    void billSaved(bool success, const QString& message);

    // 账单同步状态变化信号
    void billSyncStatusChanged(int billId, bool synced);

private:
    // 私有构造函数（单例）
    explicit BillService(QObject* parent = nullptr);

};

#endif // BILL_SERVICE_H
