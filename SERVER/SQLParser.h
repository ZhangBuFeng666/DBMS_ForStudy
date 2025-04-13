#pragma once
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <map>

using namespace std;

// 定义 SQL 命令结构体，表示解析后的 SQL 操作及其相关参数
struct SQLCommand {
    enum CommandType { INSERT, CREATE, DROP, UPDATE, DELETE, UNKNOWN } type; // 操作类型
    string tableName;     // 表名
    string dbName;        // 数据库名
    vector<string> values; // 插入时的字段值
    int rowIndex = -1;     // 用于 UPDATE 和 DELETE 的行号
    string newRow;         // 用于 UPDATE 的新行数据

    // 新增字段定义和约束
    vector<string> fieldDefinitions; // 字段定义（完整字符串）
    map<string, int> constraints;     // 约束条件（类型 -> 字段位置）
};

// SQL 解析器类，用于将字符串形式的 SQL 转换为 SQLCommand 结构体
class SQLParser {
public:
    SQLCommand parse(const string& sql);

private:
    vector<string> split_values(const string& input);

    // 新增解析字段定义的私有方法
    void parse_field_definitions(const string& fieldDefs, SQLCommand& cmd);
};

#endif // SQLPARSER_H

