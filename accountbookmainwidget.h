#ifndef ACCOUNTBOOKMAINWIDGET_H
#define ACCOUNTBOOKMAINWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDate>
#include <QScrollArea>
#include <QStackedWidget>
#include "settings_widget.h"
#include "statistics_widget.h"

class AccountBookMainWidget : public QWidget
{
    Q_OBJECT
public:
    QPushButton *m_addBtn;
    explicit AccountBookMainWidget(QWidget *parent = nullptr);
    void updateBillData(const QList<AccountRecord>& records);

private slots:
    void onPrevMonth();
    void onNextMonth();
    void onMonthLabelClicked();
    void onSearchTextChanged(const QString &text);
    void onNavButtonClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUI();
    void initStyleSheet();
    void updateDateDisplay();
    void loadBillsForMonth();

    // 工具函数 - 创建单个账单项（核心动态生成逻辑）
    QWidget* createBillItemWidget(const QString& date, const QString& cateIcon,
                                  const QString& cateName, const QString& time,
                                  const QString& amount, bool isExpense);
    // 更新收支统计（总支出/总收入/结余）
    void updateStatistic(double totalExpense, double totalIncome);

    QDate m_currentDate; // 当前显示的月份

    // 界面堆栈
    QStackedWidget *m_stackedWidget;
    QWidget *m_bookPage;
    StatisticsWidget *m_statisticsPage;
    SettingsWidget *m_settingsPage;

    // 顶部控件
    QLineEdit *m_searchEdit;      // 搜索框
    QPushButton *m_prevMonthBtn;  // 上月按钮
    QLabel *m_monthLabel;         // 月份显示
    QPushButton *m_nextMonthBtn;  // 下月按钮

    // 收支统计卡片
    QFrame *m_statCard;
    QLabel *m_totalExpenseLabel;  // 总支出
    QLabel *m_totalIncomeLabel;   // 总收入
    QLabel *m_monthSurplusLabel;  // 月结余

    // 账单列表
    QListWidget *m_billListWidget;

    // 底部导航
    QPushButton *m_bookNavBtn;    // 账本（默认选中）
    QPushButton *m_statNavBtn;    // 统计
    QPushButton *m_userNavBtn;    // 我的
};

#endif // ACCOUNTBOOKMAINWIDGET_H
