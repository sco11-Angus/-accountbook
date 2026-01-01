#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include "User.h"

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);

private:
    void initUI();
    void initStyleSheet();
    
    // 创建设置项按钮
    QPushButton* createSettingItem(const QString& icon, const QString& title, const QString& value = "");
    // 创建功能板块
    QFrame* createSectionFrame();

    void updateProfileDisplay();

private slots:
    void onProfileClicked();
    void onPhoneClicked();
    void onEmailClicked();
    void onCurrencyClicked();
    void onRateClicked();

private:
    QLabel* m_avatarLabel;
    QLabel* m_nicknameLabel;
    QLabel* m_accountLabel;
    
    QPushButton* m_backBtn;
};

#endif // SETTINGS_WIDGET_H
