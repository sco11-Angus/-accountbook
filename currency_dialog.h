#ifndef CURRENCY_DIALOG_H
#define CURRENCY_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>

class CurrencyDialog : public QDialog {
    Q_OBJECT
public:
    explicit CurrencyDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("选择货币");
        setFixedSize(250, 350);
        QVBoxLayout *layout = new QVBoxLayout(this);
        
        m_list = new QListWidget();
        m_list->addItem("人民币 (CNY)");
        m_list->addItem("美元 (USD)");
        m_list->addItem("日元 (JPY)");
        m_list->addItem("欧元 (EUR)");
        m_list->addItem("英镑 (GBP)");
        m_list->addItem("韩元 (KRW)");
        
        layout->addWidget(m_list);
        
        QPushButton *btn = new QPushButton("确定");
        connect(btn, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btn);
    }
    
    QString getSelectedCurrency() const {
        if (m_list->currentItem()) return m_list->currentItem()->text();
        return "";
    }

private:
    QListWidget *m_list;
};

#endif // CURRENCY_DIALOG_H
