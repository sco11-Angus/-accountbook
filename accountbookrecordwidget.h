#ifndef ACCOUNTBOOKRECORDWIDGET_H
#define ACCOUNTBOOKRECORDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>

class AccountBookRecordWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountBookRecordWidget(QWidget *parent = nullptr);

signals:
    // 记账完成信号，可携带账单数据（也可只发信号，主界面自己查数据库）
    void billRecorded();

private:
    void initUI();
    void initStyleSheet();
    QPushButton* createCateBtn(const QString& text, const QString& color);
    void createKeyboard();

    // 标签页
    QTabWidget *m_tabWidget;

    // 支出页
    QWidget *m_expensePage;
    QGridLayout *m_expenseCateLayout;
    QLineEdit *m_expenseAmountEdit;

    // 收入页
    QWidget *m_incomePage;
    QGridLayout *m_incomeCateLayout;
    QLineEdit *m_incomeAmountEdit;

    // 转账页
    QWidget *m_transferPage;
    QComboBox *m_outAccountCombo;
    QComboBox *m_inAccountCombo;
    QLineEdit *m_transferAmountEdit;

    // 数字键盘+完成按钮
    QWidget *m_keyboardWidget;
    QPushButton *m_completeBtn;
};

#endif // ACCOUNTBOOKRECORDWIDGET_H
