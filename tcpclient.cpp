#include "tcpclient.h"
#include <QDebug>
#include <QJsonParseError>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
{
    m_socket = new QTcpSocket(this);
    
    // 连接信号槽
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    // Qt 6 使用 errorOccurred 信号，Qt 5 使用 error 信号
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &TcpClient::onError);
    #else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &TcpClient::onError);
    #endif
}

TcpClient::~TcpClient()
{
    disconnectFromServer();
}

bool TcpClient::connectToServer(const QString& host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "已经连接到服务端";
        return true;
    }
    
    m_socket->connectToHost(host, port);
    
    // 等待连接，最多等待3秒
    if (!m_socket->waitForConnected(3000)) {
        qDebug() << "连接失败:" << m_socket->errorString();
        emit errorOccurred("连接失败: " + m_socket->errorString());
        return false;
    }
    
    qDebug() << "成功连接到服务端" << host << ":" << port;
    return true;
}

void TcpClient::disconnectFromServer()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
    m_buffer.clear();
}

bool TcpClient::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpClient::syncBills(const QList<AccountRecord>& bills)
{
    if (!isConnected()) {
        qDebug() << "未连接到服务端，无法同步账单";
        emit errorOccurred("未连接到服务端");
        return false;
    }
    
    // 构建JSON消息
    QJsonObject message;
    message["type"] = "sync_bills";
    message["action"] = "upload";
    
    // 将账单列表转换为JSON数组
    QJsonArray billsArray;
    for (const AccountRecord& bill : bills) {
        QJsonObject billObj;
        billObj["id"] = bill.getId();
        billObj["userId"] = bill.getUserId();
        billObj["amount"] = bill.getAmount();
        billObj["type"] = bill.getType();
        billObj["remark"] = bill.getRemark();
        billObj["voucherPath"] = bill.getVoucherPath();
        billObj["isDeleted"] = bill.getIsDeleted();
        billObj["deleteTime"] = bill.getDeleteTime();
        billObj["createTime"] = bill.getCreateTime();
        billObj["modifyTime"] = bill.getModifyTime();
        billsArray.append(billObj);
    }
    
    message["bills"] = billsArray;
    message["count"] = bills.size();
    
    qDebug() << "发送同步账单请求，账单数量:" << bills.size();
    return sendJsonMessage(message);
}

bool TcpClient::fetchLatestData(int userId, const QString& lastSyncTime)
{
    if (!isConnected()) {
        qDebug() << "未连接到服务端，无法获取最新数据";
        emit errorOccurred("未连接到服务端");
        return false;
    }
    
    // 构建JSON消息
    QJsonObject message;
    message["type"] = "fetch_latest";
    message["userId"] = userId;
    if (!lastSyncTime.isEmpty()) {
        message["lastSyncTime"] = lastSyncTime;
    }
    
    qDebug() << "发送获取最新数据请求，用户ID:" << userId;
    return sendJsonMessage(message);
}

bool TcpClient::backupData(int userId, const QString& backupPath)
{
    if (!isConnected()) {
        qDebug() << "未连接到服务端，无法备份数据";
        emit errorOccurred("未连接到服务端");
        return false;
}

    // 构建JSON消息
    QJsonObject message;
    message["type"] = "backup_data";
    message["userId"] = userId;
    if (!backupPath.isEmpty()) {
        message["backupPath"] = backupPath;
    }
    
    qDebug() << "发送备份数据请求，用户ID:" << userId;
    return sendJsonMessage(message);
}

void TcpClient::onConnected()
{
    qDebug() << "已连接到服务端";
    emit connected();
}

void TcpClient::onDisconnected()
{
    qDebug() << "已断开与服务端的连接";
    m_buffer.clear();
    emit disconnected();
}

void TcpClient::onReadyRead()
{
    // 读取所有可用数据
    QByteArray data = m_socket->readAll();
    m_buffer.append(data);
    
    // 处理完整的消息（以换行符分隔）
    while (m_buffer.contains('\n')) {
        int lineEnd = m_buffer.indexOf('\n');
        QByteArray messageData = m_buffer.left(lineEnd);
        m_buffer.remove(0, lineEnd + 1);
        
        if (!messageData.isEmpty()) {
            parseMessage(messageData);
        }
    }
}

void TcpClient::onError(QAbstractSocket::SocketError socketError)
{
    QString errorString = "Socket错误: ";
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString += "连接被拒绝";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString += "远程主机关闭连接";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString += "找不到主机";
        break;
    case QAbstractSocket::NetworkError:
        errorString += "网络错误";
        break;
    default:
        errorString += m_socket->errorString();
        break;
    }
    
    qDebug() << errorString;
    emit errorOccurred(errorString);
}

void TcpClient::parseMessage(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        emit errorOccurred("JSON解析错误: " + error.errorString());
        return;
    }
    
    if (!doc.isObject()) {
        qDebug() << "数据不是JSON对象";
        return;
    }
    
    QJsonObject message = doc.object();
    QString type = message["type"].toString();
    
    qDebug() << "接收到消息，类型:" << type;
    
    if (type == "sync_bills_response") {
        // 处理同步账单响应
        bool success = message["success"].toBool();
        QString msg = message["message"].toString();
        qDebug() << "同步账单响应:" << (success ? "成功" : "失败") << msg;
        emit syncBillsResponse(success, msg);
    }
    else if (type == "fetch_latest_response") {
        // 处理获取最新数据响应
        bool success = message["success"].toBool();
        if (success && message.contains("bills")) {
            QJsonArray billsArray = message["bills"].toArray();
            qDebug() << "获取最新数据成功，账单数量:" << billsArray.size();
            emit latestDataReceived(billsArray);
        } else {
            QString errorMsg = message["message"].toString();
            qDebug() << "获取最新数据失败:" << errorMsg;
            emit errorOccurred("获取最新数据失败: " + errorMsg);
        }
    }
    else if (type == "backup_data_response") {
        // 处理备份数据响应
        bool success = message["success"].toBool();
        QString msg = message["message"].toString();
        QString backupPath = message.contains("backupPath") ? message["backupPath"].toString() : "";
        qDebug() << "备份数据响应:" << (success ? "成功" : "失败") << msg;
        emit backupDataResponse(success, msg, backupPath);
    }
    else if (type == "welcome") {
        // 处理欢迎消息
        QString msg = message["message"].toString();
        qDebug() << "服务端欢迎消息:" << msg;
    }
    else {
        qDebug() << "未知的消息类型:" << type;
    }
}

bool TcpClient::sendJsonMessage(const QJsonObject& message)
{
    if (!isConnected()) {
        qDebug() << "未连接到服务端，无法发送消息";
        return false;
    }
    
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append("\n");  // 添加换行符作为消息分隔符
    
    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        qDebug() << "发送消息失败:" << m_socket->errorString();
        emit errorOccurred("发送消息失败: " + m_socket->errorString());
        return false;
    }
    
    m_socket->flush();
    qDebug() << "发送消息成功，字节数:" << bytesWritten;
    return true;
}
