#ifndef BILL_SYNC_CLIENT_H
#define BILL_SYNC_CLIENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
class QNetworkAccessManager;
class QNetworkReply;
#include "account_record.h"
#include "User.h"

class BillSyncClient : public QObject
{
    Q_OBJECT
public:
    explicit BillSyncClient(QObject *parent = nullptr);

    void initSyncConfig();
    void setServerUrl(const QString& url);
    void setCurrentUser(const User& user);
    void syncRecord(const AccountRecord& record);
    void pullLatestRecords(int userId, const QString& lastSyncTime);
    void startAutoSync(int intervalSeconds = 60);
    void stopAutoSync();

signals:
    void syncSuccess(const AccountRecord& record);
    void syncFailed(const AccountRecord& record, const QString& reason);
    void recordsPulled(const QList<AccountRecord>& records);
    void syncStatusChanged(bool isSyncing);

private slots:
    void onSyncTimerTimeout();
    void onNetworkReplyFinished(QNetworkReply* reply);  // 补充参数

private:
    QNetworkAccessManager* m_networkManager;  // 指针类型，前向声明即可
    QString m_serverUrl;
    User m_currentUser;
    QTimer* m_syncTimer;
    bool m_isSyncing;

    // 内部辅助方法声明
    QByteArray buildSyncJson(const AccountRecord& record);
    QList<AccountRecord> parseRecordsFromJson(const QByteArray& json);
};

#endif // BILL_SYNC_CLIENT_H
