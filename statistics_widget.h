#ifndef STATISTICS_WIDGET_H
#define STATISTICS_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QProgressBar>
#include "statistics_manager.h"

class StatisticsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StatisticsWidget(QWidget *parent = nullptr);
    void updateData(int userId, int year, int month);

private:
    void initUI();
    void initStyleSheet();
    QWidget* createCategoryItem(const CategoryStat& stat);

    QLabel *m_titleLabel;
    QLabel *m_totalExpenseLabel;
    QLabel *m_totalIncomeLabel;
    QLabel *m_balanceLabel;
    
    QVBoxLayout *m_expenseListLayout;
    QVBoxLayout *m_incomeListLayout;
    
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    
    QPushButton *m_expenseTabBtn;
    QPushButton *m_incomeTabBtn;
    
    int m_currentUserId;
    int m_currentYear;
    int m_currentMonth;
    bool m_isShowingExpense = true;

    void refreshList(const MonthlyStat& stat);

private slots:
    void onExpenseTabClicked();
    void onIncomeTabClicked();
};

#endif // STATISTICS_WIDGET_H
