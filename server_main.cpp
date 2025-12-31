#include "server_main.h"
#include <QDebug>
#include <QTcpSocket>

server_main::server_main(QObject *parent)
    : QObject(parent)
    , m_tcpServer(nullptr)
    , m_billHandler(nullptr)
{
    m_tcpServer = new tcp_server(this);
    m_billHandler = new bill_handler();
    
    // 连接消息接收信号
    connect(m_tcpServer, &tcp_server::messageReceived,
            this, &server_main::onMessageReceived);
}

server_main::~server_main()
{
    stopServer();
    if (m_billHandler) {
        delete m_billHandler;
        m_billHandler = nullptr;
    }
}

bool server_main::startServer(quint16 port)
{
    if (m_tcpServer->isListening()) {
        qDebug() << "服务器已经在运行中";
        return false;
    }
    
    bool success = m_tcpServer->startServer(port);
    if (success) {
        qDebug() << "服务器主程序启动成功，监听端口:" << port;
    }
    
    // 初始化数据库管理器
    s_dbmanger = DBManager::getInstance();
    // 服务器端也需要初始化 DBManager
    s_dbmanger->initialize();
    
    // 连接远程 MySQL 数据库
    // 注意：这里的 localhost, root, CcSs123456@ 是根据常见设置提供的默认值，如果您的数据库环境不同请修改
    if (!s_dbmanger->connectRemoteDatabase("localhost", 3306, "root", "CcSs123456@", "account_book_server")) {
        qWarning() << "远程数据库连接失败:" << s_dbmanger->getLastError();
        qWarning() << "请检查: 1. MySQL服务是否启动 2. 用户名密码是否正确 3. 数据库驱动是否安装";
    } else {
        qDebug() << "远程数据库连接成功，状态:" << (s_dbmanger->isRemoteConnected() ? "在线" : "离线");
    }
    
    return success;
}

void server_main::stopServer()
{
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->stopServer();
        qDebug() << "服务器主程序已停止";
    }
}

bool server_main::isServerRunning() const
{
    return m_tcpServer && m_tcpServer->isListening();
}

void server_main::onMessageReceived(qintptr socketDescriptor, const QJsonObject& message)
{
    QString type = message["type"].toString();
    qDebug() << "处理消息，Socket描述符:" << socketDescriptor << "消息类型:" << type;
    
    QJsonObject response;
    
    if (type == "sync_bills") {
        // 处理同步账单请求
        response = m_billHandler->handleSyncBills(message);
    }
    else if (type == "fetch_latest") {
        // 处理查询账单请求（获取最新数据）
        response = m_billHandler->handleQueryBills(message);
    }
    else if (type == "query_bills") {
        // 处理查询账单请求（带查询条件）
        response = m_billHandler->handleQueryBills(message);
    }
    else if (type == "backup_data") {
        // 处理备份数据请求
        response = m_billHandler->handleBackupData(message);
    }
    else {
        // 未知消息类型
        response["type"] = "error_response";
        response["success"] = false;
        response["message"] = QString("未知的消息类型: %1").arg(type);
        qDebug() << "未知的消息类型:" << type;
    }
    
    // 发送响应给客户端
    sendResponse(socketDescriptor, response);
}

QTcpSocket* server_main::getClientSocket(qintptr socketDescriptor)
{
    return m_tcpServer->getClientSocket(socketDescriptor);
}

void server_main::sendResponse(qintptr socketDescriptor, const QJsonObject& response)
{
    m_tcpServer->sendMessageToClient(socketDescriptor, response);
}
