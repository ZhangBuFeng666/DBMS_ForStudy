#include "SQLParser.h"

#include <iostream> // 用于调试输出
#include <string>   // 确保包含 string
#include <vector>   // 确保包含 vector
#include <algorithm>
#include <cctype>

using namespace std;


static bool iequals(const string& a, const string& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char a, char b) { return tolower(a) == tolower(b); });
}
//修改5.2

// --- 分割逗号分隔的值 (增强版，处理引号) ---
vector<string> SQLParser::split_values(const string& input) {
    vector<string> result;
    // 修改后的正则表达式说明：
    // 1. \s*                                   # 前导空格
    // 2. (?:                                   # 非捕获组开始
    //      '((?:[^']|'')*)'                    # 单引号字符串 (捕获组1)
    //      |                                   # 或
    //      (NULL)                              # NULL关键字 (捕获组2)
    //      |                                   # 或
    //      ([^',]+)                            # 非引号普通值 (捕获组3)
    //    )                                     # 非捕获组结束
    // 3. \s*                                   # 尾随空格
    // 4. ,?                                    # 可选逗号
    regex valueRegex(R"(\s*(?:'((?:[^']|'')*)'|(NULL)|([^',]+))\s*,?)", regex::icase);

    auto begin = sregex_iterator(input.begin(), input.end(), valueRegex);
    auto end = sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        smatch match = *it;
        if (match[1].matched) { // 匹配到引号包围的值
            // 处理转义单引号 ('' -> ')
            result.push_back(regex_replace(match[1].str(), regex("''"), "'"));
        }
        else if (match[2].matched) { // 匹配到NULL关键字
            // 将NULL转为空字符串表示（根据我们的NULL处理约定）
            result.push_back("");
        }
        else if (match[3].matched) { // 匹配到普通值
            string val = trim(match[3].str());
            // 检查是否为未加引号的NULL（不区分大小写）
            if (iequals(val, "NULL")) {
                result.push_back("");
    }
            else {
                result.push_back(val);
            }
        }
    }

    // 调试输出
    cout << "调试: 分割值列表 '" << input << "' => [";
    for (size_t i = 0; i < result.size(); ++i) {
        if (i > 0) cout << ", ";
        cout << (result[i].empty() ? "NULL" : "'" + result[i] + "'");
    }
    cout << "]" << endl;

    return result;
}

// --- 解析 CREATE TABLE 语句中的字段定义 ---
void SQLParser::parse_table_definitions(const string& tableDefs, SQLCommand& cmd) {
    // 规范化空白：将多个空白替换为单个空格
    string cleanedDefs = regex_replace(tableDefs, regex(R"(\s+)"), " ");
    // 去除首尾空白
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(^\s+|\s+$)"), "");
    // 规范化逗号周围的空白：确保每个逗号后有一个空格（可选，主要为了正则匹配方便）
    cleanedDefs = regex_replace(cleanedDefs, regex(R"(\s*,\s*)"), ", ");

    // 正则表达式，用于匹配字段定义，支持 CHAR(N) 和 PRIMARY KEY
    // \s*(\w+)\s+                         # 字段名 (捕获组 1)
    // (\w+(?:\(\s*\d+\s*\))?)             # 字段类型，如 INT, CHAR(10) (允许括号内有空格) (捕获组 2)
    // (?:\s+(PRIMARY\s+KEY))?             # 可选的主键约束 (非捕获组，但内部有捕获组 3)
    // \s*,?\s*                            # 可选的逗号和周围的空白
    regex tableRegex(
        R"(\s*(\w+)\s+)"
        R"((\w+(?:\(\s*\d+\s*\))?))"
        R"((?:\s+(PRIMARY\s+KEY))?)"
        R"(\s*,?\s*)",
        regex::icase // 忽略大小写
    );

    sregex_iterator it(cleanedDefs.begin(), cleanedDefs.end(), tableRegex);
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

// 辅助函数：解析 CREATE USER 语句中用户定义的部分
// userDefs 示例: "myuser IDENTIFIED BY 'mypass' WITH 1"
void SQLParser::parse_user_definitions(const std::string& userDefs, SQLCommand& cmd) {
    // 正则表达式，用于匹配 CREATE USER 后面的语法:
    // username IDENTIFIED BY 'password' [WITH right]
    // \s*(\w+)\s+                   # 1: 用户名 (一个或多个字母数字下划线)
    // IDENTIFIED\s+BY\s+          # 匹配关键字 IDENTIFIED BY
    // '([^']*)'                    # 2: 密码 (单引号内，非贪婪匹配任何非单引号字符)
    // (?:\s+WITH\s+(\d+))?         # 可选的 WITH 子句 (非捕获组?:), 内部 (\d+) 捕获数字权限 (3)
    // \s*;?                         # 可选的尾部空白和分号
    std::regex userRegex(
        R"(\s*(\w+)\s+)"             // 1: username
        R"(IDENTIFIED\s+BY\s+)"
        R"('([^']*)')"              // 2: password inside single quotes
        R"((?:\s+WITH\s+(\d+))?)"   // 3: optional integer right
        R"(\s*;?)",                 // optional trailing semicolon and space
        std::regex::icase           // 忽略大小写
    );

    smatch match;

    if (std::regex_match(userDefs, match, userRegex)) {
        cmd.userID = match[1].str();
        cmd.userPassword = match[2].str();

        // 检查是否匹配了可选的 WITH 子句 (捕获组 3)
        if (match[3].matched) {
            try {
                std::string right = match[3].str();
                if(right=="admin")
                    cmd.right = 0;
                else if(right=="base") {
                    cmd.right = 1;
                }
                else {
                    cmd.right = -1;
                }
            }
            catch (const std::invalid_argument& ia) {
                std::cerr << "错误: CREATE USER 语句中 WITH 子句的权限值无效 (非整数): '" << match[3].str() << "'" << std::endl;
                cmd.right = -1; // 解析失败时设置为默认值或错误值
                // 或者可以选择在这里标记解析错误
            }
            catch (const std::out_of_range& oor) {
                std::cerr << "错误: CREATE USER 语句中 WITH 子句的权限值超出整数范围: '" << match[3].str() << "'" << std::endl;
                cmd.right = -1; // 解析失败时设置为默认值或错误值
                // 或者可以选择在这里标记解析错误
            }
        }
        else {
            cmd.right = 1; // 如果没有 WITH 子句，权限设置为默认值 1
        }
    }
    else {
        std::cerr << "错误: 无法解析 CREATE USER 关键字后的用户定义: '" << userDefs << "'" << std::endl;
        // 解析失败，可以将命令类型标记为 UNKNOWN 或者设置错误状态
        cmd.type = SQLCommand::UNKNOWN;
    }
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
    regex setPairRegex(R"(\s*(\w+)\s*=\s*(?:'((?:[^']|'')*)'|NULL|([^,]+))\s*,?)", regex::icase);
    //同样修改非空正则5.2-------------
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

// --- 修改 parse_where_clause 以支持 = 和 IN ---
// 参数: whereClauseStrFull - 包含 "WHERE" 关键字的完整子句字符串
// 返回: true 如果解析成功 (即使是空条件), false 如果格式错误
bool SQLParser::parse_where_clause(const std::string& whereClauseStrFull, SQLCommand& cmd) {
    string whereClause = trim(whereClauseStrFull); // 去除首尾空白
    cmd.hasWhere = false; // 重置状态
    cmd.useInClause = false;
    cmd.whereColumn = "";
    cmd.whereValue = "";
    cmd.inValues.clear();
    cmd.useIsNullClause = false;
    cmd.isNot = false;

    if (whereClause.empty() || whereClause.rfind("WHERE", 0) != 0) {
        if (!whereClause.empty()) {
            cerr << "错误: 无效的 WHERE 子句格式 (缺少 WHERE 关键字?): " << whereClauseStrFull << endl;
            return false;
        }
        return true;
    }

    // 修改正则表达式支持 >, >=, <, <=
    regex whereCompRegex(R"(WHERE\s+(\w+)\s*(<>|>=|<=|=|>|<)\s*(?:'((?:[^']|'')*)'|([^; ]+))\s*;?)", regex::icase);
    smatch matchComp;
    if (regex_match(whereClause, matchComp, whereCompRegex)) {
        cmd.hasWhere = true;
        cmd.useInClause = false;
        cmd.whereColumn = matchComp[1].str();
        cmd.whereOperator = matchComp[2].str();
        if (matchComp[3].matched) {
            cmd.whereValue = regex_replace(matchComp[3].str(), regex("''"), "'");
        }
        else if (matchComp[4].matched) {
            cmd.whereValue = trim(matchComp[4].str());
        }
        else {
            cmd.hasWhere = false;
            cerr << "错误: WHERE 子句中无法解析值部分。" << endl;
            return false;
        }
        cout << "调试: 解析 WHERE 成功。列: " << cmd.whereColumn << ", 运算符: "
            << cmd.whereOperator << ", 值: '" << cmd.whereValue << "'" << endl;
        return true;
    }

    regex whereInRegex(R"(WHERE\s+(\w+)\s+IN\s*\((.*?)\)\s*;?)", regex::icase);
    smatch matchIn;
    if (regex_match(whereClause, matchIn, whereInRegex)) {
        cmd.hasWhere = true; // 确认有有效的 WHERE 条件
        cmd.useInClause = true;
        cmd.whereColumn = matchIn[1].str();
        cmd.inValues = split_values(matchIn[2].str());
        cout << "调试: 解析 WHERE IN 成功。列: " << cmd.whereColumn << endl;
        return true;
    }

    regex whereIsNullRegex(R"(WHERE\s+(\w+)\s+IS\s+(NOT\s+)?NULL\s*;?)", regex::icase);
    smatch matchIsNull;
    if (regex_match(whereClause, matchIsNull, whereIsNullRegex)) {
        cmd.hasWhere = true;
        cmd.useIsNullClause = true;
        cmd.whereColumn = matchIsNull[1].str();
        cmd.isNot = matchIsNull[2].matched;
        cout << "调试: 解析 WHERE IS " << (cmd.isNot ? "NOT " : "") << "NULL 成功" << endl;
        return true;
    }

    cerr << "错误: 无法解析 WHERE 子句" << endl;
    return false;
}

// --- 新增：解析 SELECT 列列表 ---
// 参数: selectListStr - SELECT 关键字之后，FROM 关键字之前的部分
void SQLParser::parse_select_list(const std::string& selectListStr, SQLCommand& cmd) {
    cmd.selectColumns.clear(); // 清空旧数据
    cmd.distinct = false;     // 重置 distinct 标志
    string listStr = trim(selectListStr); // 去除首尾空白

    // 检查是否有 DISTINCT 关键字 (忽略大小写)
    // 使用 regex_search 在开头查找 "DISTINCT "
    if (regex_search(listStr, regex(R"(^DISTINCT\s+)", regex::icase))) {
        cmd.distinct = true;
        // 移除 "DISTINCT " 部分，得到后面的列列表
        listStr = regex_replace(listStr, regex(R"(^DISTINCT\s+)", regex::icase), "");
        listStr = trim(listStr); // 再次 trim
        cout << "调试: 检测到 DISTINCT 关键字。" << endl;
    }

    // 检查是否为 "*" (选择所有列)
    if (listStr == "*") {
        cmd.selectColumns.push_back("*");
        cout << "调试: 解析 SELECT * (所有列)。" << endl;
        return;
    }

    // 按逗号分割列名
    // 正则：匹配单词字符列名，忽略前后空格，后面跟可选逗号
    regex colRegex(R"(\s*(\w+)\s*,?)");
    auto begin = sregex_iterator(listStr.begin(), listStr.end(), colRegex);
    auto end = sregex_iterator();
    bool columnsFound = false;
    string parsedColsStr; // 用于调试输出

    for (auto it = begin; it != end; ++it) {
        smatch match = *it;
        if (match[1].matched) {
            string colName = match[1].str();
            cmd.selectColumns.push_back(colName);
            parsedColsStr += (columnsFound ? ", " : "") + colName; // 拼接调试字符串
            columnsFound = true;
        }
    }

    // 基本校验：如果不是 * 且分割后列表为空，说明可能有格式问题
     if (!columnsFound && listStr != "*") {
          cerr << "警告: 解析 SELECT 列列表时出错，或列表为空/格式无效: '" << selectListStr << "'" << endl;
          // 此时 cmd.selectColumns 会是空的
     } else if (columnsFound) {
         cout << "调试: 解析 SELECT 列列表: " << parsedColsStr << endl;
     }
}


bool SQLParser::parse_order_by_clause(const std::string& orderByStrFull, SQLCommand& cmd) {
    string orderByClause = trim(orderByStrFull);
    cmd.orderByColumn = ""; // 重置状态
    cmd.sortOrder = SQLCommand::ASC; // 默认升序

    if (orderByClause.empty() || orderByClause.rfind("ORDER BY", 0) != 0) { // 必须以 ORDER BY 开头 (忽略大小写)
        if (!orderByClause.empty()) {
            cerr << "错误: 无效的 ORDER BY 子句格式 (缺少 ORDER BY 关键字?): " << orderByStrFull << endl;
        }
        return false; // 格式不对或为空
    }

    // 正则：ORDER BY <列名> [ASC|DESC] ; (可选)
    // ORDER\s+BY\s+      # ORDER BY
    // (\w+)              # 列名 (捕获组 1)
    // (?:\s+(ASC|DESC))? # 可选的 ASC 或 DESC (非捕获组，内部捕获组 2)
    // \s*;?              # 可选分号和结尾空格
    regex orderByRegex(R"(ORDER\s+BY\s+(\w+)(?:\s+(ASC|DESC))?\s*;?)", regex::icase);
    smatch match;
    // 用 regex_match 匹配整个 orderByClause
    if (regex_match(orderByClause, match, orderByRegex)) {
        cmd.orderByColumn = match[1].str();

        if (match[2].matched) { // 检查是否显式指定了 ASC 或 DESC
            string order = match[2].str();
            // 转换为大写进行比较，更健壮
            std::transform(order.begin(), order.end(), order.begin(), ::toupper);
            if (order == "DESC") {
                cmd.sortOrder = SQLCommand::DESC;
            }
            else if (order == "ASC") {
                cmd.sortOrder = SQLCommand::ASC;
            }
            else {
                // 理论上正则限制了只会是 ASC 或 DESC，但以防万一
                cerr << "警告: 无效的排序指示符 '" << match[2].str() << "'，将使用默认升序。" << endl;
                cmd.sortOrder = SQLCommand::ASC;
            }
        }
        else {
            // 未指定 ASC/DESC，默认为 ASC
            cmd.sortOrder = SQLCommand::ASC;
        }
        cout << "调试: 解析 ORDER BY 成功。列: " << cmd.orderByColumn << ", 顺序: " << (cmd.sortOrder == SQLCommand::ASC ? "ASC" : "DESC") << endl;
        return true; // 解析成功
    }

    // 如果以 ORDER BY 开头但格式不匹配
    cerr << "错误: 无法解析 ORDER BY 子句，格式应为 'ORDER BY column [ASC|DESC]': " << orderByClause << endl;
    return false; // 格式不匹配
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
        cmd.type = SQLCommand::CREATE_TABLE;
        cmd.tableName = match[1];
        parse_table_definitions(match[2].str(), cmd); // 使用辅助函数解析字段定义
        return cmd;
    }

    // 2. 解析 CREATE USER user ( ... );
    std::regex createUserRegex(R"(^\s*CREATE\s+USER\s+(.*?)\s*;?$)", std::regex::icase);
    if (std::regex_match(sql, match, createUserRegex)) {
        cmd.type = SQLCommand::CREATE_USER;
        // 将 CREATE USER 关键字后的内容传递给辅助函数进行进一步解析
        parse_user_definitions(match[1].str(), cmd);
        return cmd; // 成功解析并填充 cmd 后返回
    }
    // 4. 解析 DROP TABLE table;
    // 模式: DROP TABLE <表名> ; (可选)
    regex dropTableRegex(R"(DROP\s+TABLE\s+(\w+)\s*;?)", regex::icase);
    if (regex_match(sql, match, dropTableRegex)) {
        cmd.type = SQLCommand::DROP;
        cmd.tableName = match[1];
        return cmd;
    }

    // 5. 解析 UPDATE table SET col1=val1, ... WHERE col = val;
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

    // 6. 解析 DELETE FROM table WHERE col = val;
    // 模式: DELETE FROM <表名> WHERE <条件子句> ; (可选)
    regex deleteRegex(R"(DELETE\s+FROM\s+(\w+)\s+(WHERE\s+.*);?)", regex::icase);
    //  表名(1)       WHERE子句(2)
    if (regex_match(sql, match, deleteRegex)) {
        cmd.type = SQLCommand::DELETE_NEW;
        cmd.tableName = match[1];
        if (!parse_where_clause(match[2].str(), cmd)) { // 解析 WHERE 部分
            cerr << "错误: 无法解析 DELETE 语句中的 WHERE 子句。" << endl;
            cmd.type = SQLCommand::UNKNOWN; // 如果 WHERE 解析失败，标记为未知命令
        }
        return cmd;
    }

    // 7. 解析 ALTER TABLE 命令
    // 基础模式: ALTER TABLE <表名> <操作> ; (可选)
    regex alterTableBaseRegex(R"(ALTER\s+TABLE\s+(\w+)\s+(.*);?)", regex::icase);
    //  表名(1)      操作部分(2)
    if (regex_match(sql, match, alterTableBaseRegex)) {
        cmd.type = SQLCommand::ALTER;
        cmd.tableName = match[1];
        string actionStr = trim(match[2].str()); // 获取操作部分的字符串并去除空白

        smatch actionMatch;

        // 7a. 解析 RENAME TO new_table_name
        // 修改：添加 \s*;?\s*$ 确保匹配到结尾，并处理可选分号
        regex renameTableRegex(R"(RENAME\s+TO\s+(\w+)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, renameTableRegex)) {
            cmd.alterAction = SQLCommand::RENAME_TABLE;
            cmd.newTableName = actionMatch[1]; // 捕获新表名
            cout << "调试: 解析 RENAME TABLE TO " << cmd.newTableName << " 成功。" << endl;
            return cmd;
        }

        // 7b. 解析 ADD [COLUMN] column_name type
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

        // 7c. 解析 DROP [COLUMN] column_name
        // 修改：添加 \s*;?\s*$
        regex dropColumnRegex(R"(DROP\s+(?:COLUMN\s+)?(\w+)\s*;?\s*$)", regex::icase);
        if (regex_match(actionStr, actionMatch, dropColumnRegex)) {
            cmd.alterAction = SQLCommand::DROP_COLUMN;
            cmd.columnName = actionMatch[1].str(); // 捕获要删除的列名
            cout << "调试: 解析 DROP COLUMN " << cmd.columnName << " 成功。" << endl;
            return cmd;
        }

        // 7d. 解析 MODIFY [COLUMN] column_name new_type
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

        // 7e. 解析 RENAME [COLUMN] old_name TO new_name
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
    // 8. 解析SELECT语句
    regex selectRegex(R"(SELECT\s+(DISTINCT\s+)?(.*?)\s+FROM\s+(\w+)\s*(WHERE\s+.*?)?(ORDER\s+BY\s+.*?)?\s*;?\s*$)", regex::icase);
    if (regex_match(sql, match, selectRegex)) {
        cmd.type = SQLCommand::SELECT;

        // 处理 DISTINCT
        cmd.distinct = match[1].matched; // 如果捕获组 1 匹配成功 (即存在 "DISTINCT ")

        // 解析列列表
        parse_select_list(match[2].str(), cmd); // match[2] 是列列表字符串

        // 获取 FROM 表名
        cmd.fromTable = match[3].str();
        cmd.tableName = cmd.fromTable; // 也存入通用 tableName 字段，方便某些情况

        // 解析 WHERE 子句 (如果存在)
        if (match[4].matched) {
            cmd.whereClauseStr = trim(match[4].str()); // 存储原始 WHERE 子句 (去除首尾空白)
            if (!parse_where_clause(cmd.whereClauseStr, cmd)) { // 调用更新后的解析函数
                // WHERE 子句存在但解析失败
                cerr << "错误: 解析 SELECT 语句中的 WHERE 子句失败。" << endl;
                cmd.type = SQLCommand::UNKNOWN; // 标记为错误
                return cmd;
            }
            // parse_where_clause 内部会设置 cmd.hasWhere 等标志
        }
        else {
            cmd.hasWhere = false; // 没有 WHERE 子句
        }

        // 解析 ORDER BY 子句 (如果存在)
        if (match[5].matched) {
            if (!parse_order_by_clause(match[5].str(), cmd)) { // 调用解析函数
                // ORDER BY 子句存在但解析失败
                cerr << "错误: 解析 SELECT 语句中的 ORDER BY 子句失败。" << endl;
                cmd.type = SQLCommand::UNKNOWN; // 标记为错误
                return cmd;
            }
            // parse_order_by_clause 内部会设置 cmd.orderByColumn 等
        }
        else {
            cmd.orderByColumn = ""; // 没有 ORDER BY 子句
        }

        cout << "调试: SELECT 语句解析成功。" << endl;
        return cmd; // SELECT 命令解析成功
    }


    // 如果没有任何模式匹配成功
    cerr << "错误: 无法识别的 SQL 命令: " << sql << endl;
    cmd.type = SQLCommand::UNKNOWN;
    return cmd;
}


