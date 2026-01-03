#include "settings_widget.h"
#include "user_manager.h"
#include "profile_edit_dialog.h"
#include "bind_dialog.h"
#include "currency_dialog.h"
#include "budget_dialog.h"
#include "budget_manager.h"
#include <QPainter>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(450, 650);
    initUI();
    initStyleSheet();
}

void SettingsWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部标题栏
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    
    m_backBtn = new QPushButton("<");
    m_backBtn->setFixedSize(40, 40);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setObjectName("backBtn");
    m_backBtn->hide(); // 隐藏返回按钮，因为现在是底部导航栏页面
    connect(m_backBtn, &QPushButton::clicked, this, &QWidget::close);

    QLabel *titleLabel = new QLabel("设置");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");

    headerLayout->addWidget(m_backBtn);
    headerLayout->addStretch();
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addSpacing(40); // 占位保持标题居中

    mainLayout->addWidget(headerWidget);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(20, 25, 20, 20); // 增加顶部边距从 10 到 25
    contentLayout->setSpacing(15);

    // 1. 个人信息板块
    QPushButton *profileCard = new QPushButton();
    profileCard->setFixedHeight(90); // 设置固定高度，确保显示全
    profileCard->setObjectName("profileCard");
    profileCard->setCursor(Qt::PointingHandCursor);
    profileCard->setStyleSheet(R"(
        QPushButton#profileCard {
            background-color: white;
            border-radius: 15px;
            border: none;
            text-align: left;
        }
        QPushButton#profileCard:hover {
            background-color: #f5f5f5;
        }
    )");
    QHBoxLayout *profileLayout = new QHBoxLayout(profileCard);
    
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(60, 60);
    m_avatarLabel->setObjectName("avatarLabel");
    // 黑底黑边，白人头
    m_avatarLabel->setStyleSheet(R"(
        QLabel#avatarLabel {
            background-color: black;
            border: 2px solid black;
            border-radius: 30px;
            color: white;
            font-size: 30px;
        }
    )");
    m_avatarLabel->setText("👤");
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    
    QVBoxLayout *infoLayout = new QVBoxLayout();
    User currentUser = UserManager::getInstance()->getCurrentUser();
    m_nicknameLabel = new QLabel(currentUser.getNickname().isEmpty() ? "默认昵称" : currentUser.getNickname());
    m_nicknameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");
    m_accountLabel = new QLabel(currentUser.getAccount());
    m_accountLabel->setStyleSheet("font-size: 12px; color: #666;");
    
    infoLayout->addWidget(m_nicknameLabel);
    infoLayout->addWidget(m_accountLabel);
    
    profileLayout->addWidget(m_avatarLabel);
    profileLayout->addLayout(infoLayout);
    profileLayout->addStretch();
    profileLayout->addWidget(new QLabel(">"));

    connect(profileCard, &QPushButton::clicked, this, &SettingsWidget::onProfileClicked);
    contentLayout->addWidget(profileCard);

    // 2. 账号绑定板块
    QFrame *accountSection = createSectionFrame();
    QVBoxLayout *accountLayout = new QVBoxLayout(accountSection);
    accountLayout->setSpacing(0);
    accountLayout->setContentsMargins(0, 5, 0, 5);
    
    QPushButton *phoneItem = createSettingItem("📱", "绑定手机", currentUser.getAccount());
    QPushButton *emailItem = createSettingItem("📧", "绑定邮箱", "未绑定");
    connect(phoneItem, &QPushButton::clicked, this, &SettingsWidget::onPhoneClicked);
    connect(emailItem, &QPushButton::clicked, this, &SettingsWidget::onEmailClicked);
    
    accountLayout->addWidget(phoneItem);
    accountLayout->addWidget(emailItem);
    contentLayout->addWidget(accountSection);

    // 3. 财务板块
    QFrame *financeSection = createSectionFrame();
    QVBoxLayout *financeLayout = new QVBoxLayout(financeSection);
    financeLayout->setSpacing(0);
    financeLayout->setContentsMargins(0, 5, 0, 5);
    
    BudgetInfo budget = BudgetManager::getInstance()->getBudget(currentUser.getId());
    QPushButton *budgetItem = createSettingItem("💰", "预算设置", QString("¥%1").arg(budget.daily, 0, 'f', 2));
    connect(budgetItem, &QPushButton::clicked, this, &SettingsWidget::onBudgetClicked);
    financeLayout->addWidget(budgetItem);
    
    QPushButton *currencyItem = createSettingItem("💵", "货币", "人民币 (CNY)");
    QPushButton *rateItem = createSettingItem("📈", "汇率", "1.0000");
    connect(currencyItem, &QPushButton::clicked, this, &SettingsWidget::onCurrencyClicked);
    connect(rateItem, &QPushButton::clicked, this, &SettingsWidget::onRateClicked);
    
    financeLayout->addWidget(currencyItem);
    financeLayout->addWidget(rateItem);
    contentLayout->addWidget(financeSection);

    // 4. 其他板块
    QFrame *otherSection = createSectionFrame();
    QVBoxLayout *otherLayout = new QVBoxLayout(otherSection);
    otherLayout->setSpacing(0);
    otherLayout->setContentsMargins(0, 5, 0, 5);
    otherLayout->addWidget(createSettingItem("📜", "用户协议"));
    otherLayout->addWidget(createSettingItem("🛡️", "隐私协议"));
    otherLayout->addWidget(createSettingItem("💬", "问题反馈"));
    contentLayout->addWidget(otherSection);

    contentLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    updateProfileDisplay();
}

QFrame* SettingsWidget::createSectionFrame()
{
    QFrame *frame = new QFrame();
    frame->setObjectName("sectionFrame");
    frame->setStyleSheet(R"(
        QFrame#sectionFrame {
            background-color: white;
            border-radius: 15px;
        }
    )");
    return frame;
}

QPushButton* SettingsWidget::createSettingItem(const QString& icon, const QString& title, const QString& value)
{
    QPushButton *item = new QPushButton();
    item->setFixedHeight(50);
    item->setCursor(Qt::PointingHandCursor);
    item->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            border: none;
            text-align: left;
        }
        QPushButton:hover {
            background-color: #f5f5f5;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(15, 0, 15, 0);

    QLabel *iconLabel = new QLabel(icon);
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 14px; color: #333;");
    
    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addStretch();

    if (!value.isEmpty()) {
        QLabel *valLabel = new QLabel(value);
        valLabel->setObjectName("valueLabel");
        valLabel->setStyleSheet("font-size: 13px; color: #999;");
        layout->addWidget(valLabel);
    }

    QLabel *arrowLabel = new QLabel(">");
    arrowLabel->setStyleSheet("color: #CCC; font-weight: bold;");
    layout->addWidget(arrowLabel);

    return item;
}

void SettingsWidget::updateProfileDisplay()
{
    User user = UserManager::getInstance()->getCurrentUser();
    m_nicknameLabel->setText(user.getNickname());
    m_accountLabel->setText(user.getAccount());
    
    if (!user.getAvatar().isEmpty() && QFile::exists(user.getAvatar())) {
        QPixmap pix(user.getAvatar());
        m_avatarLabel->setPixmap(pix.scaled(60, 60, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        m_avatarLabel->setText("");
    } else {
        m_avatarLabel->setPixmap(QPixmap());
        m_avatarLabel->setText("👤");
    }
}

void SettingsWidget::onProfileClicked()
{
    ProfileEditDialog dlg(UserManager::getInstance()->getCurrentUser(), this);
    if (dlg.exec() == QDialog::Accepted) {
        User user = UserManager::getInstance()->getCurrentUser();
        user.setNickname(dlg.getNickname());
        user.setAvatar(dlg.getAvatarPath());
        
        if (UserManager::getInstance()->updateUserInfo(user)) {
            UserManager::getInstance()->setCurrentUser(user);
            updateProfileDisplay();
            QMessageBox::information(this, "提示", "修改成功");
        }
    }
}

void SettingsWidget::onPhoneClicked()
{
    BindDialog dlg("手机号", UserManager::getInstance()->getCurrentUser().getAccount(), this);
    if (dlg.exec() == QDialog::Accepted) {
        User user = UserManager::getInstance()->getCurrentUser();
        user.setAccount(dlg.getNewValue());
        if (UserManager::getInstance()->updateUserInfo(user)) {
            UserManager::getInstance()->setCurrentUser(user);
            updateProfileDisplay();
            // 更新显示
            QPushButton *btn = qobject_cast<QPushButton*>(sender());
            if (btn) {
                QLabel *valLabel = btn->findChild<QLabel*>("valueLabel");
                if (valLabel) valLabel->setText(user.getAccount());
            }
            QMessageBox::information(this, "提示", "手机号修改成功");
        }
    }
}

void SettingsWidget::onEmailClicked()
{
    BindDialog dlg("邮箱", "未绑定", this);
    if (dlg.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "提示", "邮箱绑定成功: " + dlg.getNewValue());
        QPushButton *btn = qobject_cast<QPushButton*>(sender());
        if (btn) {
            QLabel *valLabel = btn->findChild<QLabel*>("valueLabel");
            if (valLabel) valLabel->setText(dlg.getNewValue());
        }
    }
}

void SettingsWidget::onBudgetClicked()
{
    int userId = UserManager::getInstance()->getCurrentUser().getId();
    BudgetDialog dlg(userId, this);
    if (dlg.exec() == QDialog::Accepted) {
        BudgetInfo budget = BudgetManager::getInstance()->getBudget(userId);
        QPushButton *btn = qobject_cast<QPushButton*>(sender());
        if (btn) {
            QLabel *valLabel = btn->findChild<QLabel*>("valueLabel");
            if (valLabel) valLabel->setText(QString("¥%1").arg(budget.daily, 0, 'f', 2));
        }
        QMessageBox::information(this, "提示", "预算设置已更新");
    }
}

void SettingsWidget::onCurrencyClicked()
{
    CurrencyDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString currency = dlg.getSelectedCurrency();
        QPushButton *btn = qobject_cast<QPushButton*>(sender());
        if (btn) {
            QLabel *valLabel = btn->findChild<QLabel*>("valueLabel");
            if (valLabel) valLabel->setText(currency);
        }
    }
}

void SettingsWidget::onRateClicked()
{
    // 模拟不同货币的汇率
    QString rates = "当前汇率：\n"
                    "美元 (USD): 7.0017\n"
                    "日元 (JPY): 0.0468\n"
                    "欧元 (EUR): 7.6543\n"
                    "英镑 (GBP): 8.9012";
    QMessageBox::information(this, "汇率查询", rates);
}

void SettingsWidget::initStyleSheet()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(R"(
        SettingsWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E5, stop:0.5 #F0FFF0, stop:1 #E0F7FA);
        }
        QPushButton#backBtn {
            background: transparent;
            font-size: 20px;
            font-weight: bold;
            color: #333;
            border: none;
        }
        QPushButton#backBtn:hover {
            color: #007AFF;
        }
    )");
}
