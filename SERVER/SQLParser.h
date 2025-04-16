#pragma once
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <map>
#include <variant> // 用于 ALTER 操作选项

// 定义 SQL 命令结构体，表示解析后的 SQL 操作及其相关参数
struct SQLCommand {
    // 基本命令类型枚举
    enum CommandType {
        INSERT,     // 插入
        CREATE,     // 创建表
        DROP,       // 删除表
        UPDATE,     // 更新
        DELETE,     // 删除
        ALTER,      // 修改表
        UNKNOWN     // 未知或错误
    } type = UNKNOWN; // 操作类型, 默认为未知

    // 通用字段
    std::string tableName;  // 表名
    std::string dbName;     // 数据库名 (用于上下文)

    // INSERT 特定字段
    std::vector<std::string> values; // 插入时的字段值列表

    // CREATE 特定字段
    std::vector<std::pair<std::string, std::string>> fieldDefinitionsWithType; // 字段定义列表 {字段名, 字段类型}
    std::map<std::string, int> constraints; // 约束条件映射 (约束类型 -> 字段索引, 例如 {"primary_key", 0})

    // UPDATE 特定字段
    std::vector<std::pair<std::string, std::string>> setClauses; // SET 子句列表 {列名, 新值}

    // UPDATE/DELETE 特定字段 (WHERE 字段 = 值)
    std::string whereColumn; // WHERE 子句中的字段名
    std::string whereValue;  // WHERE 子句中的比较值

    // --- ALTER 特定字段 ---
    // ALTER TABLE 操作的具体类型
    enum AlterAction {
        RENAME_TABLE,   // 重命名表
        ADD_COLUMN,     // 添加列
        DROP_COLUMN,    // 删除列
        MODIFY_COLUMN,  // 修改列定义
        RENAME_COLUMN,  // 重命名列
        INVALID_ALTER   // 无效的 ALTER 操作
    } alterAction = INVALID_ALTER; // ALTER 操作类型, 默认为无效

    // 不同 ALTER 操作使用的字段
    std::string newTableName;     // 用于 RENAME TABLE
    std::string columnName;       // 用于 ADD, DROP, MODIFY, RENAME (旧列名)
    std::string columnDefinition; // 用于 ADD (如 "age INT"), MODIFY (新类型定义)
    std::string newColumnName;    // 用于 RENAME COLUMN (新列名)

};

// +++ 修改 trim 函数定义 +++
inline std::string trim(const std::string& str) {
    // 谓词：检查一个字符是否是空白符，安全地处理各种 char 值
    auto is_space_safe = [](unsigned char ch) { // 使用 lambda 包装 isspace
        return ::isspace(ch);
        };

    // 找到第一个非空白字符
    auto first = std::find_if_not(str.begin(), str.end(), is_space_safe);
    if (first == str.end()) {
        return ""; // 如果字符串全为空白或为空，返回空字符串
    }

    // 找到最后一个非空白字符 (从后往前找)
    // 注意：需要使用 .base() 来获取对应的正向迭代器
    auto last = std::find_if_not(str.rbegin(), str.rend(), is_space_safe).base();

    return std::string(first, last); // 构造子字符串
}

// SQL 解析器类
class SQLParser {
public:
    // 解析 SQL 字符串并返回 SQLCommand 结构体
    SQLCommand parse(const std::string& sql);

private:
    // 分割逗号分隔的字符串值 (处理引号)
    std::vector<std::string> split_values(const std::string& input);
    // 解析 CREATE TABLE 语句中的字段定义
    void parse_field_definitions(const std::string& fieldDefs, SQLCommand& cmd);
    // 辅助函数：解析 UPDATE 语句中的 SET 子句
    void parse_set_clause(const std::string& setClauseStr, SQLCommand& cmd);
    // 辅助函数：解析 WHERE 字段=值 子句
    bool parse_where_clause(const std::string& whereClauseStr, SQLCommand& cmd);
};

#endif // SQLPARSER_H