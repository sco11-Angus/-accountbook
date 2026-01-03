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
#include <QDateTime>
#include <QEvent>
#include <QButtonGroup>
#include <QDialog>
#include <QCalendarWidget>
#include <QSpinBox>
#include <QTimeEdit>
#include "account_record.h"
#include "account_manager.h"
#include "user_manager.h"

class DateTimePickerDialog;

class AccountBookRecordWidget : public QDialog
{
    Q_OBJECT
public:
    explicit AccountBookRecordWidget(QWidget *parent = nullptr);
    void setRecord(const AccountRecord& record); // 添加设置记录的方法，用于编辑模式

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
    void onTimeClicked();

private:
    bool m_isEditMode = false;      // 是否为编辑模式
    AccountRecord m_currentRecord;  // 当前正在编辑的记录
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
    QWidget* createCateBtn(const QString& text,const QString& imgDir);
    void createKeyboard();
    QLineEdit* getCurrentAmountEdit();

    bool eventFilter(QObject *obj, QEvent *event) override;  // 重写事件过滤器
    void updateTimeDisplay();
    void onTimeLabelClicked();
    void showDateTimePicker();
    QLabel* getCurrentTimeLabel();
    QLineEdit* getCurrentNoteEdit();

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
    QButtonGroup* m_expenseGroup;  // 支出分类按钮组
    QButtonGroup* m_incomeGroup;   // 收入分类按钮组

    QLabel *m_expenseTimeLabel;   // 支出页时间按钮
    QLineEdit *m_expenseNoteEdit;     // 支出页备注框
    QLabel *m_incomeTimeLabel;    // 收入页时间按钮
    QLineEdit *m_incomeNoteEdit;     // 收入页备注框

    QDateTime m_currentDateTime;    // 存储当前选中的时间
};

class DateTimePickerDialog : public QDialog
{
    Q_OBJECT
public:
    DateTimePickerDialog(QWidget *parent = nullptr);
    void setDateTime(const QDateTime &dateTime);
    QDateTime getDateTime() const;

private slots:
    void onPrevMonth();
    void onNextMonth();
    void onYearMonthClicked();
    void onYearMonthSelected();
    void onTimeClicked();
    void onTimeSelected();

private:
    void initUI();
    void updateDisplay();
    QDateTime m_dateTime;
    QPushButton *m_prevMonthBtn;
    QPushButton *m_yearMonthBtn;
    QPushButton *m_nextMonthBtn;
    QCalendarWidget *m_calendar;
    QLabel *m_timeLabel;
    QPushButton *m_timeBtn;
    QDialog *m_yearMonthDialog;
    QSpinBox *m_yearSpinBox;
    QSpinBox *m_monthSpinBox;
    QDialog *m_timeDialog;
    QTimeEdit *m_timeEdit;
};

#endif // ACCOUNTBOOKRECORDWIDGET_H

