#ifndef ACCOUNTBOOKRECORDWIDGET_H
#define ACCOUNTBOOKRECORDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QString>

class AccountBookRecordWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountBookRecordWidget(QWidget *parent = nullptr);

signals:
    // 记账完成信号，可携带账单数据（也可只发信号，主界面自己查数据库）
    void billRecorded();

private slots:
    void onNumberClicked(const QString& number);
    void onDotClicked();
    void onDeleteClicked();
    // 新增槽函数声明
    void onPlusGroupClicked();
    void onMinusGroupClicked();
    void updateAmountDisplay();
    void calculateIfPossible();
    void onEqualClicked();

private:
    // 新增枚举类型定义
    enum class PlusMode { Add, Mul };       // 加/乘模式
    enum class MinusMode { Sub, Div };      // 减/除模式
    enum class Op { None, Add, Sub, Mul, Div }; // 运算符
    enum class InputPhase {                 // 输入阶段
        EnteringFirst,      // 输入第一个操作数
        OperatorChosen,     // 已选择运算符
        EnteringSecond,     // 输入第二个操作数
        ResultShown         // 已显示计算结果
    };

    void initUI();
    void initStyleSheet();
    QPushButton* createCateBtn(const QString& text, const QString& color);
    void createKeyboard();
    QLineEdit* getCurrentAmountEdit();

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

    // 新增成员变量声明
    QPushButton *m_deleteBtn;    // 退格键
    QPushButton *m_plusBtn;      // 加/乘按钮
    QPushButton *m_minusBtn;     // 减/除按钮
    QPushButton *m_equalBtn;     // 等号按钮

    PlusMode m_plusMode;         // 当前加/乘模式
    MinusMode m_minusMode;       // 当前减/除模式
    Op m_currentOp;              // 当前运算符
    InputPhase m_phase;          // 当前输入阶段
    QString m_firstOperandText;  // 第一个操作数文本
    QString m_secondOperandText; // 第二个操作数文本
    QString opToString(Op op) const; // 运算符转字符串

    QString m_selectedCategory; // 选中的分类（如“餐饮”）
    bool m_isExpense;           // 是否为支出（true=支出，false=收入）
    int m_currentUserId;        // 当前登录用户ID
};

#endif // ACCOUNTBOOKRECORDWIDGET_H
