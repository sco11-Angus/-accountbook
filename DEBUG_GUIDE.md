# 账单数据保存问题诊断和修复指南

## 问题描述
点击"完成"按钮后，账单数据没有被保存到数据库。

## 原因分析

### 之前的问题（已修复）
1. **SQL 注入风险** - 使用字符串拼接构建 SQL，特殊字符（如引号）会导致 SQL 执行失败
2. **错误信息缺失** - 没有调试输出，无法定位问题
3. **验证不完整** - 缺少用户ID验证和分类选择验证

## 修复内容

### 1. 修复 account_manager.cpp

**改进：从字符串拼接改为参数化查询**

```cpp
// ❌ 旧方式（容易出错）
QString sql = QString(R"(
    INSERT INTO account_record (user_id, amount, type, remark, voucher_path, is_deleted, create_time, modify_time)
    VALUES (%1, %2, '%3', '%4', '%5', 0, '%6', '%7')
)").arg(...);

// ✅ 新方式（安全且可靠）
QString sql = QString(R"(
    INSERT INTO account_record (user_id, amount, type, remark, voucher_path, is_deleted, create_time, modify_time)
    VALUES (?, ?, ?, ?, ?, 0, ?, ?)
)");

QVariantList params;
params << record.getUserId()
       << record.getAmount()
       << record.getType()
       << record.getRemark()
       << record.getVoucherPath()
       << now
       << now;

bool success = m_dbHelper->executeSqlWithParams(sql, params);
```

**优势**：
- 参数与 SQL 分离，避免特殊字符干扰
- 自动类型转换和转义
- 防止 SQL 注入攻击
- 更易调试

### 2. 增强 accountbookrecordwidget.cpp 中的完成按钮逻辑

**添加的改进**：
1. ✅ 完整的输入验证
   - 检查金额是否有效
   - 检查分类是否选中
   - 检查用户是否登录

2. ✅ 详细的调试输出（可在 Output 面板中查看）
   ```
   记账：支出类型 餐饮 金额 -58.5
   当前用户ID： 1
   ====== 准备保存账单 ======
   用户ID: 1
   金额: -58.5
   分类: 餐饮
   备注: ""
   创建时间: "2025-12-29 14:30:45"
   ================
   账单保存成功：用户ID= 1 金额= -58.5 分类= 餐饮
   ```

3. ✅ 用户友好的反馈
   - 成功保存时显示成功提示框
   - 保存失败时显示详细错误信息

### 3. 添加必要的头文件

在 `accountbookrecordwidget.h` 中添加：
```cpp
#include "account_record.h"
#include "account_manager.h"
#include "user_manager.h"
```

## 如何验证修复是否有效

### 方法 1：查看调试输出
1. 在 VS Code 中按 `Ctrl + Shift + U` 打开"输出"面板
2. 选择"应用程序"频道
3. 点击记账窗口的"完成"按钮
4. 查看是否出现类似以下的输出：
```
记账：支出类型 餐饮 金额 -58.5
当前用户ID： 1
====== 准备保存账单 ======
用户ID: 1
金额: -58.5
分类: 餐饮
备注: ""
创建时间: "2025-12-29 14:30:45"
================
账单保存成功：用户ID= 1 金额= -58.5 分类= 餐饮
```

### 方法 2：查看数据库文件
1. 打开文件浏览器
2. 导航到项目根目录：`c:\Users\34399\Desktop\1\1.1\Ledger-Lightweight\`
3. 查看是否存在 `account_book.db` 文件
4. 文件大小应该在修改后增加

### 方法 3：使用数据库客户端查看
1. 下载 SQLite 客户端（如 DB Browser for SQLite）
2. 打开 `account_book.db` 文件
3. 查看 `account_record` 表中是否有新数据

## 数据库表结构

```
account_record 表：
┌─────────┬───────────┬────────┬───────┬────────┬─────────┬──────────┬────────────┬────────────┬─────────────┐
│ id      │ user_id   │ amount │ type  │ remark │ voucher │ is_deleted  │ delete_time │ create_time │ modify_time │
├─────────┼───────────┼────────┼───────┼────────┼─────────┼──────────┼────────────┼────────────┼─────────────┤
│ 自增    │ 用户ID    │ 金额   │ 分类  │ 备注   │ 凭证    │ 0=正常   │ 删除时间    │ 记账时间    │ 修改时间     │
│ (主键)  │ (必填)    │ (必填) │(必填)│(可选)  │ (可选)  │ 1=已删   │            │ 1=自动     │ 1=自动     │
└─────────┴───────────┴────────┴───────┴────────┴─────────┴──────────┴────────────┴────────────┴─────────────┘
```

## 常见问题排查

### Q1：记账窗口不显示，或点击"完成"后窗口闪退？
**A：** 检查是否有编译错误
- 清理构建：Build → Clean
- 重新构建：Build → Build Project
- 检查"问题"面板中的错误信息

### Q2：显示"保存记录失败"错误？
**A：** 可能原因：
1. 数据库文件被锁定（另一个程序正在访问）
2. 用户ID为 0（用户未正确登录）
3. 某个必填字段为空或格式错误

**解决方案：**
- 检查调试输出中的用户ID是否正确
- 关闭其他可能访问数据库的程序
- 删除 `account_book.db` 重新运行程序

### Q3：显示成功但账本主界面没有更新？
**A：** 检查 `billRecorded()` 信号是否被正确接收
- 在 main.cpp 中的 connect 函数打印调试信息
- 确认 `AccountBookMainWidget::updateBillData()` 被调用

### Q4：数据库文件路径不对？
**A：** 默认路径是 `./account_book.db`（相对于应用程序运行目录）
- 如果需要修改路径，在 main.cpp 中初始化数据库时指定

## 代码改进建议（未来可考虑）

1. **事务处理**
   ```cpp
   // 在 addAccountRecord 中使用事务
   m_dbHelper->beginTransaction();
   bool success = m_dbHelper->executeSqlWithParams(sql, params);
   if (success) m_dbHelper->commitTransaction();
   else m_dbHelper->rollbackTransaction();
   ```

2. **自动查询验证**
   ```cpp
   // 保存后立即查询验证
   QList<AccountRecord> saved = queryMonthlyRecords(userId, year, month);
   bool verified = saved.last().getAmount() == amount;
   ```

3. **离线缓存机制**
   ```cpp
   // 如果数据库连接失败，保存到临时缓存，待连接恢复后同步
   ```

## 总结

✅ **主要修复**：
- 使用参数化查询替代字符串拼接
- 添加详细的调试输出
- 增强输入验证和错误处理
- 优化用户反馈提示

✅ **预期效果**：
- 数据能正确保存到 SQLite 数据库
- 记账后立即显示成功/失败提示
- 主窗口自动刷新显示最新账单
- 可通过调试输出跟踪整个过程

如有问题，请检查调试输出并参考此指南的排查部分。
