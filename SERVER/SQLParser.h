#pragma once
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <map>

// 定义 SQL 命令结构体，表示解析后的 SQL 操作及其相关参数
struct SQLCommand {
    enum CommandType { INSERT, CREATE, DROP, UPDATE, DELETE, UNKNOWN } type; // 操作类型
    std::string tableName;        // 表名
    std::string dbName;          // 数据库名 (当前未使用)
    std::vector<std::string> values; // 插入时的字段值
    int rowIndex = -1;          // 用于 UPDATE 和 DELETE 的行号
    std::string newRow;            // 用于 UPDATE 的新行数据

    // 新增字段定义和约束
    std::vector<std::pair<std::string, std::string>> fieldDefinitionsWithType; // {字段名, 字段类型}
    std::map<std::string, int> constraints;      // 约束条件（类型 -> 字段位置）
};

// SQL 解析器类，用于将字符串形式的 SQL 转换为 SQLCommand 结构体
class SQLParser {
public:
    // 解析 SQL 字符串并返回 SQLCommand 结构体
    SQLCommand parse(const std::string& sql);

private:
    // 分割逗号分隔的字符串值
    std::vector<std::string> split_values(const std::string& input);
    // 解析 CREATE TABLE 语句中的字段定义
    void parse_field_definitions(const std::string& fieldDefs, SQLCommand& cmd);
};

#endif // SQLPARSER_H