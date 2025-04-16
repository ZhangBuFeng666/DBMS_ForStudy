#include "SQLParser.h"
#include <iostream> // 用于调试输出
#include <string>   // 确保包含 string
#include <vector>   // 确保包含 vector

using namespace std;

// --- 分割逗号分隔的值 (增强版，处理引号) ---
vector<string> SQLParser::split_values(const string& input) {
    vector<string> result;
    // 正则表达式：匹配被单引号包围的字符串（允许内部连续两个单引号表示一个单引号）或不包含逗号且非引号开头的普通值
    // \s*                                    # 匹配前导空格
    // (?:                                    # 开始非捕获组 (用于选择)
    //   '((?:[^']|'')*)'                      # 匹配单引号包围的值 (捕获组 1):
    //                                          #   '...' 内可以包含非单引号字符 ([^']) 或连续两个单引号 ('')
    //   |                                      # 或者
    //   ([^',]+)                              # 匹配不包含单引号和逗号的非空值 (捕获组 2)
    // )                                      # 结束非捕获组
    // \s*                                    # 匹配尾随空格
    // ,?                                     # 匹配可选的逗号
    regex valueRegex(R"(\s*(?:'((?:[^']|'')*)'|([^',]+))\s*,?)");
    auto begin = sregex_iterator(input.begin(), input.end(), valueRegex);
    auto end = sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        smatch match = *it;
        if (match[1].matched) { // 匹配到引号包围的值 (捕获组 1)
            // 将连续两个单引号 ('') 替换回单个单引号 (')
            result.push_back(regex_replace(match[1].str(), regex("''"), "'"));
        }
        else if (match[2].matched) { // 匹配到非引号包围的值 (捕获组 2)
            result.push_back(trim(match[2].str())); // 去除可能存在的前后空格
        }
    }
    return result;
}

// --- 解析 CREATE TABLE 语句中的字段定义 ---
void SQLParser::parse_field_definitions(const string& fieldDefs, SQLCommand& cmd) {
    // 规范化空白：将多个空白替换为单个空格
    string cleanedDefs = regex_replace(fieldDefs, regex(R"(\s+)"), " ");
    // 去除首尾空白
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(^\s+|\s+$)"), "");
    // 规范化逗号周围的空白：确保每个逗号后有一个空格（可选，主要为了正则匹配方便）
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(\s*,\s*)"), ", ");

    // 正则表达式，用于匹配字段定义，支持 CHAR(N) 和 PRIMARY KEY
    // \s*(\w+)\s+                         # 字段名 (捕获组 1)
    // (\w+(?:\(\s*\d+\s*\))?)             # 字段类型，如 INT, CHAR(10) (允许括号内有空格) (捕获组 2)
    // (?:\s+(PRIMARY\s+KEY))?             # 可选的主键约束 (非捕获组，但内部有捕获组 3)
    // \s*,?\s*                            # 可选的逗号和周围的空白
    regex fieldRegex(
        R"(\s*(\w+)\s+)"
        R"((\w+(?:\(\s*\d+\s*\))?))"
        R"((?:\s+(PRIMARY\s+KEY))?)"
        R"(\s*,?\s*)",
        regex::icase // 忽略大小写
    );

    sregex_iterator it(cleanedDefs.begin(), cleanedDefs.end(), fieldRegex);
    sregex_iterator end;
    int fieldIndex = 0; // 当前字段的索引

    for (; it != end; ++it) {
        smatch match = *it;
        // match[0] 是整个匹配项
        // match[1] 是字段名
        // match[2] 是字段类型
        // match[3] 是 "PRIMARY KEY" (如果存在)
        if (match.size() >= 3 && match[1].matched && match[2].matched) { // 确保捕获到名字和类型
            std::string fieldName = match[1].str();
            std::string fieldType = match[2].str();
            // 规范化类型字符串，例如去除 "CHAR ( 10 )" 中的空格
            fieldType = regex_replace(fieldType, regex(R"(\s+)"), "");
            cmd.fieldDefinitionsWithType.push_back({ fieldName, fieldType });

            // 处理主键约束
            if (match[3].matched) {
                // 检查是否已定义主键
                if (cmd.constraints.count("primary_key")) {
                    // 处理错误：定义了多个主键
                    // 可以报错，或者像这里一样覆盖旧的并给警告
                    cerr << "警告: 定义了多个主键约束，将使用最后一个。" << endl;
                }
                cmd.constraints["primary_key"] = fieldIndex;
            }
            // 在此添加对其他约束（如 NOT NULL, UNIQUE）的解析...

            fieldIndex++;
        }
        else {
            // 如果某段不匹配，给出警告
            string context = it->prefix().str(); // 获取匹配失败位置之前的内容
            size_t lastComma = context.rfind(',');
            if (lastComma != string::npos) context = context.substr(lastComma + 1);
            cerr << "警告: 无法解析字段定义中的片段，靠近: '" << trim(context) << "'" << endl;
        }
    }
    // 可选：检查是否有未解析的尾随字符
    // string remaining = cleanedDefs.substr(it->suffix().first - cleanedDefs.begin());
    // if (!trim(remaining).empty()) {
    //     cerr << "Warning: Unparsed trailing characters in field definitions: " << remaining << endl;
    // }
}


// --- 辅助函数：解析 SET 子句 (例如 "col1 = val1, col2 = 'val 2'") ---
void SQLParser::parse_set_clause(const std::string& setClauseStr, SQLCommand& cmd) {
    cmd.setClauses.clear();
    // 正则表达式匹配 "字段名 = 值" 对，能处理引号包围的值（允许内部'')
    // \s*(\w+)\s*=\s*                    # 匹配 "字段名 =" (捕获组 1: 字段名)
    // (?:                                # 开始非捕获组 (选择值的部分)
    //   '((?:[^']|'')*)'                  #   匹配引号包围的值 (捕获组 2: 值内容)
    //   |                                # 或者
    //   ([^,]+)                          #   匹配不包含逗号的非引号值 (捕获组 3: 值内容)
    // )
    // \s*,?                              # 匹配可选的逗号和空格
    regex setPairRegex(R"(\s*(\w+)\s*=\s*(?:'((?:[^']|'')*)'|([^,]+))\s*,?)");

    auto begin = sregex_iterator(setClauseStr.begin(), setClauseStr.end(), setPairRegex);
    auto end = sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        smatch match = *it;
        string colName = match[1].str();
        string value;
        if (match[2].matched) { // 匹配到引号值
            value = regex_replace(match[2].str(), regex("''"), "'"); // 去转义
        }
        else if (match[3].matched) { // 匹配到非引号值
            value = trim(match[3].str());
        }
        else {
            // 这个正则表达式下理论上不会到这里
            continue;
        }
        cmd.setClauses.push_back({ colName, value });
    }
}

// --- 辅助函数：解析简单的 WHERE 子句 (例如 "WHERE 字段 = 值" 或 "WHERE 字段 = '值'") ---
bool SQLParser::parse_where_clause(const std::string& whereClauseStr, SQLCommand& cmd) {
    // 正则表达式匹配 "WHERE 字段 = 值"
    // WHERE\s+(\w+)\s*=\s*               # 匹配 "WHERE 字段 =" (捕获组 1: 字段名)
    // (?:                               # 开始非捕获组 (选择值的部分)
    //   '((?:[^']|'')*)'                 #   匹配引号值 (捕获组 2: 值内容)
    //   |                               # 或者
    //   ([^;' ]+)                       #   匹配非引号值 (不含分号和空格) (捕获组 3: 值内容)
    // )
    // \s*;?                             # 匹配可选的结尾分号和空格
    regex whereRegex(R"(WHERE\s+(\w+)\s*=\s*(?:'((?:[^']|'')*)'|([^;' ]+))\s*;?)", regex::icase);
    smatch whereMatch;
    // 这里用 regex_match 是因为 WHERE 子句通常是跟在 SET 或 FROM 之后，作为独立的、完整的待解析部分传入
    if (regex_match(whereClauseStr, whereMatch, whereRegex)) {
        cmd.whereColumn = whereMatch[1].str();
        if (whereMatch[2].matched) { // 匹配到引号值
            cmd.whereValue = regex_replace(whereMatch[2].str(), regex("''"), "'"); // 去转义
        }
        else if (whereMatch[3].matched) { // 匹配到非引号值
            cmd.whereValue = trim(whereMatch[3].str());
        }
        else {
            return false; // 不应发生
        }
        return true; // 解析成功
    }
    return false; // WHERE 子句格式不匹配
}


// --- 主解析函数 ---
SQLCommand SQLParser::parse(const string& sqlInput) {
    SQLCommand cmd;
    smatch match;
    string sql = trim(sqlInput); // 去除输入SQL的首尾空白

    // 1. 解析 INSERT INTO table VALUES ('val1', 2, ...);
    // 模式: INSERT INTO <表名> VALUES (<值列表>) ; (可选)
    regex insertRegex(R"(INSERT\s+INTO\s+(\w+)\s+VALUES\s*\((.*?)\)\s*;?)", regex::icase);
    if (regex_match(sql, match, insertRegex)) {
        cmd.type = SQLCommand::INSERT;
        cmd.tableName = match[1];
        string valuesStr = match[2]; // 提取括号内的值字符串
        cmd.values = split_values(valuesStr); // 使用辅助函数分割值
        return cmd;
    }

    // 2. 解析 CREATE TABLE table ( col1 TYPE, ... );
    // 模式: CREATE TABLE <表名> (<字段定义列表>) ; (可选)
    regex createTableRegex(R"(CREATE\s+TABLE\s+(\w+)\s*\((.*?)\)\s*;?)", regex::icase);
    if (regex_match(sql, match, createTableRegex)) { // 使用 regex_match 确保整个字符串匹配
        cmd.type = SQLCommand::CREATE;
        cmd.tableName = match[1];
        parse_field_definitions(match[2].str(), cmd); // 使用辅助函数解析字段定义
        return cmd;
    }


    // 3. 解析 DROP TABLE table;
    // 模式: DROP TABLE <表名> ; (可选)
    regex dropTableRegex(R"(DROP\s+TABLE\s+(\w+)\s*;?)", regex::icase);
    if (regex_match(sql, match, dropTableRegex)) {
        cmd.type = SQLCommand::DROP;
        cmd.tableName = match[1];
        return cmd;
    }

    // 4. 解析 UPDATE table SET col1=val1, ... WHERE col = val;
    // 模式: UPDATE <表名> SET <设置子句> WHERE <条件子句> ; (可选)
    regex updateRegex(R"(UPDATE\s+(\w+)\s+SET\s+(.*?)\s+(WHERE\s+.*);?)", regex::icase);
    // 表名(1)      设置子句(2)   WHERE子句(3)
    if (regex_match(sql, match, updateRegex)) {
        cmd.type = SQLCommand::UPDATE;
        cmd.tableName = match[1];
        parse_set_clause(match[2].str(), cmd); // 解析 SET 部分
        if (!parse_where_clause(match[3].str(), cmd)) { // 解析 WHERE 部分
            cerr << "错误: 无法解析 UPDATE 语句中的 WHERE 子句。" << endl;
            cmd.type = SQLCommand::UNKNOWN; // 如果 WHERE 解析失败，标记为未知命令
        }
        return cmd;
    }

    // 5. 解析 DELETE FROM table WHERE col = val;
    // 模式: DELETE FROM <表名> WHERE <条件子句> ; (可选)
    regex deleteRegex(R"(DELETE\s+FROM\s+(\w+)\s+(WHERE\s+.*);?)", regex::icase);
    //  表名(1)       WHERE子句(2)
    if (regex_match(sql, match, deleteRegex)) {
        cmd.type = SQLCommand::DELETE;
        cmd.tableName = match[1];
        if (!parse_where_clause(match[2].str(), cmd)) { // 解析 WHERE 部分
            cerr << "错误: 无法解析 DELETE 语句中的 WHERE 子句。" << endl;
            cmd.type = SQLCommand::UNKNOWN; // 如果 WHERE 解析失败，标记为未知命令
        }
        return cmd;
    }

    // 6. 解析 ALTER TABLE 命令
    // 基础模式: ALTER TABLE <表名> <操作> ; (可选)
    regex alterTableBaseRegex(R"(ALTER\s+TABLE\s+(\w+)\s+(.*);?)", regex::icase);
    //  表名(1)      操作部分(2)
    if (regex_match(sql, match, alterTableBaseRegex)) {
        cmd.type = SQLCommand::ALTER;
        cmd.tableName = match[1];
        string actionStr = trim(match[2].str()); // 获取操作部分的字符串并去除空白

        smatch actionMatch;

        // 6a. 解析 RENAME TO new_table_name
        // 修改：添加 \s*;?\s*$ 确保匹配到结尾，并处理可选分号
        regex renameTableRegex(R"(RENAME\s+TO\s+(\w+)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, renameTableRegex)) {
            cmd.alterAction = SQLCommand::RENAME_TABLE;
            cmd.newTableName = actionMatch[1]; // 捕获新表名
            cout << "调试: 解析 RENAME TABLE TO " << cmd.newTableName << " 成功。" << endl;
            return cmd;
        }

        // 6b. 解析 ADD [COLUMN] column_name type
        // 修改：添加 \s*;?\s*$
        regex addColumnRegex(R"(ADD\s+(?:COLUMN\s+)?(\w+)\s+(\w+(?:\(\s*\d+\s*\))?)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, addColumnRegex)) {
            cmd.alterAction = SQLCommand::ADD_COLUMN;
            cmd.columnName = actionMatch[1].str();
            string rawType = actionMatch[2].str();
            string normalizedType = regex_replace(rawType, regex(R"(\s+)"), "");
            cmd.columnDefinition = cmd.columnName + " " + normalizedType;
            cout << "调试: 解析 ADD COLUMN 成功。列名: " << cmd.columnName << ", 定义: " << cmd.columnDefinition << endl;
            return cmd;
        }

        // 6c. 解析 DROP [COLUMN] column_name
        // 修改：添加 \s*;?\s*$
        regex dropColumnRegex(R"(DROP\s+(?:COLUMN\s+)?(\w+)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, dropColumnRegex)) {
            cmd.alterAction = SQLCommand::DROP_COLUMN;
            cmd.columnName = actionMatch[1].str(); // 捕获要删除的列名
            cout << "调试: 解析 DROP COLUMN " << cmd.columnName << " 成功。" << endl;
            return cmd;
        }

        // 6d. 解析 MODIFY [COLUMN] column_name new_type
        // 修改：添加 \s*;?\s*$
        regex modifyColumnRegex(R"(MODIFY\s+(?:COLUMN\s+)?(\w+)\s+(\w+(?:\(\s*\d+\s*\))?)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, modifyColumnRegex)) {
            cmd.alterAction = SQLCommand::MODIFY_COLUMN;
            cmd.columnName = actionMatch[1].str(); // 捕获要修改的列名
            string rawNewType = actionMatch[2].str();
            cmd.columnDefinition = regex_replace(rawNewType, regex(R"(\s+)"), ""); // 存储规范化后的新类型定义
            cout << "调试: 解析 MODIFY COLUMN " << cmd.columnName << " TO " << cmd.columnDefinition << " 成功。" << endl;
            return cmd;
        }

        // 6e. 解析 RENAME [COLUMN] old_name TO new_name
        // 修改：添加 \s*;?\s*$
        regex renameColumnRegex(R"(RENAME\s+(?:COLUMN\s+)?(\w+)\s+TO\s+(\w+)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, renameColumnRegex)) {
            cmd.alterAction = SQLCommand::RENAME_COLUMN;
            cmd.columnName = actionMatch[1].str(); // 存储旧列名
            cmd.newColumnName = actionMatch[2].str(); // 存储新列名
            cout << "调试: 解析 RENAME COLUMN " << cmd.columnName << " TO " << cmd.newColumnName << " 成功。" << endl;
            return cmd;
        }

        // 如果 ALTER TABLE 后面的操作部分无法匹配任何已知模式
        cerr << "错误: 未知或无效的 ALTER TABLE 操作: '" << actionStr << "'" << endl;
        cmd.type = SQLCommand::UNKNOWN; // 标记为无效命令
        cmd.alterAction = SQLCommand::INVALID_ALTER;
        return cmd;
        
       
    }
    // 如果没有任何模式匹配成功
    cmd.type = SQLCommand::UNKNOWN;
    return cmd;
}


