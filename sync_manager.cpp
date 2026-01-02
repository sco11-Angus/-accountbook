#include "sync_manager.h"
#include "thread_manager.h"
#include "user_manager.h"
#include "tcpclient.h"
#include "account_manager.h"
#include <QDebug>

SyncManager* SyncManager::m_instance = nullptr;

SyncManager* SyncManager::getInstance() {
    if (!m_instance) {
        m_instance = new SyncManager();
    }
    return m_instance;
}

SyncManager::SyncManager(QObject *parent) 
    : QObject(parent), m_isSyncing(false) 
{
    m_syncTimer = new QTimer(this);
    connect(m_syncTimer, &QTimer::timeout, this, &SyncManager::onTimerTimeout);
    
    // 监听 TcpClient 的同步响应
    connect(TcpClient::getInstance(), &TcpClient::syncBillsResponse, 
            this, &SyncManager::onSyncResponse);
    connect(TcpClient::getInstance(), &TcpClient::errorOccurred, 
            this, &SyncManager::onNetworkError);
}

void SyncManager::startAutoSync(int interval) {
    m_syncTimer->start(interval);
    qDebug() << "自动同步已开启，间隔:" << interval / 1000 << "秒";
}

void SyncManager::stopAutoSync() {
    m_syncTimer->stop();
    qDebug() << "自动同步已停止";
}

void SyncManager::triggerSync() {
    if (m_isSyncing) return;
    performSyncTask();
}

void SyncManager::onTimerTimeout() {
    triggerSync();
}

void SyncManager::performSyncTask() {
    User user = UserManager::getInstance()->getCurrentUser();
    if (user.getId() <= 0) return;

    m_isSyncing = true;
    emit syncStarted();

    // 使用 ThreadManager 在后台线程获取待同步数据
    ThreadManager::getInstance()->runAsync([user]() {
        AccountManager am;
        // 获取所有本地未同步或需要更新的数据（此处简化为获取该用户所有记录）
        QList<AccountRecord> records = am.queryAccountRecord(user.getId());
        
        // 切回主线程（或直接通过单例 TcpClient 发送，TcpClient 内部处理异步网络）
        QMetaObject::invokeMethod(TcpClient::getInstance(), [records]() {
            TcpClient::getInstance()->syncBills(records);
        });
    });
}

void SyncManager::onSyncResponse(bool success, const QString& message) {
    m_isSyncing = false;
    if (success) {
        m_lastSyncTime = QDateTime::currentDateTime();
        qDebug() << "同步成功:" << message;
        emit dataUpdated();
    } else {
        qDebug() << "同步失败:" << message;
    }
    emit syncFinished(success, message);
}

void SyncManager::onNetworkError(const QString& error) {
    m_isSyncing = false;
    qDebug() << "同步网络错误:" << error;
    emit syncFinished(false, error);
}
