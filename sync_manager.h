#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "tcpclient.h"
#include "account_manager.h"

/**
 * @brief 同步管理器 - 负责本地数据与服务端的同步逻辑，支持定时同步和增量更新
 */
class SyncManager : public QObject
{
    Q_OBJECT
public:
    static SyncManager* getInstance();

    // 开启自动同步（每隔 interval 毫秒）
    void startAutoSync(int interval = 300000); // 默认5分钟
    void stopAutoSync();

    // 执行一次全量/增量同步
    void triggerSync();

signals:
    void syncStarted();
    void syncFinished(bool success, const QString& message);
    void dataUpdated(); // 同步完成后通知 UI 更新

private slots:
    void onTimerTimeout();
    void onSyncResponse(bool success, const QString& message);
    void onNetworkError(const QString& error);

private:
    explicit SyncManager(QObject *parent = nullptr);
    static SyncManager* m_instance;

    QTimer* m_syncTimer;
    bool m_isSyncing;
    QDateTime m_lastSyncTime;

    void performSyncTask();
};

#endif // SYNC_MANAGER_H
