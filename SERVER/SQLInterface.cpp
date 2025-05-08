#include "SQLInterface.h"


#include <fstream>
#include <iostream>
#include <filesystem> // C++17 文件系统库，用于文件/目录操作
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <stdexcept> // 用于抛出异常
#include <algorithm> // 用于 std::remove, std::transform
#include <set>       // 用于 std::set (实现 distinct 的另一种方式，这里没用)
#include <map>       // 用于列名到索引的映射


// 确保命名空间被使用
using namespace std;
namespace fs = std::filesystem; // 文件系统命名空间别名



// === 内部辅助函数 (放在匿名命名空间中，限制作用域) ===
namespace {
    std::string strip_single_quotes(const std::string& s) {
        // 检查字符串长度是否至少为2 (一个开头引号，一个结尾引号)
        if (s.length() >= 2) {
            // 检查第一个字符是否为单引号，最后一个字符是否为单引号
            if (s.front() == '\'' && s.back() == '\'') {
                // 提取并返回中间的子字符串
                return s.substr(1, s.length() - 2);
            }
        }
        // 如果不满足上述条件，则原样返回字符串
        return s;
    }


    // --- 文件读写辅助 ---
    vector<string> readLinesFromFile(const string& filepath) {
        vector<string> lines;
        ifstream file(filepath);
        string line;
        if (file.is_open()) {
            while (getline(file, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                lines.push_back(line);
            }
            file.close();
        }
        else {
            cerr << "错误: 无法打开文件进行读取: " << filepath << endl;
        }
        return lines;
    }

    // 新增：辅助函数，从 .tid 文件加载所有列的约束
    std::map<int, LoadedColumnConstraints> load_column_constraints_from_tid(const std::string& dbName, const std::string& tableName, const std::string& metadata_db_root) {
        std::map<int, LoadedColumnConstraints> all_column_constraints;
        std::string tidPath = metadata_db_root + dbName + "/" + tableName + "/" + tableName + ".tid";

        if (!fs::exists(tidPath)) {
            // std::cerr << "调试: " << tidPath << " 文件不存在，无约束加载。" << std::endl;
            return all_column_constraints; // 没有 .tid 文件，代表没有这些约束
        }

        std::vector<std::string> lines = readLinesFromFile(tidPath); // 使用你已有的 readLinesFromFile
        for (const std::string& line : lines) {
            std::string trimmed_line = trim(line);
            if (trimmed_line.empty()) continue;

            std::stringstream ss(trimmed_line);
            std::string constraint_type;
            int column_index;

            ss >> constraint_type;
            if (!(ss >> column_index)) {
                std::cerr << "警告: 解析 .tid 文件行失败: '" << trimmed_line << "'" << std::endl;
                continue;
            }

            if (constraint_type == "primary_key") {
                all_column_constraints[column_index].isPrimaryKey = true;
                all_column_constraints[column_index].isNotNull = true; // 主键隐含非空
                all_column_constraints[column_index].isUnique = true;  // 主键隐含唯一
            }
            else if (constraint_type == "not_null") {
                all_column_constraints[column_index].isNotNull = true;
            }
            else if (constraint_type == "unique") {
                all_column_constraints[column_index].isUnique = true;
            }
            // 可以扩展以支持其他约束类型
        }
        return all_column_constraints;
    }

    bool iequals(const string& a, const string& b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) { return tolower(a) == tolower(b); });
    }
    //5.2_________________________________________


    bool writeLinesToFile(const string& filepath, const vector<string>& lines) {
        ofstream file(filepath);
        if (!file.is_open()) {
            cerr << "错误: 无法打开文件进行写入: " << filepath << endl;
            return false;
        }
        for (const auto& line : lines) {
            file << line << '\n';
        }
        file.close();
        return file.good();
    }

    // --- 元数据访问辅助 ---
    int findColumnIndex(const string& dbName, const string& tableName, const string& columnName, const string& metadata_root) {
        string tdfPath = metadata_root + dbName + "/" + tableName + "/" + tableName + ".tdf";
        if (!fs::exists(tdfPath)) {
            return -1;
        }
        auto columns = readLinesFromFile(tdfPath);
        for (size_t i = 0; i < columns.size(); ++i) {
            if (trim(columns[i]) == columnName) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // --- CSV 数据行处理辅助 ---
    vector<string> parseCsvRow(const string& row) {
        vector<string> result;
        string field;
        bool in_quotes = false;
        string current_field;

        for (size_t i = 0; i < row.length(); ++i) {
            char c = row[i];
            if (c == '\'' && (i + 1 < row.length() && row[i + 1] == '\'')) {
                current_field += '\''; i++;
            }
            else if (c == '\'') {
                in_quotes = !in_quotes;
            }
            else if (c == ',' && !in_quotes) {
                result.push_back(trim(current_field)); current_field = "";
            }
            else {
                current_field += c;
            }
        }
        result.push_back(trim(current_field));
        return result;
    }

    string joinToCsvRow(const vector<string>& values) {
        stringstream ss;
        for (size_t i = 0; i < values.size(); ++i) {
            string val = values[i];

            // NULL 值处理（空字符串）
            if (val.empty()) {
                if (i < values.size() - 1) ss << ",";
                continue;
            }

            bool needsQuoting = (val.find(',') != string::npos ||
                val.find('"') != string::npos ||
                val.find('\'') != string::npos ||
                (!val.empty() && (val.front() == ' ' || val.back() == ' ')));

            if (needsQuoting) {
                // 转义单引号
                string escaped;
                for (char c : val) {
                    if (c == '\'') escaped += "''";
                    else escaped += c;
                }
                ss << "'" << escaped << "'";
            }
            else {
                ss << val;
            }

            if (i < values.size() - 1) ss << ",";
        }
        return ss.str();
    }

    // --- 其他辅助 ---
    bool tableHasData(const string& tableName, const string& common_root) {
        string trdPath = common_root + tableName + ".trd";
        error_code ec;
        uintmax_t size = fs::file_size(trdPath, ec);
        if (ec) { return (ec == errc::no_such_file_or_directory) ? false : (cerr << "错误: 检查文件大小时出错 " << trdPath << ": " << ec.message() << endl, false); }
        return size > 0;
    }

    bool parseCharLength(const string& typeStr, int& length) {
        static const regex charRegex(R"(CHAR\s*\((\d+)\))", regex::icase);
        smatch match;
        if (regex_match(typeStr, match, charRegex) && match.size() == 2) {
            try { length = stoi(match[1].str()); return true; }
            catch (...) { cerr << "错误: CHAR 类型长度解析失败: " << typeStr << endl; }
        }
        length = -1; return false;
    }

} // 结束匿名命名空间






// --- 新增：检查用户登录 ---
bool SQLInterface::check_login(const std::string& username, const std::string& password) {
    const string dataPath = METADATA_USER_ROOT + "user_data.txt";
    if (!fs::exists(dataPath)) {
        cerr << "错误: 用户数据文件不存在: " << dataPath << endl;
        return false; // 文件不存在无法登录
    }

    ifstream dataFile(dataPath);
    string line;
    if (dataFile.is_open()) {
        while (getline(dataFile, line)) {
            stringstream ss(line);
            string fileUser, filePass, filePriv; // 读取文件中的信息
            if (ss >> fileUser >> filePass) { // 至少要读到用户名和密码
                if (fileUser == username && filePass == password) {
                    dataFile.close();
                    cout << "调试: 用户 '" << username << "' 登录验证成功。" << endl;
                    return true; // 找到匹配的用户和密码
                }
            }
        }
        dataFile.close();
    }
    else {
        cerr << "错误: 无法打开用户数据文件: " << dataPath << endl;
        return false;
    }

    cerr << "调试: 用户 '" << username << "' 登录验证失败 (用户名或密码错误)。" << endl;
    return false; // 遍历完文件未找到匹配项
}

// --- 新增：处理 SQL 命令的核心方法 ---
// 返回值: true 表示命令被识别并尝试执行（可能执行失败），false 表示严重错误（如解析失败）
// result_message: 用于返回执行结果信息（成功、失败原因、影响行数等）
// select_result: 用于返回 SELECT 查询的结果
bool SQLInterface::process_sql_command(const std::string& sql, const std::string& currentDbName, /* out */ std::string& result_message, /* out */ SelectResult& select_result) {
    cout << "SQLInterface 正在处理 SQL: " << sql << endl;
    if (sql.empty()) {
        result_message = "错误: SQL 命令为空。";
        return false;
    }

    SQLCommand cmd = internal_parser.parse(sql); // 使用内部解析器
    cmd.dbName = currentDbName; // 设置数据库上下文

    select_result = SelectResult(); // 重置 select 结果
    bool success = false; // 操作是否成功

    switch (cmd.type) {
    //case SQLCommand::CREATE:
    //    if (cmd.tableName.empty() || cmd.fieldDefinitionsWithType.empty()) { result_message = "错误: 无效的 CREATE TABLE 语句。"; success = false; }
    //    else { success = create_table(cmd.dbName, cmd.tableName, cmd.fieldDefinitionsWithType, cmd.constraints); result_message = success ? "表 '" + cmd.tableName + "' 创建成功。" : "错误: 创建表 '" + cmd.tableName + "' 失败。"; }
    //    break;
    case SQLCommand::CREATE_TABLE: 
        if (cmd.tableName.empty() || cmd.fieldDefinitionsWithType.empty()) {
            cerr << "错误: 无效的 CREATE TABLE 语句 (缺少表名或字段定义)。" << endl;
            success = false;
        }
        else {
            cout << "尝试在数据库 '" << cmd.dbName << "' 中创建表 '" << cmd.tableName << "'" << endl;

            // 打印字段和解析出的约束信息 (可选，用于调试)
            cout << "  字段定义:" << endl;
            for (size_t i = 0; i < cmd.fieldDefinitionsWithType.size(); ++i) {
                cout << "    " << cmd.fieldDefinitionsWithType[i].first << " " << cmd.fieldDefinitionsWithType[i].second;
                if (cmd.columnConstraintsInfo.size() > i) { // 确保 columnConstraintsInfo 有对应条目
                    const auto& constr = cmd.columnConstraintsInfo[i];
                    if (constr.isPrimaryKey) cout << " PRIMARY KEY";
                    if (constr.isNotNull && !constr.isPrimaryKey) cout << " NOT NULL"; // PK 隐含 NN
                    if (constr.isUnique && !constr.isPrimaryKey) cout << " UNIQUE";   // PK 隐含 UQ
                }
                cout << endl;
            }

            // 调用 SQLInterface::create_table，传递所有需要的参数
            // 旧的 cmd.constraints 主要用于向后兼容或表级约束（如果支持）
            // 新的 cmd.columnConstraintsInfo 传递列级约束详情
            success = create_table(
                cmd.dbName,
                cmd.tableName,
                cmd.fieldDefinitionsWithType,
                cmd.constraints,             // 作为 tableLevelConstraints 传递
                cmd.columnConstraintsInfo    // 新增的参数，传递列的详细约束
            );

            if (success) {
                cout << "成功: 表 '" << cmd.tableName << "' 已创建。" << endl;
                // print_metadata(cmd.dbName, cmd.tableName, db); // 如果需要，取消注释
            }
            else {
                // 失败的原因可能有很多：表已存在、文件系统错误、或者更深层次的约束验证失败（如果FileManager做了更多检查）
                cerr << "失败: 无法创建表 '" << cmd.tableName << "' (可能已存在、定义无效或发生文件系统错误)。" << endl;
            }
        }
        break;
    case SQLCommand::CREATE_USER:
        if (!cmd.userID.empty() && !cmd.userPassword.empty()) {
            success = create_user(cmd.userID, cmd.userPassword, cmd.right);
            result_message = success ? "用户 '" + cmd.userID + "' 创建成功。" : "错误: 创建表 '" + cmd.userID + "' 失败。";
        }
        else {
            result_message = "错误: 无效的 CREATE USER 语句。"; success = false;
        }
        break;
    case SQLCommand::DROP:
        if (cmd.tableName.empty()) { result_message = "错误: 无效的 DROP TABLE 语句。"; success = false; }
        else { success = drop_table(cmd.dbName, cmd.tableName); result_message = success ? "表 '" + cmd.tableName + "' 删除成功。" : "错误: 删除表 '" + cmd.tableName + "' 失败。"; }
        break;
    case SQLCommand::INSERT:
        if (cmd.tableName.empty() || cmd.values.empty()) { result_message = "错误: 无效的 INSERT 语句。"; success = false; }
        else { success = insert_into_table(cmd.dbName, cmd.tableName, cmd.values); result_message = success ? "数据插入成功。" : "错误: 插入数据失败。"; }
        break;
    case SQLCommand::UPDATE:
        if (cmd.tableName.empty() || cmd.setClauses.empty() || !cmd.hasWhere) { result_message = "错误: 无效的 UPDATE 语句。"; success = false; }
        else { success = update_table_row(cmd.dbName, cmd.tableName, cmd.setClauses, cmd.whereColumn, cmd.whereValue); result_message = success ? "数据更新成功。" : "错误: 更新数据失败。"; /* TODO: 返回影响行数 */ }
        break;
    case SQLCommand::DELETE_NEW:
        if (cmd.tableName.empty() || !cmd.hasWhere) { result_message = "错误: 无效的 DELETE 语句。"; success = false; }
        else { success = delete_table_row(cmd.dbName, cmd.tableName, cmd.whereColumn, cmd.whereValue); result_message = success ? "数据删除成功。" : "错误: 删除数据失败。"; /* TODO: 返回影响行数 */ }
        break;
    case SQLCommand::ALTER:
        if (cmd.tableName.empty() || cmd.alterAction == SQLCommand::INVALID_ALTER) { result_message = "错误: 无效的 ALTER TABLE 语句。"; success = false; }
        else { success = alter_table(cmd); result_message = success ? "表修改成功。" : "错误: 修改表失败。"; }
        break;
    case SQLCommand::SELECT:
        if (cmd.fromTable.empty() || cmd.selectColumns.empty()) { result_message = "错误: 无效的 SELECT 语句。"; success = false; }
        else {
            select_result = select_from_table(cmd); // 调用 select 方法获取结果
            success = select_result.success; // select 方法内部会设置 success 标志
            if (success) {
                result_message = "查询成功，返回 " + std::to_string(select_result.data.size()) + " 行。";
                // select_result 包含了数据，调用者需要处理
            }
            else {
                result_message = select_result.errorMessage; // 使用 select 方法返回的错误信息
            }
        }
        break;
    case SQLCommand::UNKNOWN:
    default:
        result_message = "错误: 无法解析或不支持的 SQL 命令。";
        success = false;
        break;
    }
    return success; // 返回操作是否基本成功执行
}

// === SQLInterface 类的成员函数实现 ===

// --- 用户和数据库管理 (保持不变) ---
bool SQLInterface::create_user(const string& username, const string& password, const int right) { /* ... 实现 ... */
    // 确保用户目录存在
    if (!fileManager.create_directory(METADATA_USER_ROOT)) {
        cerr << "错误: 无法创建用户元数据目录 " << METADATA_USER_ROOT << endl;
        return false;
    }
    const string headerPath = METADATA_USER_ROOT + "user_header.txt";
    const string dataPath = METADATA_USER_ROOT + "user_data.txt";

    ifstream headerFileIn(headerPath);
    if (!headerFileIn.good()) {
        headerFileIn.close();
        ofstream newHeader(headerPath);
        if (!newHeader) { cerr << "错误: 无法创建用户头文件 " << headerPath << endl; return false; 
        }
        newHeader << "用户名 密码 权限\n";
        if (!newHeader.good()) {
            cerr << "错误: 写入用户头文件失败 " << headerPath << endl;
            newHeader.close(); return false;
        }
        newHeader.close();
    }
    else {
        headerFileIn.close();
    }

    ifstream dataFile(dataPath);
    string line;
    bool userExists = false;
    if (dataFile.is_open()) {
        while (getline(dataFile, line)) {
            stringstream iss(line); string existingUser;
            if (iss >> existingUser && existingUser == username) { userExists = true; break; }
        }
        dataFile.close();
    }

    if (userExists) { cerr << "信息: 用户 '" << username << "' 已存在。" << endl; return false; }

    ofstream outFile(dataPath, ios::app);
    if (!outFile) { cerr << "错误: 无法打开用户数据文件进行追加 " << dataPath << endl; return false; }
    switch (right) {
    case 0:
        outFile << username << " " << password << " " << "admin" << "\n";
        break;
    case 1:
        outFile << username << " " << password << " " << "base" << "\n";
        break;
    default:
        cerr << "错误: 授予" << username << "未知权限" << endl;
        outFile.close();
        return false;
    }
    bool success = outFile.good();
    outFile.close();
    if (!success) { cerr << "错误: 写入用户数据失败 " << dataPath << endl; }
    return success;
}
bool SQLInterface::create_database(const string& username) { /* ... 实现 ... */
    string dbMetaPath = METADATA_DB_ROOT + username + "/";
    if (fs::exists(dbMetaPath)) { cerr << "信息: 用户 '" << username << "' 的数据库目录已存在。" << endl; return true; }
    if (!fileManager.create_directory(dbMetaPath)) { cerr << "错误: 创建数据库元数据目录失败: " << dbMetaPath << endl; return false; }
    if (!fileManager.create_directory(COMMONDATA_ROOT)) { cerr << "错误: 创建通用数据目录失败: " << COMMONDATA_ROOT << endl; return false; }
    cout << "数据库目录 '" << dbMetaPath << "' 创建成功。" << endl; return true;
}

// --- 表结构操作 (DDL) ---
bool SQLInterface::create_table(const std::string& dbName, const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
    const std::map<std::string, int>& tableLevelConstraints, // 例如 "primary_key" -> index
    const std::vector<ColumnConstraintInfo>& columnConstraints) { // 新增参数
    // ...
    // 调用 FileManager::create_table 时传递新的 columnConstraints 参数
    return fileManager.create_table(dbName, tableName, fieldsWithType, tableLevelConstraints, columnConstraints);
}
bool SQLInterface::drop_table(const string& dbName, const string& tableName) {
    return fileManager.delete_table(dbName, tableName);
}

// 处理 ALTER TABLE 命令的核心逻辑
bool SQLInterface::alter_table(const SQLCommand& cmd) {
    if (cmd.type != SQLCommand::ALTER) { cerr << "内部错误: alter_table 函数被非 ALTER 命令调用。" << endl; return false; }

    string dbMetaDir = METADATA_DB_ROOT + cmd.dbName + "/";
    string tableMetaDir = dbMetaDir + cmd.tableName + "/";
    string tableDataPath = COMMONDATA_ROOT + cmd.tableName + ".trd";
    string tdfPath = tableMetaDir + cmd.tableName + ".tdf";
    string ticPath = tableMetaDir + cmd.tableName + ".tic";
    string tidPath = tableMetaDir + cmd.tableName + ".tid"; // 虽然不修改，检查主键时可能需要

    if (!fs::exists(tableMetaDir)) { cerr << "错误: 表 '" << cmd.tableName << "' 在数据库 '" << cmd.dbName << "' 中不存在。" << endl; return false; }

    switch (cmd.alterAction) {
    case SQLCommand::RENAME_TABLE: { /* ... 实现 (保持不变) ... */
        string newTableName = cmd.newTableName;
        string newTableMetaDir = dbMetaDir + newTableName + "/";
        string newTableDataPath = COMMONDATA_ROOT + newTableName + ".trd";
        if (fs::exists(newTableMetaDir) || fs::exists(newTableDataPath)) { cerr << "错误: 无法重命名表，目标表名 '" << newTableName << "' 已存在。" << endl; return false; }
        error_code ec_trd, ec_dir, ec_f;
        if (fs::exists(tableDataPath)) {
            fs::rename(tableDataPath, newTableDataPath, ec_trd);
            if (ec_trd) { cerr << "错误: 重命名数据文件 (.trd) 失败: " << ec_trd.message() << endl; return false; }
        }
        fs::rename(tableMetaDir, newTableMetaDir, ec_dir);
        if (ec_dir) {
            cerr << "错误: 重命名元数据目录失败: " << ec_dir.message() << endl;
            if (fs::exists(newTableDataPath)) { fs::rename(newTableDataPath, tableDataPath, ec_trd); } // 回滚 trd
            return false;
        }
        string oldTdfInNewDir = newTableMetaDir + cmd.tableName + ".tdf"; string newTdfInNewDir = newTableMetaDir + newTableName + ".tdf";
        string oldTicInNewDir = newTableMetaDir + cmd.tableName + ".tic"; string newTicInNewDir = newTableMetaDir + newTableName + ".tic";
        string oldTidInNewDir = newTableMetaDir + cmd.tableName + ".tid"; string newTidInNewDir = newTableMetaDir + newTableName + ".tid";
        if (fs::exists(oldTdfInNewDir)) { fs::rename(oldTdfInNewDir, newTdfInNewDir, ec_f); if (ec_f) cerr << "警告: 重命名 .tdf 文件失败: " << ec_f.message() << endl; }
        if (fs::exists(oldTicInNewDir)) { fs::rename(oldTicInNewDir, newTicInNewDir, ec_f); if (ec_f) cerr << "警告: 重命名 .tic 文件失败: " << ec_f.message() << endl; }
        if (fs::exists(oldTidInNewDir)) { fs::rename(oldTidInNewDir, newTidInNewDir, ec_f); if (ec_f) cerr << "警告: 重命名 .tid 文件失败: " << ec_f.message() << endl; }
        cout << "表 '" << cmd.tableName << "' 已成功重命名为 '" << newTableName << "'。" << endl; return true;
    }

    case SQLCommand::ADD_COLUMN: { /* ... 实现 (保持不变) ... */
        string colName, colType; stringstream ss_def(cmd.columnDefinition);
        if (!(ss_def >> colName >> colType)) { cerr << "错误: ADD COLUMN 的列定义格式无效: '" << cmd.columnDefinition << "'" << endl; return false; }
        colType = regex_replace(colType, regex(R"(\s+)"), "");
        if (!validateFieldType(colType)) { cerr << "错误: 指定的字段类型无效: " << colType << endl; return false; }
        if (findColumnIndex(cmd.dbName, cmd.tableName, colName, METADATA_DB_ROOT) != -1) { cerr << "错误: 列 '" << colName << "' 已存在于表 '" << cmd.tableName << "'。" << endl; return false; }
        ofstream tdfFile(tdfPath, ios::app); ofstream ticFile(ticPath, ios::app);
        if (!tdfFile.is_open() || !ticFile.is_open()) { cerr << "错误: 无法打开 .tdf 或 .tic 文件进行追加。" << endl; if (tdfFile.is_open()) tdfFile.close(); if (ticFile.is_open()) ticFile.close(); return false; }
        tdfFile << colName << '\n'; ticFile << colType << '\n'; tdfFile.close(); ticFile.close();
        if (!tdfFile.good() || !ticFile.good()) { cerr << "错误: 写入 .tdf 或 .tic 文件时发生错误。" << endl; return false; } // 回滚复杂
        if (fs::exists(tableDataPath) && tableHasData(cmd.tableName, COMMONDATA_ROOT)) {
            string tempTrdPath = tableDataPath + ".tmp"; vector<string> lines = readLinesFromFile(tableDataPath); vector<string> newLines; newLines.reserve(lines.size());
            string defaultValueStr;
            if (colType == "INT") defaultValueStr = "0"; else if (colType == "BOOL") defaultValueStr = "false"; else if (colType == "DATE") defaultValueStr = "'1970-01-01'"; else if (colType.rfind("CHAR", 0) == 0) defaultValueStr = "''"; else defaultValueStr = "''";
            for (const string& line : lines) { if (line.empty()) continue; vector<string> values = parseCsvRow(line); values.push_back(defaultValueStr); newLines.push_back(joinToCsvRow(values)); }
            if (!writeLinesToFile(tempTrdPath, newLines)) { cerr << "错误: 写入数据到临时文件失败 (" << tempTrdPath << ")" << endl; fs::remove(tempTrdPath); return false; } // 回滚复杂
            error_code ec; fs::remove(tableDataPath, ec); if (ec && ec != errc::no_such_file_or_directory) { cerr << "错误: 删除原始数据文件失败: " << ec.message() << endl; fs::remove(tempTrdPath); return false; } // 回滚复杂
            fs::rename(tempTrdPath, tableDataPath, ec); if (ec) { cerr << "错误: 重命名临时数据文件失败: " << ec.message() << endl; return false; } // 回滚复杂
        }
        else { ofstream touchTrd(tableDataPath); touchTrd.close(); }
        cout << "列 '" << colName << "' 已成功添加到表 '" << cmd.tableName << "'。" << endl; return true;
    }

                               // --- 修改后的 DROP_COLUMN ---
    // --- 修改后的 DROP_COLUMN --- （这是你之前代码中的版本）
    case SQLCommand::DROP_COLUMN: {
        std::string colToDrop = cmd.columnName;

        // 1. 查找要删除列的索引
        // 使用匿名空间内的 findColumnIndex，它读取 .tdf
        int colIndex = ::findColumnIndex(cmd.dbName, cmd.tableName, colToDrop, METADATA_DB_ROOT);
        if (colIndex == -1) {
            std::cerr << "错误: 列 '" << colToDrop << "' 在表 '" << cmd.tableName << "' 中不存在。" << std::endl;
            return false;
        }

        // 2. 检查是否为主键列 (从 .tid 文件读取)
        std::string tidPathEffective = tableMetaDir + cmd.tableName + ".tid"; // 确保路径正确
        bool is_primary_key_column = false;
        std::map<int, LoadedColumnConstraints> current_column_constraints = load_column_constraints_from_tid(cmd.dbName, cmd.tableName, METADATA_DB_ROOT);

        if (current_column_constraints.count(colIndex) && current_column_constraints[colIndex].isPrimaryKey) {
            is_primary_key_column = true;
        }

        if (is_primary_key_column) {
            std::cerr << "错误: 无法删除列 '" << colToDrop << "'，因为它是主键的一部分。" << std::endl;
            return false; // 阻止删除主键列
        }

        // 3. 修改元数据文件 (.tdf, .tic) - 同时需要更新 .tid 来移除与被删列相关的约束
        std::vector<std::string> tdfLines = readLinesFromFile(tdfPath);
        std::vector<std::string> ticLines = readLinesFromFile(ticPath);

        if (colIndex >= tdfLines.size() || colIndex >= ticLines.size()) {
            std::cerr << "错误: 元数据文件 (.tdf/.tic) 与列索引不一致。" << std::endl; return false;
        }

        tdfLines.erase(tdfLines.begin() + colIndex);
        ticLines.erase(ticLines.begin() + colIndex);

        if (!writeLinesToFile(tdfPath, tdfLines) || !writeLinesToFile(ticPath, ticLines)) {
            std::cerr << "错误: 写入更新后的元数据文件 (.tdf/.tic) 失败。" << std::endl; return false;
        }
        std::cout << "调试: .tdf 和 .tic 文件更新成功 (删除列)。" << std::endl;

        // NEW: 更新 .tid 文件
        // 需要移除所有与 colIndex 相关的约束，并调整其余约束的列索引（如果它们在 colIndex 之后）
        if (fs::exists(tidPathEffective)) {
            std::vector<std::string> tid_lines_original = readLinesFromFile(tidPathEffective);
            std::vector<std::string> tid_lines_new;
            bool tid_modified = false;

            for (const std::string& line : tid_lines_original) {
                std::string trimmed_line = trim(line);
                if (trimmed_line.empty()) continue;

                std::stringstream ss(trimmed_line);
                std::string constraint_type;
                int constraint_column_index;
                ss >> constraint_type;
                if (!(ss >> constraint_column_index)) {
                    tid_lines_new.push_back(line); //无法解析的行，原样保留？或警告？
                    continue;
                }

                if (constraint_column_index == colIndex) {
                    tid_modified = true; // 这个约束是针对被删除列的，跳过它
                    continue;
                }
                else if (constraint_column_index > colIndex) {
                    // 对于在被删除列之后的列的约束，其索引需要减1
                    tid_lines_new.push_back(constraint_type + " " + std::to_string(constraint_column_index - 1));
                    tid_modified = true;
                }
                else {
                    // 对于在被删除列之前的列的约束，索引不变
                    tid_lines_new.push_back(line);
                }
            }

            if (tid_modified) {
                if (!writeLinesToFile(tidPathEffective, tid_lines_new)) {
                    std::cerr << "错误: 写入更新后的约束文件 (.tid) 失败。" << std::endl;
                    // 这里应该有一个回滚机制，至少回滚.tdf和.tic的修改，但目前没有实现
                    return false;
                }
                std::cout << "调试: .tid 文件更新成功 (调整约束索引)。" << std::endl;
            }
        }


        // 4. 修改数据文件 (.trd) (这部分逻辑保持不变，从列中移除数据)
        if (fs::exists(tableDataPath) && tableHasData(cmd.tableName, COMMONDATA_ROOT)) {
            std::string tempTrdPath = tableDataPath + ".tmp";
            std::vector<std::string> lines_trd = readLinesFromFile(tableDataPath);
            std::vector<std::string> newLines_trd;
            newLines_trd.reserve(lines_trd.size());
            // 注意: tdfLines 此刻已经是删除列之后的列定义了
            int expectedColsAfterDrop = tdfLines.size();

            int lineNum = 0;
            for (const std::string& line_trd : lines_trd) {
                lineNum++; if (line_trd.empty()) continue;
                std::vector<std::string> values = parseCsvRow(line_trd);

                // 原始列数应该是 expectedColsAfterDrop + 1
                if (values.size() != (expectedColsAfterDrop + 1)) {
                    std::cerr << "警告 (行 " << lineNum << "): 行数据列数 (" << values.size()
                        << ") 与预期删除前列数 (" << (expectedColsAfterDrop + 1) << ") 不符，可能导致数据错位。跳过此行数据修改或原样保留。" << std::endl;
                    // 决定如何处理：是原样保留还是尝试按预期删除？原样保留更安全些避免数据损坏
                    newLines_trd.push_back(line_trd); // 原样保留格式不符的行
                    continue;
                }

                if (colIndex < values.size()) { // 确保索引有效
                    values.erase(values.begin() + colIndex);
                    newLines_trd.push_back(joinToCsvRow(values));
                }
                else {
                    // 理论上如果上面列数检查通过，这里不应该发生
                    std::cerr << "警告 (行 " << lineNum << "): 列索引 " << colIndex << " 超出范围 (大小 " << values.size() << ")，保留原始行: " << line_trd << std::endl;
                    newLines_trd.push_back(line_trd);
                }
            }

            if (!writeLinesToFile(tempTrdPath, newLines_trd)) { std::cerr << "错误: 写入更新后的数据到临时文件失败。" << std::endl; fs::remove(tempTrdPath); return false; }
            error_code ec_del_trd;
            fs::remove(tableDataPath, ec_del_trd);
            if (ec_del_trd && ec_del_trd != std::errc::no_such_file_or_directory) {
                std::cerr << "错误: 删除旧数据文件失败: " << ec_del_trd.message() << std::endl; fs::remove(tempTrdPath); return false;
            }
            error_code ec_rename_trd;
            fs::rename(tempTrdPath, tableDataPath, ec_rename_trd);
            if (ec_rename_trd) { std::cerr << "错误: 重命名临时数据文件失败: " << ec_rename_trd.message() << std::endl; return false; }
            std::cout << "调试: 数据文件 .trd 更新成功 (删除列数据)。" << std::endl;
        }
        else {
            std::cout << "调试: 数据文件不存在或为空，无需修改数据。" << std::endl;
        }

        std::cout << "列 '" << colToDrop << "' 已成功从表 '" << cmd.tableName << "' 中删除。" << std::endl;
        return true;
    } // 结束 DROP_COLUMN

    case SQLCommand::MODIFY_COLUMN: { /* ... 实现 (保持不变) ... */
        string colToModify = cmd.columnName; string newType = cmd.columnDefinition;
        int colIndex = findColumnIndex(cmd.dbName, cmd.tableName, colToModify, METADATA_DB_ROOT);
        if (colIndex == -1) { cerr << "错误: 要修改的列 '" << colToModify << "' 不存在。" << endl; return false; }
        if (!validateFieldType(newType)) { cerr << "错误: 指定的无效新字段类型: " << newType << endl; return false; }
        vector<string> ticLines = readLinesFromFile(ticPath);
        if (colIndex >= ticLines.size()) { cerr << "错误: 列索引超出 .tic 文件范围。" << endl; return false; }
        string oldType = ticLines[colIndex];
        if (tableHasData(cmd.tableName, COMMONDATA_ROOT)) {
            int oldLen = -1, newLen = -1; bool oldIsChar = parseCharLength(oldType, oldLen); bool newIsChar = parseCharLength(newType, newLen);
            if (!(oldIsChar && newIsChar && newLen > oldLen)) {
                cerr << "错误: 无法修改列 '" << colToModify << "' 的类型从 '" << oldType << "' 到 '" << newType << "'，因为表中有数据。" << "在非空表上，只允许增加 CHAR 类型的长度。" << endl; return false;
            } cout << "注意: 正在有数据的表上修改 CHAR 长度 (" << oldType << " -> " << newType << ")。" << endl;
        }
        else { cout << "调试: 表为空，允许类型修改 (" << oldType << " -> " << newType << ")。" << endl; }
        ticLines[colIndex] = newType;
        if (!writeLinesToFile(ticPath, ticLines)) { cerr << "错误: 写入更新后的类型文件 (.tic) 失败。" << endl; return false; } // 回滚复杂
        cout << "列 '" << colToModify << "' 的类型已成功修改为 '" << newType << "'。" << endl; return true;
    }

    case SQLCommand::RENAME_COLUMN: { /* ... 实现 (保持不变) ... */
        string oldName = cmd.columnName; string newName = cmd.newColumnName;
        int colIndex = findColumnIndex(cmd.dbName, cmd.tableName, oldName, METADATA_DB_ROOT);
        if (colIndex == -1) { cerr << "错误: 要重命名的列 '" << oldName << "' 不存在。" << endl; return false; }
        if (findColumnIndex(cmd.dbName, cmd.tableName, newName, METADATA_DB_ROOT) != -1) { cerr << "错误: 无法重命名列，目标名称 '" << newName << "' 已存在。" << endl; return false; }
        vector<string> tdfLines = readLinesFromFile(tdfPath);
        if (colIndex >= tdfLines.size()) { cerr << "错误: 列索引超出 .tdf 文件范围。" << endl; return false; }
        tdfLines[colIndex] = newName;
        if (!writeLinesToFile(tdfPath, tdfLines)) { cerr << "错误: 写入更新后的定义文件 (.tdf) 失败。" << endl; return false; } // 回滚复杂
        cout << "列 '" << oldName << "' 已成功重命名为 '" << newName << "'。" << endl; return true;
    }

    default:
        cerr << "错误: 不支持的 ALTER TABLE 操作类型。" << endl; 
        return false;
    } // 结束 switch(cmd.alterAction)
} // 结束 alter_table





// 修改 SQLInterface::insert_into_table 函数
bool SQLInterface::insert_into_table(const std::string& dbName, const std::string& tableName, const std::vector<std::string>& values) {
    std::string dataPath = COMMONDATA_ROOT + tableName + ".trd";
    std::string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/";
    std::string ticPath = metaDir + tableName + ".tic"; // 类型文件
    std::string tdfPath = metaDir + tableName + ".tdf"; // 列名文件

    if (!fs::exists(ticPath) || !fs::exists(tdfPath)) {
        std::cerr << "错误: 无法找到表 '" << tableName << "' 的类型或列定义文件 (.tic, .tdf)。" << std::endl;
        return false;
    }

    std::vector<std::string> fieldTypes = parseFieldTypes(dbName, tableName); // 你已有的函数
    std::vector<std::string> fieldNames = readLinesFromFile(tdfPath);      // 你已有的函数, 确保trim列名

    if (fieldTypes.empty() || fieldNames.empty()) {
        std::cerr << "错误: 未能从元数据文件加载字段类型或名称。" << std::endl;
        return false;
    }

    if (values.size() != fieldTypes.size()) {
        std::cerr << "错误: 插入的值数量 (" << values.size() << ") 与表定义的字段数量 (" << fieldTypes.size() << ") 不匹配。" << std::endl;
        return false;
    }

    // 1. 加载约束信息
    std::map<int, LoadedColumnConstraints> column_constraints = load_column_constraints_from_tid(dbName, tableName, METADATA_DB_ROOT);

    // 准备要插入的经过处理的值
    std::vector<std::string> processedValues;
    processedValues.reserve(values.size());

    for (size_t i = 0; i < values.size(); ++i) {
        std::string trimmedVal = trim(values[i]);
        // 系统的 NULL 表示方式：空字符串 "" 或者不区分大小写的 "NULL" 字符串
        bool isSystemNull = (trimmedVal.empty() || iequals(trimmedVal, "NULL"));

        std::string actualValueToStore = isSystemNull ? "" : values[i]; // 假设数据库内部用空串表示NULL

        // 2. 非空 (NOT NULL) 约束检查
        if (column_constraints.count(i) && column_constraints[i].isNotNull) {
            if (isSystemNull) {
                std::cerr << "错误: 列 '" << trim(fieldNames[i]) << "' 不允许为空 (NOT NULL constraint violated)。" << std::endl;
                return false;
            }
        }

        // 类型验证 (应该在约束检查之后，或者如果值为NULL则跳过严格类型验证)
        if (!isSystemNull && !validateValueType(fieldTypes[i], values[i])) { // 你已有的函数
            std::cerr << "错误: 第 " << (i + 1) << " 个值 '" << values[i]
                << "' 的类型不符合字段 '" << trim(fieldNames[i]) << "' 要求的类型 '" << fieldTypes[i] << "'。" << std::endl;
                return false;
        }
        processedValues.push_back(actualValueToStore);
    }

    // 3. 唯一 (UNIQUE) 和 主键 (PRIMARY KEY) 约束检查
    //    由于没有强制索引，我们需要读取整个数据文件进行检查。
    //    如果表很大，这将非常低效。
    std::vector<std::vector<std::string>> existing_data_rows;
    if (fs::exists(dataPath) && tableHasData(tableName, COMMONDATA_ROOT)) { // tableHasData 检查文件大小 > 0
        std::vector<std::string> data_lines = readLinesFromFile(dataPath);
        for (const std::string& line : data_lines) {
            if (line.empty()) continue;
            existing_data_rows.push_back(parseCsvRow(line)); // 使用你已有的 parseCsvRow
        }
    }

    for (size_t i = 0; i < processedValues.size(); ++i) {
        if (column_constraints.count(i) && (column_constraints[i].isUnique || column_constraints[i].isPrimaryKey)) {
            const std::string& value_to_check = processedValues[i];

            // 对于 UNIQUE 约束，SQL标准通常允许多个NULL。
            // 如果我们内部用空字符串代表NULL，那么多个空字符串也会违反唯一性（除非我们特殊处理）。
            // 这里，如果值是系统NULL (即空字符串)，我们跳过唯一性检查（符合SQL标准对NULL的处理）。
            // 但主键列不能是NULL（这个已由isNotNull检查覆盖了）。
            bool isSystemNullForThisValue = value_to_check.empty();
            if (column_constraints[i].isUnique && isSystemNullForThisValue) {
                continue; // 允许唯一的NULL值插入
            }

            for (const auto& row : existing_data_rows) {
                if (i < row.size()) { // 确保列索引有效
                    if (row[i] == value_to_check) {
                        std::string constraint_type_str = column_constraints[i].isPrimaryKey ? "PRIMARY KEY" : "UNIQUE";
                        std::cerr << "错误: 列 '" << trim(fieldNames[i]) << "' 的值 '" << value_to_check
                            << "' 违反了 " << constraint_type_str << " 约束。" << std::endl;
                        return false;
                    }
                }
            }
        }
    }

    // 所有检查通过，执行插入
    std::string rowData = joinToCsvRow(processedValues); // 你已有的函数
    std::ofstream dataFile(dataPath, std::ios::app);
    if (!dataFile.is_open()) {
        std::cerr << "错误: 无法打开数据文件进行追加: " << dataPath << std::endl;
        return false;
    }

    dataFile << rowData << "\n";
    bool success = dataFile.good();
    dataFile.close();

    if (!success) {
        std::cerr << "错误: 写入数据到文件 " << dataPath << " 失败。" << std::endl;
    }
    else {
        std::cout << "插入成功: " << rowData << std::endl;
    }
    return success;
}

//// --- 数据操作 (DML) (保持不变) ---
//bool SQLInterface::insert_into_table(const string& dbName, const string& tableName, const vector<string>& values) {
//    string dataPath = COMMONDATA_ROOT + tableName + ".trd";
//    string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/";
//    string ticPath = metaDir + tableName + ".tic";
//
//    if (!fs::exists(ticPath)) {
//        cerr << "错误: 无法找到表 '" << tableName << "' 的类型定义文件 (.tic)。" << endl;
//        return false;
//    }
//
//    vector<string> fieldTypes = parseFieldTypes(dbName, tableName);
//    if (fieldTypes.empty()) {
//        cerr << "错误: 未能从 .tic 文件加载字段类型。" << endl;
//        return false;
//    }
//
//    if (values.size() != fieldTypes.size()) {
//        cerr << "错误: 插入的值数量 (" << values.size() << ") 与表定义的字段数量 (" << fieldTypes.size() << ") 不匹配。" << endl;
//        return false;
//    }
//
//    // 处理并验证值
//    vector<string> processedValues;
//    for (size_t i = 0; i < values.size(); ++i) {
//        string trimmedVal = trim(values[i]);
//        bool isNull = (trimmedVal.empty() || iequals(trimmedVal, "NULL"));
//
//        if (!isNull && !validateValueType(fieldTypes[i], values[i])) {
//            cerr << "错误: 第 " << (i + 1) << " 个值 '" << values[i]
//                << "' 的类型不符合字段要求的类型 '" << fieldTypes[i] << "'。" << endl;
//            return false;
//        }
//        processedValues.push_back(isNull ? "" : values[i]);
//    }
//
//    string rowData = joinToCsvRow(processedValues);
//    ofstream dataFile(dataPath, ios::app);
//    if (!dataFile.is_open()) {
//        cerr << "错误: 无法打开数据文件进行追加: " << dataPath << endl;
//        return false;
//    }
//
//    dataFile << rowData << "\n";
//    bool success = dataFile.good();
//    dataFile.close();
//
//    if (!success) {
//        cerr << "错误: 写入数据到文件 " << dataPath << " 失败。" << endl;
//    }
//    else {
//        cout << "插入成功: " << rowData << endl;
//    }
//    return success;
//}


// 在 SQLInterface.cpp 中

// (确保 load_column_constraints_from_tid, LoadedColumnConstraints, 
//  readLinesFromFile, parseCsvRow, joinToCsvRow, findColumnIndex, iequals, trim, tableHasData 等辅助函数已存在)

// 在 SQLInterface.cpp 中

// (确保所有相关的辅助函数如 load_column_constraints_from_tid, findColumnIndex,
//  readLinesFromFile, parseCsvRow, joinToCsvRow, trim, iequals, validateValueType,
//  tableHasData, LoadedColumnConstraints 结构体等已存在且功能正确)
// 并且 METADATA_DB_ROOT, COMMONDATA_ROOT, fs 命名空间等已正确设置

bool SQLInterface::update_table_row(const std::string& dbName, const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& setClauses,
    const std::string& whereColumn, const std::string& whereValue) {

    std::string dataPath = COMMONDATA_ROOT + tableName + ".trd";
    std::string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/";
    std::string tdfPath = metaDir + tableName + ".tdf";
    std::string ticPath = metaDir + tableName + ".tic";

    if (setClauses.empty()) {
        std::cerr << "错误: UPDATE 语句必须包含至少一个 SET 子句。" << std::endl;
        return false;
    }

    if (whereColumn.empty()) {
        std::cerr << "错误: UPDATE 语句需要一个 WHERE 子句。" << std::endl;
        return false;
    }

    if (!fs::exists(dataPath) || !fs::exists(tdfPath) || !fs::exists(ticPath)) {
        std::cerr << "错误: 表 '" << tableName << "' 的数据或元数据文件未找到。" << std::endl;
        return false;
    }

    std::vector<std::string> all_column_names = readLinesFromFile(tdfPath); // 使用你已有的 readLinesFromFile
    std::vector<std::string> all_column_types = readLinesFromFile(ticPath); // 假设类型与列名一一对应

    if (all_column_names.empty() || all_column_names.size() != all_column_types.size()) {
        std::cerr << "错误: 表元数据文件不一致或为空。" << std::endl;
        return false;
    }

    // 1. 加载约束信息
    std::map<int, LoadedColumnConstraints> column_constraints = load_column_constraints_from_tid(dbName, tableName, METADATA_DB_ROOT);
    // 调试
    std::cout << "DEBUG: Constraints for table " << tableName << ":" << std::endl;
    for (const auto& pair_constr : column_constraints) {
        std::cout << "  Col Index " << pair_constr.first << ": PK=" << pair_constr.second.isPrimaryKey
            << ", NN=" << pair_constr.second.isNotNull
            << ", UQ=" << pair_constr.second.isUnique << std::endl;
    }
    // 2. 解析 SET 子句，并进行类型和 NOT NULL 约束的初步检查
    std::map<int, std::string> set_column_index_to_new_value;
    for (const auto& set_pair : setClauses) {
        std::string col_to_set_name = trim(set_pair.first);
        // int col_to_set_index = findColumnIndex(dbName, tableName, col_to_set_name, METADATA_DB_ROOT); // 旧的 findColumnIndex
        // 改为从已加载的 all_column_names 中查找索引，避免重复文件IO
        int col_to_set_index = -1;
        for (size_t i = 0; i < all_column_names.size(); ++i) {
            if (trim(all_column_names[i]) == col_to_set_name) {
                col_to_set_index = i;
                break;
            }
        }

        if (col_to_set_index == -1) {
            std::cerr << "错误: SET 子句中的列 '" << col_to_set_name << "' 在表 '" << tableName << "' 中不存在。" << std::endl;
            return false;
        }
        std::string new_value_str = strip_single_quotes(set_pair.second); // 去掉单引号
        // 原始值，可能包含 'NULL' 字符串或就是空串
        std::string trimmed_new_val = trim(new_value_str);
        bool is_new_value_system_null = (trimmed_new_val.empty() || iequals(trimmed_new_val, "NULL"));

        // --- FIX 1 START: 统一 NOT NULL 对空字符串的处理 ---
        if (column_constraints.count(col_to_set_index) && column_constraints[col_to_set_index].isNotNull) {
            if (is_new_value_system_null) { // 如果系统认为它是NULL (包括空字符串)
                std::cerr << "错误: 更新列 '" << col_to_set_name << "' 失败，该列不允许为空 (NOT NULL constraint violated)。" << std::endl;
                return false;
            }
        }
        // --- FIX 1 END ---

        // 类型验证 (如果不是系统NULL)
        if (!is_new_value_system_null && !validateValueType(all_column_types[col_to_set_index], new_value_str)) { // 使用原始 new_value_str 进行类型验证
            std::cerr << "错误: 为列 '" << col_to_set_name << "' 提供的新值 '" << new_value_str
                << "' 类型不符合其要求的类型 '" << all_column_types[col_to_set_index] << "'。" << std::endl;
            return false;
        }
        // 存储处理后的值，系统NULL统一存为空字符串
        set_column_index_to_new_value[col_to_set_index] = is_new_value_system_null ? "" : new_value_str;
    }

    // 3. 准备处理数据文件和WHERE条件
    // 解析 WHERE 条件和运算符 (这部分逻辑来自你提供的代码，保持不变)
    int where_col_idx = -1; // 重命名以避免与旧的 whereIndex 混淆
    for (size_t i = 0; i < all_column_names.size(); ++i) {
        if (trim(all_column_names[i]) == trim(whereColumn)) {
            where_col_idx = i;
            break;
        }
    }
    if (where_col_idx == -1) {
        std::cerr << "错误: WHERE 列 '" << whereColumn << "' 不存在。" << std::endl;
        return false;
    }
    std::string actualWhereCompareValue = whereValue;
    std::string whereOperator = "=";
    // (你提供的解析 whereOperator 和 actualWhereCompareValue 的逻辑...)
    if (whereValue.size() > 1 && (whereValue[0] == '>' || whereValue[0] == '<' || whereValue[0] == '=')) {
        if (whereValue.size() > 1 && whereValue[1] == '=') {
            whereOperator = whereValue.substr(0, 2); actualWhereCompareValue = trim(whereValue.substr(2));
        }
        else if (whereValue.size() > 1 && whereValue[0] == '<' && whereValue[1] == '>') {
            whereOperator = "<>"; actualWhereCompareValue = trim(whereValue.substr(2));
        }
        else {
            whereOperator = whereValue.substr(0, 1); actualWhereCompareValue = trim(whereValue.substr(1));
        }
    }
    else { // 如果没有操作符前缀，则 actualWhereCompareValue 就是 whereValue 本身
        actualWhereCompareValue = trim(whereValue);
    }


    if (whereOperator != "=" && whereOperator != "<>" && whereOperator != ">" &&
        whereOperator != ">=" && whereOperator != "<" && whereOperator != "<=") {
        std::cerr << "错误: 不支持的比较运算符 '" << whereOperator << "'" << std::endl;
        return false;
    }
    std::string where_col_type_str = all_column_types[where_col_idx];
    if ((whereOperator == ">" || whereOperator == ">=" || whereOperator == "<" || whereOperator == "<=") && (where_col_type_str == "INT")) { // 假设 isIntegerType 辅助函数存在

        std::cerr << "错误: 比较运算符 " << whereOperator << " 仅推荐用于 INT 类型列 (当前列类型: " << where_col_type_str << ")" << std::endl;
        // return false; // 根据严格程度决定是否报错退出
    }


    // 4. 阶段1: 识别将要被更新的行，并暂存它们更新后的状态
    std::vector<std::string> original_data_lines = readLinesFromFile(dataPath);
    std::vector<std::vector<std::string>> rows_if_updated_candidates;
    std::vector<int> original_line_indices_of_updated_rows;

    int line_num_debug = 0;
    for (size_t line_idx = 0; line_idx < original_data_lines.size(); ++line_idx) {
        const std::string& line = original_data_lines[line_idx];
        line_num_debug++;
        if (line.empty()) continue;

        std::vector<std::string> current_row_values = parseCsvRow(line);
        if (current_row_values.size() != all_column_names.size()) {
            std::cerr << "警告 (行 " << line_num_debug << "): 列数不匹配，跳过该行。" << std::endl;
            continue;
        }

        bool row_matches_where = false;
        if (where_col_idx < current_row_values.size()) {
            const std::string& value_to_check_in_db = current_row_values[where_col_idx];
            // --- 沿用你之前的 WHERE 匹配逻辑 ---
            bool db_value_is_system_null = (value_to_check_in_db.empty() || iequals(value_to_check_in_db, "NULL"));
            bool compare_val_is_system_null = (actualWhereCompareValue.empty() || iequals(actualWhereCompareValue, "NULL"));

            if (compare_val_is_system_null) { // WHERE X IS NULL or WHERE X IS NOT NULL (using = or <>)
                if (whereOperator == "=") row_matches_where = db_value_is_system_null;
                else if (whereOperator == "<>") row_matches_where = !db_value_is_system_null;
            }
            else if (!db_value_is_system_null) { // 数据库中的值不是NULL
                if (whereOperator == "=") row_matches_where = (value_to_check_in_db == actualWhereCompareValue);
                else if (whereOperator == "<>") row_matches_where = (value_to_check_in_db != actualWhereCompareValue);
                else if ((where_col_type_str == "INT")) {
                    try {
                        long long val_db = std::stoll(value_to_check_in_db);
                        long long val_comp = std::stoll(actualWhereCompareValue);
                        if (whereOperator == ">") row_matches_where = (val_db > val_comp);
                        else if (whereOperator == ">=") row_matches_where = (val_db >= val_comp);
                        else if (whereOperator == "<") row_matches_where = (val_db < val_comp);
                        else if (whereOperator == "<=") row_matches_where = (val_db <= val_comp);
                    }
                    catch (const std::exception&) { /* 转换失败，不匹配 */ }
                }
            }
        }

        if (row_matches_where) {
            std::vector<std::string> updated_row_candidate = current_row_values;
            for (const auto& pair_idx_val : set_column_index_to_new_value) {
                if (pair_idx_val.first < updated_row_candidate.size()) {
                    updated_row_candidate[pair_idx_val.first] = pair_idx_val.second;
                }
            }
            rows_if_updated_candidates.push_back(updated_row_candidate);
            original_line_indices_of_updated_rows.push_back(line_idx);
        }
    }

    // 5. 阶段2: 对所有数据（包括未更新的行和“更新后”的候选行）进行 UNIQUE 和 PRIMARY KEY 检查
    // --- FIX 2 START: 确保对所有相关表都能正确执行唯一性检查 ---
    if (!rows_if_updated_candidates.empty()) { // 只在有行实际要更新时才执行此重量级检查
        std::vector<std::vector<std::string>> all_data_for_uniqueness_check;
        all_data_for_uniqueness_check.reserve(original_data_lines.size());

        size_t next_updated_row_idx = 0;
        for (size_t line_idx = 0; line_idx < original_data_lines.size(); ++line_idx) {
            const std::string& current_line_content = original_data_lines[line_idx];
            if (current_line_content.empty() && line_idx < original_data_lines.size() - 1) { // 保留空行，除非是最后一行
                all_data_for_uniqueness_check.push_back({}); // 代表一个空行的数据
                continue;
            }
            if (current_line_content.empty() && line_idx == original_data_lines.size() - 1) continue;


            bool this_line_was_candidate_for_update = false;
            if (next_updated_row_idx < original_line_indices_of_updated_rows.size() &&
                original_line_indices_of_updated_rows[next_updated_row_idx] == line_idx) {
                this_line_was_candidate_for_update = true;
            }

            if (this_line_was_candidate_for_update) {
                all_data_for_uniqueness_check.push_back(rows_if_updated_candidates[next_updated_row_idx]);
                next_updated_row_idx++;
            }
            else {
                std::vector<std::string> parsed_original_row = parseCsvRow(current_line_content);
                if (parsed_original_row.size() == all_column_names.size()) { // 只添加列数正确的行
                    all_data_for_uniqueness_check.push_back(parsed_original_row);
                }
                else {
                    // 对于列数不匹配的原始行，它们不会被更新，所以对唯一性检查的影响较小
                    // 但如果它们本身就违反了唯一性，那是数据本身的问题，UPDATE不应负责修复
                    // 为了简化，这里可以不把格式错误的行加入 all_data_for_uniqueness_check
                    // 或者加入空向量表示，然后在下面检查时跳过空向量
                    all_data_for_uniqueness_check.push_back({}); // 标记为无效行数据
                }
            }
        }

        for (size_t col_idx_to_check = 0; col_idx_to_check < all_column_names.size(); ++col_idx_to_check) {
            if (column_constraints.count(col_idx_to_check) &&
                (column_constraints.at(col_idx_to_check).isUnique || column_constraints.at(col_idx_to_check).isPrimaryKey)) {

                std::map<std::string, int> value_counts;
                for (const auto& row_data_vec : all_data_for_uniqueness_check) {
                    if (row_data_vec.empty() || col_idx_to_check >= row_data_vec.size()) { // 跳过空行数据或列索引越界
                        continue;
                    }
                    const std::string& val_in_col = row_data_vec[col_idx_to_check];
                    // 系统NULL（空字符串）对于UNIQUE约束可以重复，但对于PRIMARY KEY不行（已被NOT NULL覆盖）
                    bool is_val_system_null_for_unique_check = val_in_col.empty();

                    if (column_constraints.at(col_idx_to_check).isUnique &&
                        !column_constraints.at(col_idx_to_check).isPrimaryKey && // 如果只是UNIQUE而不是PK
                        is_val_system_null_for_unique_check) {
                        continue;
                    }
                    // 对于主键，空字符串（即系统NULL）是不允许的，这个应该由NOT NULL检查阶段处理。
                    // 但为防万一，如果到了这里值仍是空串且是主键，value_counts会统计它。
                    value_counts[val_in_col]++;
                }

                for (const auto& pair_val_count : value_counts) {
                    if (pair_val_count.second > 1) {
                        std::string constraint_type_str = column_constraints.at(col_idx_to_check).isPrimaryKey ? "PRIMARY KEY" : "UNIQUE";
                        std::cerr << "错误: 更新操作会导致列 '" << trim(all_column_names[col_idx_to_check]) << "' 的值 '" << pair_val_count.first
                            << "' 重复，违反了 " << constraint_type_str << " 约束。" << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    // --- FIX 2 END ---


    // 6. 阶段3: 所有检查通过，实际构造新文件内容
    std::vector<std::string> new_file_lines;
    new_file_lines.reserve(original_data_lines.size());
    int updated_rows_count = 0;
    size_t current_updated_candidate_idx = 0;

    for (size_t line_idx = 0; line_idx < original_data_lines.size(); ++line_idx) {
        bool was_this_line_updated = false;
        if (current_updated_candidate_idx < original_line_indices_of_updated_rows.size() &&
            original_line_indices_of_updated_rows[current_updated_candidate_idx] == line_idx) {
            was_this_line_updated = true;
        }

        if (was_this_line_updated) {
            new_file_lines.push_back(joinToCsvRow(rows_if_updated_candidates[current_updated_candidate_idx]));
            updated_rows_count++;
            current_updated_candidate_idx++;
        }
        else {
            new_file_lines.push_back(original_data_lines[line_idx]); // 原样保留未匹配或格式错误的行
        }
    }

    // 7. 文件操作（写回）
    std::string tempPath = dataPath + ".tmp";
    if (!writeLinesToFile(tempPath, new_file_lines)) {
        std::cerr << "错误: 写入更新数据到临时文件失败。" << std::endl;
        fs::remove(tempPath);
        return false;
    }

    error_code ec;
    fs::remove(dataPath, ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
        std::cerr << "错误: 删除旧数据文件 '" << dataPath << "' 失败: " << ec.message() << std::endl;
        fs::remove(tempPath);
        return false;
    }

    fs::rename(tempPath, dataPath, ec);
    if (ec) {
        std::cerr << "错误: 重命名临时文件到 '" << dataPath << "' 失败: " << ec.message() << std::endl;
        return false;
    }

    std::cout << "更新成功。更新了 " << updated_rows_count << " 行。" << std::endl;
    return true;
}
//bool SQLInterface::update_table_row(const string& dbName, const string& tableName,
//    const vector<pair<string, string>>& setClauses,
//    const string& whereColumn, const string& whereValue) {
//
//    string dataPath = COMMONDATA_ROOT + tableName + ".trd";
//    string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/";
//    string tdfPath = metaDir + tableName + ".tdf";
//    string ticPath = metaDir + tableName + ".tic";
//
//    if (setClauses.empty()) {
//        cerr << "错误: UPDATE 语句必须包含至少一个 SET 子句。" << endl;
//        return false;
//    }
//
//    if (whereColumn.empty()) {
//        cerr << "错误: UPDATE 语句需要一个 WHERE 子句。" << endl;
//        return false;
//    }
//
//    if (!fs::exists(dataPath) || !fs::exists(tdfPath) || !fs::exists(ticPath)) {
//        cerr << "错误: 表 '" << tableName << "' 的数据或元数据文件未找到。" << endl;
//        return false;
//    }
//
//    auto columns = readLinesFromFile(tdfPath);
//    auto types = readLinesFromFile(ticPath);
//
//    if (columns.empty() || columns.size() != types.size()) {
//        cerr << "错误: 表元数据文件不一致或为空。" << endl;
//        return false;
//    }
//
//    int whereIndex = findColumnIndex(dbName, tableName, whereColumn, METADATA_DB_ROOT);
//    if (whereIndex == -1) {
//        cerr << "错误: WHERE 列 '" << whereColumn << "' 不存在。" << endl;
//        return false;
//    }
//
//    // 解析 WHERE 条件和运算符
//    string actualWhereValue = whereValue;
//    string whereOp = "="; // 默认运算符
//
//    // 检查 whereValue 是否包含运算符（如 ">20"）
//    if (whereValue.size() > 1 && (whereValue[0] == '>' || whereValue[0] == '<' || whereValue[0] == '=')) {
//        // 处理 >= 和 <=
//        if (whereValue.size() > 1 && whereValue[1] == '=') {
//            whereOp = whereValue.substr(0, 2);
//            actualWhereValue = whereValue.substr(2);
//        }
//        // 处理 <> 
//        else if (whereValue.size() > 1 && whereValue[0] == '<' && whereValue[1] == '>') {
//            whereOp = "<>";
//            actualWhereValue = whereValue.substr(2);
//        }
//        // 处理 > 或 <
//        else {
//            whereOp = whereValue.substr(0, 1);
//            actualWhereValue = whereValue.substr(1);
//        }
//    }
//
//    // 验证运算符是否合法
//    if (whereOp != "=" && whereOp != "<>" && whereOp != ">" &&
//        whereOp != ">=" && whereOp != "<" && whereOp != "<=") {
//        cerr << "错误: 不支持的比较运算符 '" << whereOp << "'" << endl;
//        return false;
//    }
//
//    // 验证 WHERE 列的类型是否支持该运算符
//    string whereColType = types[whereIndex];
//    if ((whereOp == ">" || whereOp == ">=" || whereOp == "<" || whereOp == "<=") &&
//        whereColType != "INT") {
//        cerr << "错误: 比较运算符 " << whereOp << " 只能用于 INT 类型列" << endl;
//        return false;
//    }
//
//    // SET 子句处理（保持不变）
//    map<int, string> setIndexToValue;
//    for (const auto& pair : setClauses) {
//        int setIndex = findColumnIndex(dbName, tableName, pair.first, METADATA_DB_ROOT);
//        if (setIndex == -1) {
//            cerr << "错误: SET 列 '" << pair.first << "' 不存在。" << endl;
//            return false;
//        }
//
//        string trimmedVal = trim(pair.second);
//        bool isNull = (trimmedVal.empty() || iequals(trimmedVal, "NULL"));
//
//        if (!isNull && !validateValueType(types[setIndex], pair.second)) {
//            cerr << "错误: 值 '" << pair.second << "' 类型无效。" << endl;
//            return false;
//        }
//        setIndexToValue[setIndex] = isNull ? "" : pair.second;
//    }
//
//    // 处理数据文件
//    string tempPath = dataPath + ".tmp";
//    vector<string> lines = readLinesFromFile(dataPath);
//    vector<string> newLines;
//    newLines.reserve(lines.size());
//    int updatedRows = 0;
//    int lineNum = 0;
//
//    for (const string& line : lines) {
//        lineNum++;
//        if (line.empty()) continue;
//
//        vector<string> values = parseCsvRow(line);
//        if (values.size() != columns.size()) {
//            cerr << "警告 (行 " << lineNum << "): 列数不匹配，保留原始行。" << endl;
//            newLines.push_back(line);
//            continue;
//        }
//
//        bool match = false;
//        if (whereIndex < values.size()) {
//            string& valueToCheck = values[whereIndex];
//            bool valueIsNull = (valueToCheck.empty() || valueToCheck == "NULL");
//            bool compareValueIsNull = (actualWhereValue.empty() || iequals(actualWhereValue, "NULL"));
//
//            // 处理 IS NULL/IS NOT NULL
//            if (compareValueIsNull) {
//                if (whereOp == "=") {
//                    match = valueIsNull;
//                }
//                else if (whereOp == "<>") {
//                    match = !valueIsNull;
//                }
//                // 其他运算符对 NULL 比较都返回 false
//            }
//            // 处理常规比较
//            else if (!valueIsNull) {
//                if (whereOp == "=") {
//                    match = (valueToCheck == actualWhereValue);
//                }
//                else if (whereOp == "<>") {
//                    match = (valueToCheck != actualWhereValue);
//                }
//                else if (whereColType == "INT") {
//                    try {
//                        int val = stoi(valueToCheck);
//                        int compareVal = stoi(actualWhereValue);
//
//                        if (whereOp == ">") match = (val > compareVal);
//                        else if (whereOp == ">=") match = (val >= compareVal);
//                        else if (whereOp == "<") match = (val < compareVal);
//                        else if (whereOp == "<=") match = (val <= compareVal);
//                    }
//                    catch (...) {
//                        cerr << "警告 (行 " << lineNum << "): 无法转换为 INT 进行比较" << endl;
//                    }
//                }
//            }
//        }
//
//        if (match) {
//            // 应用 SET 更新
//            for (const auto& [index, newValue] : setIndexToValue) {
//                if (index < values.size()) {
//                    values[index] = newValue;
//                }
//            }
//            newLines.push_back(joinToCsvRow(values));
//            updatedRows++;
//        }
//        else {
//            newLines.push_back(line);
//        }
//    }
//
//    // 文件操作（保持不变）
//    if (!writeLinesToFile(tempPath, newLines)) {
//        cerr << "错误: 写入临时文件失败。" << endl;
//        fs::remove(tempPath);
//        return false;
//    }
//
//    error_code ec;
//    fs::remove(dataPath, ec);
//    if (ec && ec != errc::no_such_file_or_directory) {
//        cerr << "错误: 删除旧文件失败: " << ec.message() << endl;
//        fs::remove(tempPath);
//        return false;
//    }
//
//    fs::rename(tempPath, dataPath, ec);
//    if (ec) {
//        cerr << "错误: 重命名失败: " << ec.message() << endl;
//        return false;
//    }
//
//    cout << "更新成功。更新了 " << updatedRows << " 行。" << endl;
//    return true;
//}
bool SQLInterface::delete_table_row(const string& dbName, const string& tableName,
    const string& whereColumn, const string& whereValue) { /* ... 实现 ... */
    string dataPath = COMMONDATA_ROOT + tableName + ".trd"; 
    string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/"; string tdfPath = metaDir + tableName + ".tdf";
    if (whereColumn.empty()) { cerr << "错误: DELETE 语句当前需要一个 'WHERE 字段 = 值' 子句。" << endl; return false; } 
    if (!fs::exists(dataPath) || !fs::exists(tdfPath)) { cerr << "错误: 表 '" << tableName << "' 的数据或元数据文件 (.trd, .tdf) 未找到。" << endl; return false; }
    auto columns = readLinesFromFile(tdfPath); if (columns.empty()) { cerr << "错误: 无法读取表 '" << tableName << "' 的列定义 (.tdf)。" << endl; return false; }
    int whereIndex = findColumnIndex(dbName, tableName, whereColumn, METADATA_DB_ROOT); 
    if (whereIndex == -1) { cerr << "错误: WHERE 子句中的列 '" << whereColumn << "' 在表 '" << tableName << "' 中未找到。" << endl; return false; }
    string tempPath = dataPath + ".tmp"; 
    vector<string> lines = readLinesFromFile(dataPath); vector<string> newLines; 
    newLines.reserve(lines.size()); 
    int deletedRows = 0; int lineNum = 0;
    for (const string& line : lines) { lineNum++; if (line.empty()) continue; vector<string> values = parseCsvRow(line); if (values.size() != columns.size()) { cerr << "警告 (行 " << lineNum << "): DELETE 时行数据列数 (" << values.size() << ") 与表定义 (" << columns.size() << ") 不符，保留该行: " << line << endl; newLines.push_back(line); continue; } bool match = false; if (whereIndex < values.size()) { if (values[whereIndex] == whereValue) { match = true; } } else { cerr << "警告 (行 " << lineNum << "): WHERE 列索引 " << whereIndex << " 超出范围，保留该行。" << endl; newLines.push_back(line); continue; } if (match) { deletedRows++; } else { newLines.push_back(line); } }
    if (!writeLinesToFile(tempPath, newLines)) { cerr << "错误: 写入更新后的数据到临时文件失败。" << endl; fs::remove(tempPath); return false; } error_code ec; 
    fs::remove(dataPath, ec); if (ec && ec != errc::no_such_file_or_directory) { cerr << "错误: 删除旧数据文件失败: " << ec.message() << endl; fs::remove(tempPath); return false; } 
    fs::rename(tempPath, dataPath, ec); if (ec) { cerr << "错误: 重命名临时数据文件失败: " << ec.message() << endl; return false; }
    cout << "删除成功。共有 " << deletedRows << " 行受到影响。" << endl; return true;
}

// --- 权限管理 (示例) (保持不变) ---
string SQLInterface::grant_privilege_sql(const string& username, const string& privilegeType) {
    return "GRANT " + privilegeType + " TO " + username + ";";
}

// --- 私有辅助函数实现 (保持不变) ---
bool SQLInterface::validateFieldType(const string& typeStr) { /* ... 实现 ... */
    static const regex typeRegex(R"(^(INT|BOOL|DATE|CHAR\(\s*[1-9]\d*\s*\))$)", regex::icase); return regex_match(typeStr, typeRegex);
}
vector<string> SQLInterface::parseFieldTypes(const string& dbName, const string& tableName) { /* ... 实现 ... */
    string ticPath = METADATA_DB_ROOT + dbName + "/" + tableName + "/" + tableName + ".tic"; if (!fs::exists(ticPath)) { cerr << "错误: 类型文件不存在: " << ticPath << endl; return {}; } vector<string> types = readLinesFromFile(ticPath); vector<string> validated_types;
    for (const string& t : types) { string trimmed_type = trim(t); if (!trimmed_type.empty()) { if (validateFieldType(trimmed_type)) { validated_types.push_back(trimmed_type); } else { cerr << "警告: 在 " << ticPath << " 中发现无效的类型定义: '" << t << "'" << endl; return {}; } } } return validated_types;
}

//这个也要改，5.2
bool SQLInterface::validateValueType(const string& type, const string& value) {
    // 首先检查是否为 NULL 值（空字符串或"NULL"字符串）
    string trimmedValue = trim(value);
    bool isNull = (trimmedValue.empty() || iequals(trimmedValue, "NULL"));
    if (isNull) {
        return true; // NULL 值允许用于任何类型字段
    }

    // 检查 CHAR/VARCHAR 类型
    int charLen = -1;
    if (parseCharLength(type, charLen)) {
        return value.length() <= charLen;
    }
    // 检查 INT 类型
    else if (type == "INT") {
        try {
            size_t p = 0;
            stoi(value, &p);
            return p == value.length(); // 确保整个字符串都是有效数字
        }
        catch (...) {
            return false;
        }
    }
    // 检查 DATE 类型
    else if (type == "DATE") {
        static const regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
        return regex_match(value, dateRegex);
    }
    // 检查 BOOL 类型
    else if (type == "BOOL") {
        string lowerVal = value;
        transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);
        return lowerVal == "true" || lowerVal == "false" || lowerVal == "1" || lowerVal == "0";
    }
    // 检查 FLOAT 类型（新增）
    else if (type == "FLOAT") {
        try {
            size_t p = 0;
            stof(value, &p);
            return p == value.length();
        }
        catch (...) {
            return false;
        }
    }

    cerr << "警告: 未知的字段类型 '" << type << "' 用于值校验，值: '" << value << "'" << endl;
    return false;
}


// --- 新增 SELECT 实现 ---
SelectResult SQLInterface::select_from_table(const SQLCommand& cmd) {
    SelectResult result; // 初始化结果对象
    if (cmd.type != SQLCommand::SELECT) {
        result.success = false;
        result.errorMessage = "内部错误: select_from_table 收到非 SELECT 命令。";
        return result;
    }
    if (cmd.fromTable.empty()) {
        result.success = false;
        result.errorMessage = "错误: SELECT 语句缺少 FROM 子句或表名无效。";
        return result;
    }
    if (cmd.selectColumns.empty()) {
        result.success = false;
        result.errorMessage = "错误: SELECT 语句缺少选择列或 '*'。";
        return result;
    }

    cout << "调试: 开始执行 SELECT 查询，目标表: '" << cmd.fromTable << "'" << endl;

    // --- 1. 检查文件和加载元数据 ---
    string metaDir = METADATA_DB_ROOT + cmd.dbName + "/" + cmd.fromTable + "/";
    string tdfPath = metaDir + cmd.fromTable + ".tdf"; // 列名文件
    string ticPath = metaDir + cmd.fromTable + ".tic"; // 类型文件
    string dataPath = COMMONDATA_ROOT + cmd.fromTable + ".trd"; // 数据文件

    // 检查元数据文件是否存在
    if (!fs::exists(tdfPath) || !fs::exists(ticPath)) {
        result.success = false;
        result.errorMessage = "错误: 找不到表 '" + cmd.fromTable + "' 的元数据文件 (.tdf 或 .tic)。";
        return result;
    }
    // 检查数据文件是否存在（不存在视为表为空，是正常情况）
    bool dataFileExists = fs::exists(dataPath);
    if (!dataFileExists) {
        cout << "信息: 数据文件 '" << dataPath << "' 不存在，表可能为空。" << endl;
    }

    // 读取所有列名和类型
    vector<string> allColumns = readLinesFromFile(tdfPath);
    vector<string> allTypes = readLinesFromFile(ticPath);
    if (allColumns.empty()) { // TDF 不应为空
        result.success = false;
        result.errorMessage = "错误: 表 '" + cmd.fromTable + "' 的列定义文件 (.tdf) 为空或读取失败。";
        return result;
    }
    if (allColumns.size() != allTypes.size()) { // 列名和类型数量必须一致
        result.success = false;
        result.errorMessage = "错误: 表 '" + cmd.fromTable + "' 的元数据文件 (.tdf 与 .tic) 列数不匹配。";
        return result;
    }

    // 创建列名到索引的映射 (使用 trim 后的列名)
    map<string, int> colNameToIndex;
    cout << "调试: 加载表 '" << cmd.fromTable << "' 的列定义: ";
    for (size_t i = 0; i < allColumns.size(); ++i) {
        string trimmedColName = trim(allColumns[i]);
        if (trimmedColName.empty()) {
            result.success = false;
            result.errorMessage = "错误: 表 '" + cmd.fromTable + "' 的 .tdf 文件中包含空或无效的列名。";
            return result;
        }
        colNameToIndex[trimmedColName] = static_cast<int>(i);
        cout << trimmedColName << "(" << trim(allTypes[i]) << ")" << (i == allColumns.size() - 1 ? "" : ", ");
    }
    cout << endl;

    // --- 2. 确定要选择的列和它们的索引 ---
    vector<string> selectedColumnsNames; // 最终选择的列名 (保持用户请求的顺序)
    vector<int> selectedColumnIndices;   // 对应列在原始表(allColumns)中的索引

    if (cmd.selectColumns.size() == 1 && cmd.selectColumns[0] == "*") {
        // 选择所有列
        cout << "调试: 选择所有列 (*)。" << endl;
        for (const string& col : allColumns) { // 使用从 TDF 读取的原始列名
            selectedColumnsNames.push_back(trim(col)); // 存储 trim 后的名字
        }
        for (size_t i = 0; i < allColumns.size(); ++i) {
            selectedColumnIndices.push_back(static_cast<int>(i));
        }
    }
    else {
        // 选择指定列
        string selectedColsStr; // 调试用
        set<string> alreadyAdded; // 用于检查重复选择
        for (const string& reqCol : cmd.selectColumns) {
            string trimmedReqCol = trim(reqCol);
            if (colNameToIndex.count(trimmedReqCol)) { // 检查列是否存在
                if (alreadyAdded.find(trimmedReqCol) == alreadyAdded.end()) { // 检查是否已添加
                    selectedColumnsNames.push_back(trimmedReqCol); // 存储请求的列名
                    selectedColumnIndices.push_back(colNameToIndex[trimmedReqCol]); // 存储对应的原始索引
                    alreadyAdded.insert(trimmedReqCol);
                    selectedColsStr += (selectedColsStr.empty() ? "" : ", ") + trimmedReqCol;
                }
                else {
                    cout << "警告: 列 '" << trimmedReqCol << "' 在 SELECT 列表中重复，将被忽略。" << endl;
                }
            }
            else { // 请求的列不存在
                result.success = false;
                result.errorMessage = "错误: 选择的列 '" + reqCol + "' 在表 '" + cmd.fromTable + "' 中不存在。";
                return result;
            }
        }
        cout << "调试: 选择指定列: " << selectedColsStr << endl;
    }

    // 设置结果的 header (总是设置，即使表为空)
    result.header = selectedColumnsNames;

    // 如果数据文件不存在，此时已设置好 header，可以直接返回空结果集
    if (!dataFileExists) {
        result.success = true;
        cout << "调试: 数据文件不存在，返回空结果集。" << endl;
        return result; // 返回成功和空数据
    }

    // --- 3. 读取数据并过滤 (WHERE) ---
    vector<vector<string>> filteredData; // 存储通过 WHERE 过滤后的 *完整* 行数据
    vector<string> lines = readLinesFromFile(dataPath);
    int whereColIndex = -1; // WHERE 条件列在原始表(allColumns)中的索引

    // 如果有 WHERE 子句，预先查找条件列索引
    if (cmd.hasWhere) {
        string whereColTrimmed = trim(cmd.whereColumn);
        if (!colNameToIndex.count(whereColTrimmed)) { // 使用 trim 后的列名查找
            result.success = false;
            result.errorMessage = "错误: WHERE 子句中的列 '" + cmd.whereColumn + "' 在表中不存在。";
            return result;
        }
        whereColIndex = colNameToIndex[whereColTrimmed];
        cout << "调试: WHERE 条件列 '" << whereColTrimmed << "' 索引为 " << whereColIndex << endl;
    }
    else {
        cout << "调试: 无 WHERE 子句，将处理所有行。" << endl;
    }

    cout << "调试: 开始读取和过滤数据行..." << endl;
    int lineNum = 0;
    for (const string& line : lines) {
        lineNum++;
        if (line.empty()) continue; // 跳过空行

        vector<string> rawValues = parseCsvRow(line); // 解析整行数据
        // 检查解析后的列数是否与元数据匹配
        if (rawValues.size() != allColumns.size()) {
            cerr << "警告 (行 " << lineNum << "): 数据行 '" << line << "' 的列数 (" << rawValues.size()
                << ") 与表定义 (" << allColumns.size() << ") 不符，已跳过。" << endl;
            continue; // 跳过格式错误的行
        }

        // 应用 WHERE 条件过滤
        // 应用 WHERE 条件过滤
        bool keepRow = true;
        if (cmd.hasWhere) {
            if (whereColIndex < 0 || whereColIndex >= rawValues.size()) {
                cerr << "内部错误 (行 " << lineNum << "): WHERE 列索引 " << whereColIndex << " 无效。" << endl;
                keepRow = false;
            }
            else {
                string& valueToCheck = rawValues[whereColIndex];
                bool valueIsNull = (valueToCheck.empty() || valueToCheck == "NULL");

                // 1. 处理 IS NULL/IS NOT NULL
                if (cmd.useIsNullClause) {
                    keepRow = valueIsNull ^ cmd.isNot; // XOR: IS NULL 时 valueIsNull 为真则保留
                    // IS NOT NULL 时 valueIsNull 为假则保留
                }
                // 2. 处理 IN 子句
                else if (cmd.useInClause) {
                    if (valueIsNull) {
                        keepRow = false; // NULL 不匹配任何 IN 列表
                    }
                    else {
                        keepRow = false;
                        for (const string& inVal : cmd.inValues) {
                            if (valueToCheck == inVal) {
                                keepRow = true;
                                break;
                            }
                        }
                    }
                }
                // 3. 处理常规比较 (=, <>)
                else {
                    bool compareValueIsNull = (cmd.whereValue.empty());

                    if (valueIsNull || compareValueIsNull) {
                        // NULL 的特殊比较逻辑
                        if (cmd.whereOperator == "=") {
                            keepRow = (valueIsNull && compareValueIsNull); // NULL = NULL 为假
                        }
                        else if (cmd.whereOperator == "<>") {
                            keepRow = !(valueIsNull && compareValueIsNull); // NULL <> NULL 为假
                        }
                        else if (cmd.whereOperator == ">") {
                            keepRow = false;
                        }
                        else if (cmd.whereOperator == ">=") {
                            keepRow = false;
                        }
                        else if (cmd.whereOperator == "<") {
                            keepRow = false;
                        }
                        else if (cmd.whereOperator == "<=") {
                            keepRow = false;
                        }
                    }
                    else {
                        // 常规值比较
                        if (cmd.whereOperator == "=") {
                            keepRow = (valueToCheck == cmd.whereValue);
                        }
                        else if (cmd.whereOperator == "<>") {
                            keepRow = (valueToCheck != cmd.whereValue);
                        }
                        else if (cmd.whereOperator == ">") {
                            keepRow = (valueToCheck > cmd.whereValue);
                        }
                        else if (cmd.whereOperator == ">=") {
                            keepRow = (valueToCheck >= cmd.whereValue);
                        }
                        else if (cmd.whereOperator == "<") {
                            keepRow = (valueToCheck < cmd.whereValue);
                        }
                        else if (cmd.whereOperator == "<=") {
                            keepRow = (valueToCheck <= cmd.whereValue);
                        }
                    }
                }
            }
        } // 结束 if (cmd.hasWhere)

        if (keepRow) {
            // cout << "调试 (行 " << lineNum << "): 行通过过滤，保留。" << endl;
            filteredData.push_back(rawValues); // 保留符合条件的 *完整* 原始行数据
        }
    } // 结束 for each line
    cout << "调试: 数据过滤完成，保留 " << filteredData.size() << " 行。" << endl;

    // --- 4. 投影 (根据 selectedColumnIndices 选择需要的列) ---
    vector<vector<string>> projectedData; // 存储最终结果数据 (只包含选择的列)
    projectedData.reserve(filteredData.size());
    cout << "调试: 开始投影选择的列..." << endl;
    for (const auto& rawRow : filteredData) {
        vector<string> projectedRow;
        projectedRow.reserve(selectedColumnIndices.size());
        for (int index : selectedColumnIndices) { // 遍历需要选择的列的原始索引
            if (index >= 0 && index < rawRow.size()) {
                // 修改开始：将空值显示为"NULL"
                if (rawRow[index].empty()) {
                    projectedRow.push_back("NULL");
                }
                else {
                    projectedRow.push_back(rawRow[index]);
                }
                // 修改结束
            }
            else {
                // 索引无效，这通常是内部逻辑错误
                cerr << "内部错误: 投影时列索引 " << index << " 无效。" << endl;
                projectedRow.push_back("投影错误"); // 添加错误标记，或抛出异常
            }
        }
        projectedData.push_back(projectedRow); // 添加投影后的行到结果集
    }
    cout << "调试: 投影完成。" << endl;


    // --- 5. 排序 (ORDER BY) ---
    if (!cmd.orderByColumn.empty()) {
        cout << "调试: 检测到 ORDER BY 子句，列: '" << cmd.orderByColumn << "', 顺序: " << (cmd.sortOrder == SQLCommand::ASC ? "ASC" : "DESC") << endl;
        int sortColIndexInProjected = -1; // 排序依据列在 *投影后* 结果集(projectedData)中的索引
        string sortColOriginalType = "UNKNOWN"; // 排序依据列的原始数据类型

        // 查找排序列在投影结果头(result.header)中的索引
        for (size_t i = 0; i < result.header.size(); ++i) {
            if (result.header[i] == cmd.orderByColumn) {
                sortColIndexInProjected = static_cast<int>(i);
                break;
            }
        }

        if (sortColIndexInProjected == -1) {
            // 这通常不应发生，因为解析阶段已检查过列名存在性
            result.success = false;
            result.errorMessage = "内部错误: ORDER BY 列 '" + cmd.orderByColumn + "' 在投影结果中未找到。";
            return result;
        }

        // 获取排序列的原始数据类型 (需要原始索引)
        if (colNameToIndex.count(cmd.orderByColumn)) {
            int originalIndex = colNameToIndex[cmd.orderByColumn];
            if (originalIndex >= 0 && originalIndex < allTypes.size()) {
                sortColOriginalType = trim(allTypes[originalIndex]);
                cout << "调试:排序列 '" << cmd.orderByColumn << "' 的原始类型为: " << sortColOriginalType << endl;
            }
        }

        // 检查排序类型是否为 INT (按你的要求)
        if (sortColOriginalType != "INT") {
            result.success = false; // 或者可以改为警告并忽略排序
            result.errorMessage = "错误: ORDER BY 目前仅支持对 INT 类型列排序，列 '" + cmd.orderByColumn + "' 类型为 " + sortColOriginalType + "。";
            cerr << result.errorMessage << endl; // 同时输出到 cerr
            // 选择忽略排序而不是返回错误
            cout << "警告: 将忽略 ORDER BY 子句。" << endl;
            // return result; // 如果要严格报错，取消注释这行
        }
        else { // 类型为 INT，执行排序
            cout << "调试: 按 INT 列索引 " << sortColIndexInProjected << " (" << cmd.orderByColumn << ") 排序..." << endl;
            try {
                std::sort(projectedData.begin(), projectedData.end(),
                    [&](const vector<string>& rowA, const vector<string>& rowB) {
                        // 安全检查索引
                        if (sortColIndexInProjected >= rowA.size() || sortColIndexInProjected >= rowB.size()) {
                            cerr << "内部警告: 排序比较时索引越界。" << endl;
                            return false; // 保持原有相对顺序
                        }

                        // 尝试将字符串转换为整数进行比较
                        int valA = 0, valB = 0;
                        bool convA_ok = false, convB_ok = false;
                        try { valA = stoi(trim(rowA[sortColIndexInProjected])); convA_ok = true; }
                        catch (...) {}
                        try { valB = stoi(trim(rowB[sortColIndexInProjected])); convB_ok = true; }
                        catch (...) {}

                        // 处理转换失败的情况: 无法转换的排在后面
                        if (!convA_ok && !convB_ok) return false; // 都失败，保持顺序
                        if (!convA_ok) return (cmd.sortOrder == SQLCommand::DESC); // A 失败，放后面 (升序时 false, 降序时 true)
                        if (!convB_ok) return (cmd.sortOrder == SQLCommand::ASC);  // B 失败，放后面 (升序时 true, 降序时 false)

                        // 都成功转换，正常比较
                        if (cmd.sortOrder == SQLCommand::ASC) {
                            return valA < valB; // 升序
                        }
                        else {
                            return valA > valB; // 降序
                        }
                    }); // 结束 std::sort lambda
                cout << "调试: 排序完成。" << endl;
            }
            catch (const std::exception& e) {
                // 捕获 sort 过程中可能出现的意外异常 (理论上 lambda 已处理)
                result.success = false;
                result.errorMessage = "错误: 排序过程中发生异常: " + string(e.what());
                cerr << result.errorMessage << endl;
                return result;
            }
        } // 结束 else (类型为 INT)

    } // 结束 if (!cmd.orderByColumn.empty())


    // --- 6. DISTINCT ---
    if (cmd.distinct) {
        cout << "调试: 应用 DISTINCT..." << endl;
        if (!projectedData.empty()) {
            // 为了使用 std::unique，数据需要先排序。
            // 如果用户指定了 ORDER BY，数据可能已经是部分有序的，但 unique 需要完全有序。
            // 为了确保正确性，我们总是对整个行向量进行排序（使用默认的 vector<string> 比较）
            std::sort(projectedData.begin(), projectedData.end());
            cout << "调试: 为 DISTINCT 临时排序完成。" << endl;

            // 使用 std::unique 将重复的相邻元素移动到容器末尾，并返回指向第一个重复元素的迭代器
            auto last = std::unique(projectedData.begin(), projectedData.end());

            // 使用 erase 删除从 last 到末尾的所有重复元素
            projectedData.erase(last, projectedData.end());
            cout << "调试: DISTINCT 处理后剩余 " << projectedData.size() << " 行。" << endl;
        }
    } // 结束 if (cmd.distinct)


    // --- 7. 设置最终结果 ---
    result.success = true;
    result.data = projectedData; // 将最终处理后的数据放入结果对象

    cout << "调试: SELECT 查询执行成功，返回 " << result.data.size() << " 行数据。" << endl;
    return result;
}

