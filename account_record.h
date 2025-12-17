#ifndef ACCOUNT_RECORD_H
#define ACCOUNT_RECORD_H

#include <QString>

class AccountRecord {
public:
    AccountRecord() = default;
    AccountRecord(int userId, double amount, const QString& type, const QString& remark = "");

    // Getter & Setter
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    int getUserId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    double getAmount() const { return m_amount; }
    void setAmount(double amount) { m_amount = amount; }

    QString getType() const { return m_type; }
    void setType(const QString& type) { m_type = type; }

    QString getRemark() const { return m_remark; }
    void setRemark(const QString& remark) { m_remark = remark; }

    QString getVoucherPath() const { return m_voucherPath; }
    void setVoucherPath(const QString& path) { m_voucherPath = path; }

    int getIsDeleted() const { return m_isDeleted; }
    void setIsDeleted(int isDeleted) { m_isDeleted = isDeleted; }

    QString getDeleteTime() const { return m_deleteTime; }
    void setDeleteTime(const QString& time) { m_deleteTime = time; }

    QString getCreateTime() const { return m_createTime; }
    void setCreateTime(const QString& time) { m_createTime = time; }

    QString getModifyTime() const { return m_modifyTime; }
    void setModifyTime(const QString& time) { m_modifyTime = time; }

private:
    int m_id = 0;
    int m_userId = 0;       // 所属用户ID
    double m_amount = 0.0;  // 金额（正数：收入，负数：支出）
    QString m_type;         // 收支类型（餐饮/交通等）
    QString m_remark;       // 备注
    QString m_voucherPath;  // 凭证图片路径
    int m_isDeleted = 0;    // 0:正常 1:回收站
    QString m_deleteTime;   // 删除时间
    QString m_createTime;   // 创建时间
    QString m_modifyTime;   // 修改时间
};

#endif // ACCOUNT_RECORD_H
