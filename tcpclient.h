#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QTcpSocket>
#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QString>
#include "account_record.h"

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    // 连接到服务端
    bool connectToServer(const QString& host = "localhost", quint16 port = 8888);
    // 断开连接
    void disconnectFromServer();
    // 检查是否已连接
    bool isConnected() const;

    // 同步账单到服务端
    bool syncBills(const QList<AccountRecord>& bills);
    // 获取最新数据
    bool fetchLatestData(int userId, const QString& lastSyncTime = "");
    // 备份数据
    bool backupData(int userId, const QString& backupPath = "");

signals:
    // 连接成功信号
    void connected();
    // 断开连接信号
    void disconnected();
    // 接收到同步账单响应信号
    void syncBillsResponse(bool success, const QString& message);
    // 接收到最新数据信号
    void latestDataReceived(const QJsonArray& bills);
    // 接收到备份数据响应信号
    void backupDataResponse(bool success, const QString& message, const QString& backupPath = "");
    // 错误信号
    void errorOccurred(const QString& error);

private slots:
    // 处理连接成功
    void onConnected();
    // 处理断开连接
    void onDisconnected();
    // 处理接收到的数据
    void onReadyRead();
    // 处理错误
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;  // 用于存储不完整的数据
    
    // 解析接收到的消息
    void parseMessage(const QByteArray& data);
    // 发送JSON消息
    bool sendJsonMessage(const QJsonObject& message);
};

#endif // TCPCLIENT_H
