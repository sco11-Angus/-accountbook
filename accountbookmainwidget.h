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
    explicit AccountBookMainWidget(QWidget *parent = nullptr);

private:
    void initUI();
    void initStyleSheet();

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

    // 右下角加号按钮
    QPushButton *m_addBtn;
};

#endif // ACCOUNTBOOKMAINWIDGET_H
