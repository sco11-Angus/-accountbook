#ifndef EMAIL_CONFIG_DIALOG_H
#define EMAIL_CONFIG_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

class EmailConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit EmailConfigDialog(QWidget *parent = nullptr);
    QString getSenderEmail() const { return m_emailEdit->text().trimmed(); }
    QString getAuthCode() const { return m_authCodeEdit->text().trimmed(); }

private slots:
    void onConfirmClicked();
    void onTestClicked();

private:
    void initUI();

    QLineEdit *m_emailEdit;
    QLineEdit *m_authCodeEdit;
    QLabel *m_tipLabel;
};

#endif // EMAIL_CONFIG_DIALOG_H

