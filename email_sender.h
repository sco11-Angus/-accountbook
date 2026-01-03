#ifndef EMAIL_SENDER_H
#define EMAIL_SENDER_H

#include <QObject>
#include <QString>
#include <QSslSocket>
#include <QTimer>

class EmailSender : public QObject {
    Q_OBJECT

public:
    explicit EmailSender(QObject *parent = nullptr);
    ~EmailSender();

    // 配置SMTP服务器信息
    void setSmtpServer(const QString& server, int port, bool useSsl = true);
    // 设置发送邮箱和授权码
    void setCredentials(const QString& email, const QString& password);
    // 发送验证码邮件
    void sendVerificationCode(const QString& recipientEmail, const QString& code);
    // 检查是否正在发送
    bool isSending() const { return m_isSending; }

signals:
    // 发送完成信号
    void sendFinished(bool success, const QString& message);
    // 发送进度信号（可选）
    void sendProgress(const QString& message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSslErrors(const QList<QSslError>& errors);
    void onTimeout();

private:
    void sendCommand(const QString& command);
    void processResponse(const QString& response);
    void sendEmailContent(const QString& recipientEmail, const QString& code);
    void resetState();

    QSslSocket* m_socket;
    QTimer* m_timeoutTimer;

    QString m_smtpServer;
    int m_smtpPort;
    bool m_useSsl;
    QString m_senderEmail;
    QString m_senderPassword;

    QString m_recipientEmail;
    QString m_verificationCode;

    bool m_isSending;
    int m_currentStep;  // 0:未开始, 1:连接, 2:EHLO, 3:AUTH, 4:LOGIN_USER, 5:LOGIN_PASS, 6:MAIL_FROM, 7:RCPT_TO, 8:DATA, 9:QUIT
};

#endif // EMAIL_SENDER_H

