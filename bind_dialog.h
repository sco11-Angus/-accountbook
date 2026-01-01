#ifndef BIND_DIALOG_H
#define BIND_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

class BindDialog : public QDialog {
    Q_OBJECT
public:
    explicit BindDialog(const QString& title, const QString& currentVal, QWidget *parent = nullptr);
    QString getNewValue() const { return m_inputEdit->text(); }

private slots:
    void onSendCode();
    void onConfirm();

private:
    QLineEdit *m_inputEdit;
    QLineEdit *m_codeEdit;
    QPushButton *m_sendBtn;
    QLabel *m_currentLabel;
    QString m_title;
};

#endif // BIND_DIALOG_H
