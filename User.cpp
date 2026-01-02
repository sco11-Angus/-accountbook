#include "User.h"

User::User(const QString& account, const QString& password, const QString& nickname)
    : m_account(account), m_password(password), m_nickname(nickname) {}
