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

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    void setDailyStats(const QList<DailyStat>& stats, double maxAmount) {
        m_dailyStats = stats;
        m_maxDailyAmount = maxAmount;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<DailyStat> m_dailyStats;
    double m_maxDailyAmount = 0;
};

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
    QPushButton *m_prevMonthBtn;
    QPushButton *m_nextMonthBtn;
    QLabel *m_monthLabel;
    QLabel *m_totalExpenseLabel;
    QLabel *m_totalIncomeLabel;
    QLabel *m_balanceLabel;
    
    ChartWidget *m_chartWidget;
    QVBoxLayout *m_expenseListLayout;
    QVBoxLayout *m_incomeListLayout;
    
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    
    QPushButton *m_expenseTabBtn;
    QPushButton *m_incomeTabBtn;
    
    // Chart related
    QList<DailyStat> m_dailyStats;
    double m_maxDailyAmount = 0;
    
    int m_currentUserId;
    int m_currentYear;
    int m_currentMonth;
    bool m_isShowingExpense = true;

    void refreshList(const MonthlyStat& stat);
    void updateChart(const QList<DailyStat>& dailyStats);

private slots:
    void onExpenseTabClicked();
    void onIncomeTabClicked();
    void onPrevMonth();
    void onNextMonth();

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // STATISTICS_WIDGET_H
