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

class AccountBookMainWidget : public QWidget
{
    Q_OBJECT
public:
    QPushButton *m_addBtn;
    explicit AccountBookMainWidget(QWidget *parent = nullptr);
    void updateBillData(const QList<QMap<QString, QString>>& billList);



private:
    void initUI();
    void initStyleSheet();

    // 工具函数 - 创建单个账单项（核心动态生成逻辑）
    QWidget* createBillItemWidget(const QString& date, const QString& cateIcon,
                                  const QString& cateName, const QString& time,
                                  const QString& amount, bool isExpense);
    // 更新收支统计（总支出/总收入/结余）
    void updateStatistic(double totalExpense, double totalIncome);

    // 顶部控件
    QComboBox *m_bookSwitchCombo; // 账本切换
    QLineEdit *m_searchEdit;      // 搜索框
    QPushButton *m_prevMonthBtn;  // 上月按钮
    QLabel *m_monthLabel;         // 月份显示
    QPushButton *m_nextMonthBtn;  // 下月按钮
    QPushButton *m_calendarBtn;   // 收支日历

    // 收支统计卡片
    QFrame *m_statCard;
    QLabel *m_totalExpenseLabel;  // 总支出
    QLabel *m_totalIncomeLabel;   // 总收入
    QLabel *m_monthSurplusLabel;  // 月结余

    // 账单列表
    QListWidget *m_billListWidget;

    // 底部导航
    QPushButton *m_bookNavBtn;    // 账本（默认选中）
    QPushButton *m_assetNavBtn;   // 资产
    QPushButton *m_statNavBtn;    // 统计
};

#endif // ACCOUNTBOOKMAINWIDGET_H
