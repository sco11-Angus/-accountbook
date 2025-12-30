#ifndef MONTHPICKERDIALOG_H
#define MONTHPICKERDIALOG_H

#include <QDialog>
#include <QDate>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>

class MonthPickerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MonthPickerDialog(const QDate &currentDate, QWidget *parent = nullptr);
    QDate getSelectedDate() const { return m_selectedDate; }

private slots:
    void onMonthSelected();
    void onCurrentMonthClicked();

private:
    void initUI();
    void addYearSection(int year, QVBoxLayout *layout);
    
    QDate m_selectedDate;
    QDate m_initialDate;
};

#endif // MONTHPICKERDIALOG_H
