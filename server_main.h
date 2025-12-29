#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include <QObject>
#include <QJsonObject>
#include "tcp_server.h"
#include "bill_handler.h"

class server_main : public QObject
{
    Q_OBJECT

public:
    explicit server_main(QObject *parent = nullptr);
    ~server_main();

    // 启动服务器
    bool startServer(quint16 port = 8888);
    // 停止服务器
    void stopServer();
    // 获取服务器状态
    bool isServerRunning() const;

private slots:
    // 处理接收到的消息
    void onMessageReceived(qintptr socketDescriptor, const QJsonObject& message);

private:
    tcp_server* m_tcpServer;
    bill_handler* m_billHandler;
    
    // 获取客户端socket
    QTcpSocket* getClientSocket(qintptr socketDescriptor);
    // 发送响应给客户端
    void sendResponse(qintptr socketDescriptor, const QJsonObject& response);
};

#endif // SERVER_MAIN_H
