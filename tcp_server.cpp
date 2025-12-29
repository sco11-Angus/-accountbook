#include "tcp_server.h"
#include "thread_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

tcp_server::tcp_server(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_threadManager(nullptr)
    , m_port(8888)
{
    m_server = new QTcpServer(this);
    m_threadManager = new thread_manager();
    
    // 连接信号槽
    connect(m_server, &QTcpServer::newConnection, this, &tcp_server::onNewConnection);
}

tcp_server::~tcp_server()
{
    stopServer();
}

bool tcp_server::startServer(quint16 port)
{
    if (m_server->isListening()) {
        qDebug() << "服务器已经在运行中";
        return false;
    }
    
    m_port = port;
    if (!m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "服务器启动失败:" << m_server->errorString();
        return false;
    }
    
    qDebug() << "服务器启动成功，监听端口:" << port;
    emit serverStarted(port);
    return true;
}

void tcp_server::stopServer()
{
    if (m_server->isListening()) {
        // 断开所有客户端连接
        for (QTcpSocket* client : m_clients) {
            if (client) {
                client->disconnectFromHost();
                if (client->state() != QAbstractSocket::UnconnectedState) {
                    client->waitForDisconnected(1000);
                }
            }
        }
        m_clients.clear();
        
        // 停止监听
        m_server->close();
        qDebug() << "服务器已停止";
        emit serverStopped();
    }
}

bool tcp_server::isListening() const
{
    return m_server->isListening();
}

int tcp_server::getClientCount() const
{
    return m_clients.size();
}

QTcpSocket* tcp_server::getClientSocket(qintptr socketDescriptor)
{
    for (QTcpSocket* socket : m_clients) {
        if (socket && socket->socketDescriptor() == socketDescriptor) {
            return socket;
        }
    }
    return nullptr;
}

void tcp_server::sendMessageToClient(qintptr socketDescriptor, const QJsonObject& message)
{
    QTcpSocket* socket = getClientSocket(socketDescriptor);
    if (socket) {
        sendMessage(socket, message);
    } else {
        qDebug() << "未找到socket描述符对应的客户端:" << socketDescriptor;
    }
}

void tcp_server::onNewConnection()
{
    QTcpSocket* clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) {
        return;
    }
    
    qintptr socketDescriptor = clientSocket->socketDescriptor();
    m_clients.append(clientSocket);
    
    // 连接信号槽
    connect(clientSocket, &QTcpSocket::readyRead, this, &tcp_server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &tcp_server::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    
    qDebug() << "新客户端连接，Socket描述符:" << socketDescriptor;
    emit clientConnected(socketDescriptor);
    
    // 发送欢迎消息
    QJsonObject welcomeMsg;
    welcomeMsg["type"] = "welcome";
    welcomeMsg["message"] = "连接成功";
    welcomeMsg["socketDescriptor"] = static_cast<qint64>(socketDescriptor);
    sendMessage(clientSocket, welcomeMsg);
}

void tcp_server::onClientDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) {
        return;
    }
    
    qintptr socketDescriptor = clientSocket->socketDescriptor();
    m_clients.removeAll(clientSocket);
    
    // 清理该socket的缓冲区
    m_socketBuffers.remove(socketDescriptor);
    
    qDebug() << "客户端断开连接，Socket描述符:" << socketDescriptor;
    emit clientDisconnected(socketDescriptor);
}

void tcp_server::onReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) {
        return;
    }
    
    // 读取所有可用数据
    QByteArray data = clientSocket->readAll();
    qintptr socketDescriptor = clientSocket->socketDescriptor();
    
    // 将新数据追加到缓冲区
    if (m_socketBuffers.contains(socketDescriptor)) {
        m_socketBuffers[socketDescriptor].append(data);
    } else {
        m_socketBuffers[socketDescriptor] = data;
    }
    
    QByteArray& buffer = m_socketBuffers[socketDescriptor];
    
    // 处理完整的消息（以换行符分隔）
    while (buffer.contains('\n')) {
        int lineEnd = buffer.indexOf('\n');
        QByteArray messageData = buffer.left(lineEnd);
        buffer.remove(0, lineEnd + 1);
        
        if (!messageData.isEmpty()) {
            QJsonObject message = parseMessage(messageData);
    if (!message.isEmpty()) {
        qDebug() << "接收到消息，Socket描述符:" << socketDescriptor << "消息类型:" << message["type"].toString();
        emit messageReceived(socketDescriptor, message);
            }
        }
    }
}

QJsonObject tcp_server::parseMessage(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return QJsonObject();
    }
    
    if (!doc.isObject()) {
        qDebug() << "数据不是JSON对象";
        return QJsonObject();
    }
    
    return doc.object();
}

void tcp_server::sendMessage(QTcpSocket* socket, const QJsonObject& message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append("\n"); // 添加换行符作为消息分隔符
    
    qint64 bytesWritten = socket->write(data);
    if (bytesWritten == -1) {
        qDebug() << "发送消息失败:" << socket->errorString();
    } else {
        socket->flush();
        qDebug() << "发送消息成功，字节数:" << bytesWritten;
    }
}
