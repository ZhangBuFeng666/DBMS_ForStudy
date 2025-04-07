#pragma once
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>

using namespace std;

// 定义 SQL 命令结构体，表示解析后的 SQL 操作及其相关参数
struct SQLCommand {
    enum CommandType { INSERT, CREATE, DROP, UPDATE, DELETE, UNKNOWN } type; // 操作类型
    string tableName;     // 表名
    string dbName;        // 数据库名
    vector<string> values; // 插入时的字段值
    int rowIndex = -1;     // 用于 UPDATE 和 DELETE 的行号
    string newRow;         // 用于 UPDATE 的新行数据
};

// SQL 解析器类，用于将字符串形式的 SQL 转换为 SQLCommand 结构体
class SQLParser {
public:
    SQLCommand parse(const string& sql);

private:
    vector<string> split_values(const string& input);
};

#endif // SQLPARSER_H

