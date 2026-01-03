#include "email_sender.h"
#include <QDebug>
#include <QByteArray>
#include <QSslConfiguration>

EmailSender::EmailSender(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_smtpServer("smtp.qq.com")
    , m_smtpPort(465)
    , m_useSsl(true)
    , m_isSending(false)
    , m_currentStep(0)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(60000); // 60秒超时（邮件发送可能需要更长时间）
    connect(m_timeoutTimer, &QTimer::timeout, this, &EmailSender::onTimeout);
}

EmailSender::~EmailSender() {
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
}

void EmailSender::setSmtpServer(const QString& server, int port, bool useSsl) {
    m_smtpServer = server;
    m_smtpPort = port;
    m_useSsl = useSsl;
}

void EmailSender::setCredentials(const QString& email, const QString& password) {
    m_senderEmail = email;
    m_senderPassword = password;
}

void EmailSender::sendVerificationCode(const QString& recipientEmail, const QString& code) {
    if (m_isSending) {
        emit sendFinished(false, "正在发送邮件，请稍候...");
        return;
    }

    if (m_senderEmail.isEmpty() || m_senderPassword.isEmpty()) {
        emit sendFinished(false, "请先配置发送邮箱和授权码");
        return;
    }

    if (recipientEmail.isEmpty() || code.isEmpty()) {
        emit sendFinished(false, "收件人邮箱或验证码不能为空");
        return;
    }

    m_recipientEmail = recipientEmail;
    m_verificationCode = code;
    m_isSending = true;
    m_currentStep = 0;

    // 创建SSL Socket
    if (m_socket) {
        m_socket->deleteLater();
    }
    m_socket = new QSslSocket(this);

    // 配置SSL
    QSslConfiguration sslConfig = m_socket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // 跳过证书验证（生产环境建议验证）
    m_socket->setSslConfiguration(sslConfig);

    // 连接信号槽
    connect(m_socket, &QSslSocket::connected, this, &EmailSender::onSocketConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &EmailSender::onSocketDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &EmailSender::onSocketReadyRead);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &EmailSender::onSslErrors);

    emit sendProgress("正在连接SMTP服务器...");

    // 连接到SMTP服务器
    m_socket->connectToHostEncrypted(m_smtpServer, m_smtpPort);
    m_timeoutTimer->start();
}

void EmailSender::onSocketConnected() {
    m_currentStep = 1;
    emit sendProgress("已连接到SMTP服务器");
    // SSL连接后会自动进入加密模式，等待服务器响应
}

void EmailSender::onSocketDisconnected() {
    if (m_isSending && m_currentStep < 9) {
        emit sendFinished(false, "连接意外断开");
        resetState();
    }
}

void EmailSender::onSocketReadyRead() {
    QByteArray data = m_socket->readAll();
    QString response = QString::fromUtf8(data);

    qDebug() << "SMTP Response:" << response;
    emit sendProgress(QString("服务器响应: %1").arg(response.trimmed()));

    processResponse(response);
}

void EmailSender::onSslErrors(const QList<QSslError>& errors) {
    // 忽略SSL错误（开发环境，生产环境应验证证书）
    qDebug() << "SSL Errors:" << errors;
    m_socket->ignoreSslErrors();
}

void EmailSender::onTimeout() {
    if (m_isSending) {
        emit sendFinished(false, "发送超时，请检查网络连接");
        if (m_socket) {
            m_socket->disconnectFromHost();
        }
        resetState();
    }
}

void EmailSender::processResponse(const QString& response) {
    // SMTP响应可能有多行，需要检查最后一行（以响应码开头）
    QStringList lines = response.split("\r\n", Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        lines = response.split("\n", Qt::SkipEmptyParts);
    }

    // 找到最后一行有效的响应码
    QString lastLine;
    for (int i = lines.size() - 1; i >= 0; --i) {
        QString line = lines[i].trimmed();
        if (line.length() >= 3 && line[0].isDigit()) {
            lastLine = line;
            break;
        }
    }

    if (lastLine.isEmpty()) {
        lastLine = response.trimmed();
    }

    // 提取响应码（前3位数字）
    QString code = lastLine.length() >= 3 ? lastLine.left(3) : "";

    // 检查响应码
    if (code == "220") {
        // 服务器就绪，发送EHLO
        m_currentStep = 2;
        sendCommand(QString("EHLO %1").arg(m_smtpServer));
    }
    else if (code == "250" && m_currentStep == 2) {
        // EHLO成功，发送AUTH LOGIN
        m_currentStep = 3;
        sendCommand("AUTH LOGIN");
    }
    else if (code == "334" && m_currentStep == 3) {
        // 需要用户名（base64编码）
        m_currentStep = 4;
        QByteArray emailBytes = m_senderEmail.toUtf8();
        sendCommand(emailBytes.toBase64());
    }
    else if (code == "334" && m_currentStep == 4) {
        // 需要密码（base64编码）
        m_currentStep = 5;
        QByteArray pwdBytes = m_senderPassword.toUtf8();
        sendCommand(pwdBytes.toBase64());
    }
    else if (code == "235" && m_currentStep == 5) {
        // 认证成功，发送MAIL FROM
        m_currentStep = 6;
        emit sendProgress("认证成功");
        sendCommand(QString("MAIL FROM:<%1>").arg(m_senderEmail));
    }
    else if (code == "250" && m_currentStep == 6) {
        // MAIL FROM成功，发送RCPT TO
        m_currentStep = 7;
        sendCommand(QString("RCPT TO:<%1>").arg(m_recipientEmail));
    }
    else if (code == "250" && m_currentStep == 7) {
        // RCPT TO成功，发送DATA
        m_currentStep = 8;
        sendCommand("DATA");
    }
    else if (code == "354" && m_currentStep == 8) {
        // DATA命令成功，发送邮件内容
        sendEmailContent(m_recipientEmail, m_verificationCode);
    }
    else if (code == "250" && m_currentStep == 8) {
        // 邮件发送成功，发送QUIT
        m_currentStep = 9;
        emit sendProgress("邮件发送成功");
        sendCommand("QUIT");
    }
    else if (code == "221" && m_currentStep == 9) {
        // QUIT成功，完成
        emit sendFinished(true, "验证码邮件已成功发送");
        resetState();
    }
    else if (code.startsWith("5") || (code.startsWith("4") && m_currentStep > 1)) {
        // 错误响应（4xx在认证前可能是正常的）
        QString errorMsg = QString("SMTP错误: %1").arg(lastLine);
        emit sendFinished(false, errorMsg);
        resetState();
    }
    // 其他情况可能是多行响应的中间行，忽略
}

void EmailSender::sendCommand(const QString& command) {
    if (!m_socket || !m_socket->isOpen()) {
        emit sendFinished(false, "Socket未连接");
        resetState();
        return;
    }

    QString cmd = command + "\r\n";
    m_socket->write(cmd.toUtf8());
    qDebug() << "SMTP Command:" << command;
}

void EmailSender::sendEmailContent(const QString& recipientEmail, const QString& code) {
    QString subject = "红果记账 - 验证码";
    QString body = QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: "Microsoft YaHei", Arial, sans-serif; line-height: 1.6; color: #333; }
        .container { max-width: 600px; margin: 0 auto; padding: 20px; background-color: #f9f9f9; }
        .header { background: linear-gradient(135deg, #A8E6CF 0%, #DCEDC1 100%); padding: 30px; text-align: center; border-radius: 10px 10px 0 0; }
        .content { background-color: white; padding: 30px; border-radius: 0 0 10px 10px; }
        .code-box { background-color: #F0FFF4; border: 2px dashed #A8E6CF; padding: 20px; text-align: center; margin: 20px 0; border-radius: 8px; }
        .code { font-size: 32px; font-weight: bold; color: #4A4A4A; letter-spacing: 8px; }
        .footer { text-align: center; margin-top: 20px; color: #888; font-size: 12px; }
        .warning { color: #FF6B6B; font-size: 14px; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1 style="color: white; margin: 0;">红果记账</h1>
        </div>
        <div class="content">
            <h2>您好！</h2>
            <p>您正在使用邮箱 <strong>%1</strong> 进行验证操作。</p>
            <p>您的验证码是：</p>
            <div class="code-box">
                <div class="code">%2</div>
            </div>
            <p>验证码有效期为 <strong>10分钟</strong>，请勿泄露给他人。</p>
            <p class="warning">⚠️ 如果这不是您的操作，请忽略此邮件。</p>
            <div class="footer">
                <p>此邮件由系统自动发送，请勿回复。</p>
                <p>© 2024 红果记账</p>
            </div>
        </div>
    </div>
</body>
</html>
    )").arg(recipientEmail).arg(code);

    // 构建邮件头
    QString subjectEncoded = QByteArray(subject.toUtf8()).toBase64();
    QString bodyEncoded = QByteArray(body.toUtf8()).toBase64();

    // 构建完整的邮件内容（每行以\r\n结尾）
    QString emailContent;
    emailContent += QString("From: 红果记账 <%1>\r\n").arg(m_senderEmail);
    emailContent += QString("To: <%1>\r\n").arg(recipientEmail);
    emailContent += QString("Subject: =?UTF-8?B?%1?=\r\n").arg(subjectEncoded);
    emailContent += "MIME-Version: 1.0\r\n";
    emailContent += "Content-Type: text/html; charset=UTF-8\r\n";
    emailContent += "Content-Transfer-Encoding: base64\r\n";
    emailContent += "\r\n";  // 空行分隔头部和正文
    emailContent += bodyEncoded;
    emailContent += "\r\n";  // 正文结束换行

    // 发送邮件内容
    m_socket->write(emailContent.toUtf8());

    // 发送结束标记（单独的"."加\r\n）
    m_socket->write(".\r\n");

    qDebug() << "邮件内容已发送，等待服务器响应...";
}

void EmailSender::resetState() {
    m_isSending = false;
    m_currentStep = 0;
    m_timeoutTimer->stop();
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

