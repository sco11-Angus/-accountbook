#include "profile_edit_dialog.h"
#include <QMessageBox>

ProfileEditDialog::ProfileEditDialog(const User& user, QWidget *parent) 
    : QDialog(parent), m_user(user), m_avatarPath(user.getAvatar()) 
{
    setWindowTitle("修改个人信息");
    setFixedSize(300, 400);
    setStyleSheet("background-color: white;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 40, 30, 30);

    // 头像预览
    m_avatarPreview = new QLabel();
    m_avatarPreview->setFixedSize(100, 100);
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    m_avatarPreview->setStyleSheet("border: 2px solid #EEE; border-radius: 50px; background-color: #F9F9F9;");
    
    if (m_avatarPath.isEmpty() || !QFile::exists(m_avatarPath)) {
        m_avatarPreview->setText("👤");
        m_avatarPreview->setStyleSheet(m_avatarPreview->styleSheet() + "font-size: 50px; color: #CCC;");
    } else {
        QPixmap pix(m_avatarPath);
        m_avatarPreview->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }

    QPushButton *selectBtn = new QPushButton("更换头像");
    selectBtn->setStyleSheet("QPushButton { background: #007AFF; color: white; border-radius: 5px; padding: 5px; }");
    connect(selectBtn, &QPushButton::clicked, this, &ProfileEditDialog::onSelectAvatar);

    // 昵称输入
    m_nicknameEdit = new QLineEdit(user.getNickname());
    m_nicknameEdit->setPlaceholderText("请输入昵称");
    m_nicknameEdit->setStyleSheet("QLineEdit { border: 1px solid #DDD; border-radius: 5px; padding: 8px; }");

    // 保存按钮
    QPushButton *saveBtn = new QPushButton("确定");
    saveBtn->setFixedHeight(40);
    saveBtn->setStyleSheet("QPushButton { background: #007AFF; color: white; border-radius: 5px; font-weight: bold; }");
    connect(saveBtn, &QPushButton::clicked, this, &ProfileEditDialog::onSave);

    layout->addWidget(m_avatarPreview, 0, Qt::AlignCenter);
    layout->addWidget(selectBtn);
    layout->addWidget(new QLabel("昵称:"));
    layout->addWidget(m_nicknameEdit);
    layout->addStretch();
    layout->addWidget(saveBtn);
}

void ProfileEditDialog::onSelectAvatar() {
    QString fileName = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg)");
    if (!fileName.isEmpty()) {
        m_avatarPath = fileName;
        QPixmap pix(m_avatarPath);
        m_avatarPreview->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        m_avatarPreview->setText("");
    }
}

void ProfileEditDialog::onSave() {
    if (m_nicknameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "昵称不能为空");
        return;
    }
    accept();
}
