#include "account_record.h"

AccountRecord::AccountRecord(int userId, double amount, const QString& type, const QString& remark)
    : m_userId(userId), m_amount(amount), m_type(type), m_remark(remark) {}
