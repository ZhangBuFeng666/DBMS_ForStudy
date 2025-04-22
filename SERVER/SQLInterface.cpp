#include "SQLInterface.h"

// 确保命名空间被使用
using namespace std;
namespace fs = std::filesystem; // 文件系统命名空间别名

// === 内部辅助函数 (放在匿名命名空间中，限制作用域) ===
namespace {
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
            bool needs_quoting = (val.find(',') != string::npos || val.find('\'') != string::npos || (!val.empty() && (val.front() == ' ' || val.back() == ' ')));
            if (needs_quoting) {
                size_t pos = val.find('\'');
                while (pos != string::npos) { val.replace(pos, 1, "''"); pos = val.find('\'', pos + 2); }
                ss << '\'' << val << '\'';
            }
            else { ss << val; }
            if (i < values.size() - 1) { ss << ","; }
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
    case SQLCommand::CREATE:
        if (cmd.tableName.empty() || cmd.fieldDefinitionsWithType.empty()) { result_message = "错误: 无效的 CREATE TABLE 语句。"; success = false; }
        else { success = create_table(cmd.dbName, cmd.tableName, cmd.fieldDefinitionsWithType, cmd.constraints); result_message = success ? "表 '" + cmd.tableName + "' 创建成功。" : "错误: 创建表 '" + cmd.tableName + "' 失败。"; }
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
bool SQLInterface::create_user(const string& username, const string& password, const string& privilege) { /* ... 实现 ... */
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
        if (!newHeader) { cerr << "错误: 无法创建用户头文件 " << headerPath << endl; return false; }
        newHeader << "用户名 密码 权限\n";
        if (!newHeader.good()) { cerr << "错误: 写入用户头文件失败 " << headerPath << endl; newHeader.close(); return false; }
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
    outFile << username << " " << password << " " << privilege << "\n";
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
    const std::map<std::string, int>& constraints) {
    return fileManager.create_table(dbName, tableName, fieldsWithType, constraints);
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
    case SQLCommand::DROP_COLUMN: {
        string colToDrop = cmd.columnName;

        // 1. 查找要删除列的索引
        int colIndex = findColumnIndex(cmd.dbName, cmd.tableName, colToDrop, METADATA_DB_ROOT);
        if (colIndex == -1) {
            cerr << "错误: 列 '" << colToDrop << "' 在表 '" << cmd.tableName << "' 中不存在。" << endl;
            return false;
        }

        // 2. 检查是否为主键列
        // string tidPath = tableMetaDir + cmd.tableName + ".tid"; // tidPath 已在 switch 外定义
        if (fs::exists(tidPath)) {
            vector<string> tidLines = readLinesFromFile(tidPath);
            for (const string& line : tidLines) {
                stringstream ss_tid(line);
                string constraintType;
                int constraintIndex;
                if (ss_tid >> constraintType >> constraintIndex) {
                    if ((constraintType == "primary_key" || constraintType == "PRIMARY_KEY") && constraintIndex == colIndex) {
                        cerr << "错误: 无法删除列 '" << colToDrop << "'，因为它是主键。" << endl;
                        return false; // 阻止删除主键列
                    }
                }
            }
        } // 结束主键检查

        // 3. 修改元数据文件 (.tdf, .tic) - 不修改 .tid
        vector<string> tdfLines = readLinesFromFile(tdfPath);
        vector<string> ticLines = readLinesFromFile(ticPath);

        if (colIndex >= tdfLines.size() || colIndex >= ticLines.size()) {
            cerr << "错误: 元数据文件 (.tdf/.tic) 与列索引不一致。" << endl; return false;
        }

        tdfLines.erase(tdfLines.begin() + colIndex);
        ticLines.erase(ticLines.begin() + colIndex);

        if (!writeLinesToFile(tdfPath, tdfLines) || !writeLinesToFile(ticPath, ticLines)) {
            cerr << "错误: 写入更新后的元数据文件 (.tdf/.tic) 失败。" << endl; return false; // 回滚复杂
        }
        cout << "调试: .tdf 和 .tic 文件更新成功 (删除列)。" << endl;

        // 4. 修改数据文件 (.trd)
        if (fs::exists(tableDataPath) && tableHasData(cmd.tableName, COMMONDATA_ROOT)) {
            string tempTrdPath = tableDataPath + ".tmp";
            vector<string> lines = readLinesFromFile(tableDataPath);
            vector<string> newLines; newLines.reserve(lines.size());
            int expectedColsBeforeDrop = tdfLines.size() + 1; // 列数是删除 *后* 的元数据列数 + 1

            int lineNum = 0;
            for (const string& line : lines) {
                lineNum++; if (line.empty()) continue;
                vector<string> values = parseCsvRow(line);

                if (values.size() != expectedColsBeforeDrop) { // 检查列数是否为删除前的列数
                    cerr << "警告 (行 " << lineNum << "): 行数据列数 (" << values.size() << ") 与预期 (" << expectedColsBeforeDrop << ") 不符，跳过处理: " << line << endl;
                    continue;
                }

                if (colIndex < values.size()) { // 确保索引有效
                    values.erase(values.begin() + colIndex); // 删除值
                    newLines.push_back(joinToCsvRow(values)); // 重组行
                }
                else {
                    cerr << "警告 (行 " << lineNum << "): 列索引 " << colIndex << " 超出范围 (大小 " << values.size() << ")，保留原始行: " << line << endl;
                    newLines.push_back(line); // 保留有问题的数据？或跳过？
                }
            }

            if (!writeLinesToFile(tempTrdPath, newLines)) { cerr << "错误: 写入更新后的数据到临时文件失败。" << endl; fs::remove(tempTrdPath); return false; } // 回滚复杂
            error_code ec;
            fs::remove(tableDataPath, ec); if (ec && ec != errc::no_such_file_or_directory) { cerr << "错误: 删除旧数据文件失败: " << ec.message() << endl; fs::remove(tempTrdPath); return false; }
            fs::rename(tempTrdPath, tableDataPath, ec); if (ec) { cerr << "错误: 重命名临时数据文件失败: " << ec.message() << endl; return false; }
            cout << "调试: 数据文件 .trd 更新成功 (删除列)。" << endl;
        }
        else { cout << "调试: 数据文件不存在或为空，无需修改数据。" << endl; }

        cout << "列 '" << colToDrop << "' 已成功从表 '" << cmd.tableName << "' 中删除。" << endl;
        return true;
    } // 结束简化的 DROP_COLUMN

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

// --- 数据操作 (DML) (保持不变) ---
bool SQLInterface::insert_into_table(const string& dbName, const string& tableName, const vector<string>& values) { /* ... 实现 ... */
    string dataPath = COMMONDATA_ROOT + tableName + ".trd"; string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/"; string ticPath = metaDir + tableName + ".tic";
    if (!fs::exists(ticPath)) { cerr << "错误: 无法找到表 '" << tableName << "' 的类型定义文件 (.tic)。" << endl; return false; }
    vector<string> fieldTypes = parseFieldTypes(dbName, tableName); if (fieldTypes.empty()) { cerr << "错误: 未能从 .tic 文件加载字段类型。" << endl; return false; }
    if (values.size() != fieldTypes.size()) { cerr << "错误: 插入的值数量 (" << values.size() << ") 与表定义的字段数量 (" << fieldTypes.size() << ") 不匹配。" << endl; return false; }
    for (size_t i = 0; i < values.size(); ++i) { if (!validateValueType(fieldTypes[i], values[i])) { cerr << "错误: 第 " << (i + 1) << " 个值 '" << values[i] << "' 的类型不符合字段要求的类型 '" << fieldTypes[i] << "'。" << endl; return false; } }
    string rowData = joinToCsvRow(values);
    ofstream dataFile(dataPath, ios::app); if (!dataFile.is_open()) { cerr << "错误: 无法打开数据文件进行追加: " << dataPath << endl; return false; }
    dataFile << rowData << "\n"; bool success = dataFile.good(); dataFile.close();
    if (!success) { cerr << "错误: 写入数据到文件 " << dataPath << " 失败。" << endl; } return success;
}
bool SQLInterface::update_table_row(const string& dbName, const string& tableName,
    const vector<pair<string, string>>& setClauses,
    const string& whereColumn, const string& whereValue) { /* ... 实现 ... */
    string dataPath = COMMONDATA_ROOT + tableName + ".trd"; string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/"; string tdfPath = metaDir + tableName + ".tdf"; string ticPath = metaDir + tableName + ".tic";
    if (setClauses.empty()) { cerr << "错误: UPDATE 语句必须包含至少一个 SET 子句。" << endl; return false; } if (whereColumn.empty()) { cerr << "错误: UPDATE 语句当前需要一个 'WHERE 字段 = 值' 子句。" << endl; return false; } if (!fs::exists(dataPath) || !fs::exists(tdfPath) || !fs::exists(ticPath)) { cerr << "错误: 表 '" << tableName << "' 的数据或元数据文件 (.trd, .tdf, .tic) 未找到。" << endl; return false; }
    auto columns = readLinesFromFile(tdfPath); auto types = readLinesFromFile(ticPath); if (columns.empty() || columns.size() != types.size()) { cerr << "错误: 表 '" << tableName << "' 的元数据文件 (.tdf/.tic) 不一致或为空。" << endl; return false; }
    int whereIndex = findColumnIndex(dbName, tableName, whereColumn, METADATA_DB_ROOT); if (whereIndex == -1) { cerr << "错误: WHERE 子句中的列 '" << whereColumn << "' 在表 '" << tableName << "' 中未找到。" << endl; return false; }
    map<int, string> setIndexToValue;
    for (const auto& pair : setClauses) { int setIndex = findColumnIndex(dbName, tableName, pair.first, METADATA_DB_ROOT); if (setIndex == -1) { cerr << "错误: SET 子句中的列 '" << pair.first << "' 在表 '" << tableName << "' 中未找到。" << endl; return false; } if (!validateValueType(types[setIndex], pair.second)) { cerr << "错误: 为列 '" << pair.first << "' (类型 " << types[setIndex] << ") 提供的值 '" << pair.second << "' 类型无效。" << endl; return false; } setIndexToValue[setIndex] = pair.second; }
    string tempPath = dataPath + ".tmp"; vector<string> lines = readLinesFromFile(dataPath); vector<string> newLines; newLines.reserve(lines.size()); int updatedRows = 0; int lineNum = 0;
    for (const string& line : lines) { lineNum++; if (line.empty()) continue; vector<string> values = parseCsvRow(line); if (values.size() != columns.size()) { cerr << "警告 (行 " << lineNum << "): UPDATE 时行数据列数 (" << values.size() << ") 与表定义 (" << columns.size() << ") 不符，保留原始行: " << line << endl; newLines.push_back(line); continue; } bool match = false; if (whereIndex < values.size()) { if (values[whereIndex] == whereValue) { match = true; } } else { cerr << "警告 (行 " << lineNum << "): WHERE 列索引 " << whereIndex << " 超出范围，保留原始行。" << endl; newLines.push_back(line); continue; } if (match) { for (const auto& [index, newValue] : setIndexToValue) { if (index < values.size()) { values[index] = newValue; } } newLines.push_back(joinToCsvRow(values)); updatedRows++; } else { newLines.push_back(line); } }
    if (!writeLinesToFile(tempPath, newLines)) { cerr << "错误: 写入更新后的数据到临时文件失败。" << endl; fs::remove(tempPath); return false; } error_code ec; fs::remove(dataPath, ec); if (ec && ec != errc::no_such_file_or_directory) { cerr << "错误: 删除旧数据文件失败: " << ec.message() << endl; fs::remove(tempPath); return false; } fs::rename(tempPath, dataPath, ec); if (ec) { cerr << "错误: 重命名临时数据文件失败: " << ec.message() << endl; return false; }
    cout << "更新成功。共有 " << updatedRows << " 行受到影响。" << endl; return true;
}
bool SQLInterface::delete_table_row(const string& dbName, const string& tableName,
    const string& whereColumn, const string& whereValue) { /* ... 实现 ... */
    string dataPath = COMMONDATA_ROOT + tableName + ".trd"; string metaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/"; string tdfPath = metaDir + tableName + ".tdf";
    if (whereColumn.empty()) { cerr << "错误: DELETE 语句当前需要一个 'WHERE 字段 = 值' 子句。" << endl; return false; } if (!fs::exists(dataPath) || !fs::exists(tdfPath)) { cerr << "错误: 表 '" << tableName << "' 的数据或元数据文件 (.trd, .tdf) 未找到。" << endl; return false; }
    auto columns = readLinesFromFile(tdfPath); if (columns.empty()) { cerr << "错误: 无法读取表 '" << tableName << "' 的列定义 (.tdf)。" << endl; return false; }
    int whereIndex = findColumnIndex(dbName, tableName, whereColumn, METADATA_DB_ROOT); if (whereIndex == -1) { cerr << "错误: WHERE 子句中的列 '" << whereColumn << "' 在表 '" << tableName << "' 中未找到。" << endl; return false; }
    string tempPath = dataPath + ".tmp"; vector<string> lines = readLinesFromFile(dataPath); vector<string> newLines; newLines.reserve(lines.size()); int deletedRows = 0; int lineNum = 0;
    for (const string& line : lines) { lineNum++; if (line.empty()) continue; vector<string> values = parseCsvRow(line); if (values.size() != columns.size()) { cerr << "警告 (行 " << lineNum << "): DELETE 时行数据列数 (" << values.size() << ") 与表定义 (" << columns.size() << ") 不符，保留该行: " << line << endl; newLines.push_back(line); continue; } bool match = false; if (whereIndex < values.size()) { if (values[whereIndex] == whereValue) { match = true; } } else { cerr << "警告 (行 " << lineNum << "): WHERE 列索引 " << whereIndex << " 超出范围，保留该行。" << endl; newLines.push_back(line); continue; } if (match) { deletedRows++; } else { newLines.push_back(line); } }
    if (!writeLinesToFile(tempPath, newLines)) { cerr << "错误: 写入更新后的数据到临时文件失败。" << endl; fs::remove(tempPath); return false; } error_code ec; fs::remove(dataPath, ec); if (ec && ec != errc::no_such_file_or_directory) { cerr << "错误: 删除旧数据文件失败: " << ec.message() << endl; fs::remove(tempPath); return false; } fs::rename(tempPath, dataPath, ec); if (ec) { cerr << "错误: 重命名临时数据文件失败: " << ec.message() << endl; return false; }
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
bool SQLInterface::validateValueType(const string& type, const string& value) { /* ... 实现 ... */
    int charLen = -1; if (parseCharLength(type, charLen)) { return value.length() <= charLen; }
    else if (type == "INT") { try { size_t p = 0; stoi(value, &p); return p == value.length(); } catch (...) { return false; } }
    else if (type == "DATE") { static const regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)"); return regex_match(value, dateRegex); }
    else if (type == "BOOL") { string lowerVal = value; transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower); return lowerVal == "true" || lowerVal == "false" || lowerVal == "1" || lowerVal == "0"; }
    cerr << "警告: 未知的字段类型用于值校验: " << type << endl; return false;
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
        bool keepRow = true; // 默认保留该行
        if (cmd.hasWhere) {
            if (whereColIndex < 0 || whereColIndex >= rawValues.size()) {
                // 理论上不应发生
                cerr << "内部错误 (行 " << lineNum << "): WHERE 列索引 " << whereColIndex << " 无效。" << endl;
                keepRow = false; // 跳过此行
            }
            else {
                string& valueToCheck = rawValues[whereColIndex]; // 获取待检查的值
                if (cmd.useInClause) {
                    // 处理 WHERE ... IN (...)
                    bool foundInList = false;
                    // cout << "调试 (行 " << lineNum << "): 检查 '" << valueToCheck << "' 是否在 IN 列表中..." << endl;
                    for (const string& inVal : cmd.inValues) {
                        // **重要**: 当前是字符串比较。对于数字等需要类型转换比较！
                        if (valueToCheck == inVal) {
                            foundInList = true;
                            // cout << "调试 (行 " << lineNum << "): 匹配到 IN 值 '" << inVal << "'" << endl;
                            break;
                        }
                    }
                    if (!foundInList) {
                        keepRow = false; // 不在 IN 列表中，则不保留
                    }
                }
                else {
                    // 处理 WHERE ... = ...
                     // cout << "调试 (行 " << lineNum << "): 检查 '" << valueToCheck << "' 是否等于 '" << cmd.whereValue << "'" << endl;
                    // **重要**: 当前是字符串比较。
                    if (valueToCheck != cmd.whereValue) {
                        keepRow = false; // 不相等，则不保留
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
                projectedRow.push_back(rawRow[index]); // 从原始行中取出对应索引的值
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

