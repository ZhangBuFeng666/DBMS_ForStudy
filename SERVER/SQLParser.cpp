#include "SQLParser.h"

using namespace std;

// 解析 SQL 字符串为 SQLCommand
SQLCommand SQLParser::parse(const string& sql) {
    using namespace std;

    SQLCommand cmd;
    cmd.type = SQLCommand::UNKNOWN;

    smatch match;

    // 解析 INSERT 语句
    regex insertRegex(R"(INSERT INTO (\w+) VALUES \((.*?)\);?)", regex::icase);
    if (regex_match(sql, match, insertRegex)) {
        cmd.type = SQLCommand::INSERT;
        cmd.tableName = match[1];
        string valuesStr = match[2];
        cmd.values = split_values(valuesStr);
        return cmd;
    }

    // 解析 CREATE TABLE 语句
    regex createTableRegex(
        R"(CREATE\s+TABLE\s+(\w+)\s*\(\s*([^;]+)\s*\)\s*;?)",
        regex::icase);
    if (regex_search(sql, match, createTableRegex)) {
        cmd.type = SQLCommand::CREATE;
        cmd.tableName = match[1];
        parse_field_definitions(match[2], cmd); // 解析字段定义
        return cmd;
    }

    // 解析 DROP TABLE 语句
    regex dropTableRegex(R"(DROP TABLE (\w+);?)", regex::icase);
    if (regex_match(sql, match, dropTableRegex)) {
        cmd.type = SQLCommand::DROP;
        cmd.tableName = match[1];
        return cmd;
    }

    // 解析 UPDATE 语句 (假设 WHERE 子句是行索引)
    regex updateRegex(R"(UPDATE (\w+) SET (.*?) WHERE (\d+);)", regex::icase);
    if (regex_match(sql, match, updateRegex)) {
        cmd.type = SQLCommand::UPDATE;
        cmd.tableName = match[1];
        cmd.newRow = match[2];
        cmd.rowIndex = stoi(match[3]);
        return cmd;
    }

    // 解析 DELETE 语句 (假设 WHERE 子句是行索引)
    regex deleteRegex(R"(DELETE FROM (\w+) WHERE (\d+);)", regex::icase);
    if (regex_match(sql, match, deleteRegex)) {
        cmd.type = SQLCommand::DELETE;
        cmd.tableName = match[1];
        cmd.rowIndex = stoi(match[2]);
        return cmd;
    }

    return cmd; // 如果未匹配任何语句，则类型为 UNKNOWN
}

// 分割 INSERT 语句中的值列表
vector<string> SQLParser::split_values(const string& input) {
    vector<string> result;
    regex valueRegex(R"((?:'[^']*')|(?:[^,]+))");
    auto begin = sregex_iterator(input.begin(), input.end(), valueRegex);
    auto end = sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        string val = it->str();
        if (val.front() == '\'' && val.back() == '\'') {
            val = val.substr(1, val.size() - 2); // 去除单引号
        }
        result.push_back(val);
    }
    return result;
}

// 解析 CREATE TABLE 语句中的字段定义
void SQLParser::parse_field_definitions(const string& fieldDefs, SQLCommand& cmd) {
    // Normalize whitespace: replace multiple spaces with a single space
    string cleanedDefs = regex_replace(fieldDefs, regex(R"(\s+)"), " ");
    // Trim leading and trailing whitespace
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(^\s+|\s+$)"), "");
    // Normalize whitespace around commas: ensure a single space after each comma
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(\s*,\s*)"), ", ");

    regex fieldRegex(
        R"(\s*(\w+)\s+)"            // 字段名 (捕获组 1)杀
        R"((\w+(?:\(\d+\))?))"        // 字段类型（支持CHAR(9)格式） (捕获组 2)
        R"(\s*(PRIMARY\s+KEY)?)"        // 主键约束 (捕获组 3)
        R"(\s*,?\s*)",                    // 结尾逗号
        regex::icase
    );

    sregex_iterator it(cleanedDefs.begin(), cleanedDefs.end(), fieldRegex);
    sregex_iterator end;
    int fieldIndex = 0;

    for (; it != end; ++it) {
        smatch match = *it;
        if (match.size() == 5) {
            std::string fieldName = match[1].str();
            std::string fieldType = match[2].str();
            cmd.fieldDefinitionsWithType.push_back({ fieldName, fieldType });

            // 处理主键约束
            if (match[3].matched) {
                cmd.constraints["primary_key"] = fieldIndex;
            }
            fieldIndex++;
        }
    }
}