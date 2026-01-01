#ifndef PROFILE_EDIT_DIALOG_H
#define PROFILE_EDIT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPixmap>
#include "User.h"

class ProfileEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProfileEditDialog(const User& user, QWidget *parent = nullptr);
    QString getNickname() const { return m_nicknameEdit->text(); }
    QString getAvatarPath() const { return m_avatarPath; }

private slots:
    void onSelectAvatar();
    void onSave();

private:
    QLineEdit *m_nicknameEdit;
    QLabel *m_avatarPreview;
    QString m_avatarPath;
    User m_user;
};

#endif // PROFILE_EDIT_DIALOG_H
