#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QList>
#include <QMap>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>

class ThreadManager;

class tcp_server : public QObject
{
    Q_OBJECT

public:
    explicit tcp_server(QObject *parent = nullptr);
    ~tcp_server();

    // 启动服务器
    bool startServer(quint16 port = 8888);
    // 停止服务器
    void stopServer();
    // 获取服务器状态
    bool isListening() const;
    // 获取当前连接的客户端数量
    int getClientCount() const;
    // 根据socket描述符获取socket
    QTcpSocket* getClientSocket(qintptr socketDescriptor);
    // 根据socket描述符发送消息
    void sendMessageToClient(qintptr socketDescriptor, const QJsonObject& message);

signals:
    // 服务器启动信号
    void serverStarted(quint16 port);
    // 服务器停止信号
    void serverStopped();
    // 新客户端连接信号
    void clientConnected(qintptr socketDescriptor);
    // 客户端断开信号
    void clientDisconnected(qintptr socketDescriptor);
    // 接收到消息信号
    void messageReceived(qintptr socketDescriptor, const QJsonObject& message);

private slots:
    // 处理新连接
    void onNewConnection();
    // 处理客户端断开
    void onClientDisconnected();
    // 处理接收到的数据
    void onReadyRead();

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
    ThreadManager* m_threadManager;
    quint16 m_port;
    QMap<qintptr, QByteArray> m_socketBuffers;  // 存储每个socket的不完整数据
    
    // 解析接收到的数据
    QJsonObject parseMessage(const QByteArray& data);
    // 发送消息给客户端
    void sendMessage(QTcpSocket* socket, const QJsonObject& message);
};

#endif // TCP_SERVER_H
