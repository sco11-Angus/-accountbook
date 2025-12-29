#include "bill_sync_client.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUrl>

BillSyncClient::BillSyncClient(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);  // 初始化网络管理器
    m_syncTimer = new QTimer(this);
    m_isSyncing = false;

    // 关联信号槽
    connect(m_syncTimer, &QTimer::timeout, this, &BillSyncClient::onSyncTimerTimeout);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &BillSyncClient::onNetworkReplyFinished);
}

void BillSyncClient::initSyncConfig() {
    // 初始化同步配置（如默认服务器地址）
    m_serverUrl = "http://127.0.0.1:8080";  // 本地测试地址
}

// 同步方法实现
void BillSyncClient::setServerUrl(const QString& url) {
    m_serverUrl = url;
}

void BillSyncClient::setCurrentUser(const User& user) {
    m_currentUser = user;
}

void BillSyncClient::syncRecord(const AccountRecord& record) {
    if(m_serverUrl.isEmpty() || m_currentUser.getId() <= 0)
        return;

    m_isSyncing = true;
    emit syncStatusChanged(true);

    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrl + "/api/sync/record"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // 添加用户认证（示例）
    request.setRawHeader("Authorization",
                         QString("Bearer %1").arg(m_currentUser.getAccount()).toUtf8());

    QByteArray data = buildSyncJson(record);
    m_networkManager->post(request, data);
}

void BillSyncClient::pullLatestRecords(int userId, const QString& lastSyncTime) {
    if(m_serverUrl.isEmpty())
        return;

    m_isSyncing = true;
    emit syncStatusChanged(true);

    QString url = QString("%1/api/records?user_id=%2&since=%3")
                      .arg(m_serverUrl)
                      .arg(userId)
                      .arg(lastSyncTime);

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setRawHeader("Authorization",
                         QString("Bearer %1").arg(m_currentUser.getAccount()).toUtf8());

    m_networkManager->get(request);
}

void BillSyncClient::startAutoSync(int intervalSeconds) {
    m_syncTimer->start(intervalSeconds * 1000);
}

void BillSyncClient::stopAutoSync() {
    m_syncTimer->stop();
}

// 槽函数实现
void BillSyncClient::onSyncTimerTimeout() {
    // 自动同步逻辑：拉取近1天的最新记录
    if(m_currentUser.getId() > 0)
    {
        QString lastSyncTime = QDateTime::currentDateTime().addDays(-1).toString("yyyy-MM-dd HH:mm:ss");
        pullLatestRecords(m_currentUser.getId(), lastSyncTime);
    }
}

void BillSyncClient::onNetworkReplyFinished(QNetworkReply* reply) {
    m_isSyncing = false;
    emit syncStatusChanged(false);

    if(reply->error() != QNetworkReply::NoError)
    {
        emit syncFailed(AccountRecord(), reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QString url = reply->url().toString();

    if(url.contains("/api/records")) {
        // 解析拉取的记录并发送信号
        QList<AccountRecord> records = parseRecordsFromJson(data);
        emit recordsPulled(records);
    } else if(url.contains("/api/sync")) {
        // 解析同步结果
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if(doc.isObject() && doc.object()["success"].toBool()) {
            // 同步成功（此处可根据服务器返回获取recordId）
            emit syncSuccess(AccountRecord());
        } else {
            emit syncFailed(AccountRecord(), "同步失败：" + doc.object()["message"].toString());
        }
    }

    reply->deleteLater();
}

// 内部辅助方法实现
QByteArray BillSyncClient::buildSyncJson(const AccountRecord& record) {
    QJsonObject obj;
    obj["id"] = record.getId();
    obj["user_id"] = record.getUserId();
    obj["amount"] = record.getAmount();
    obj["type"] = record.getType();
    obj["remark"] = record.getRemark();
    obj["voucher_path"] = record.getVoucherPath();
    obj["is_deleted"] = record.getIsDeleted();
    obj["create_time"] = record.getCreateTime();
    obj["modify_time"] = record.getModifyTime();

    return QJsonDocument(obj).toJson();
}

QList<AccountRecord> BillSyncClient::parseRecordsFromJson(const QByteArray& json) {
    QList<AccountRecord> records;
    QJsonDocument doc = QJsonDocument::fromJson(json);

    if(!doc.isObject() || !doc.object()["success"].toBool())
        return records;

    QJsonArray array = doc.object()["records"].toArray();
    for(const auto& item : array)
    {
        if(item.isObject())
        {
            QJsonObject obj = item.toObject();
            AccountRecord record;
            record.setId(obj["id"].toInt());
            record.setUserId(obj["user_id"].toInt());
            record.setAmount(obj["amount"].toDouble());
            record.setType(obj["type"].toString());
            record.setRemark(obj["remark"].toString());
            record.setVoucherPath(obj["voucher_path"].toString());
            record.setIsDeleted(obj["is_deleted"].toInt());
            record.setCreateTime(obj["create_time"].toString());
            record.setModifyTime(obj["modify_time"].toString());
            records.append(record);
        }
    }
    return records;
}
