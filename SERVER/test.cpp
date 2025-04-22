#include "SQLInterface.h" // 包含 SelectResult 定义

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits>    // 用于 numeric_limits
#include <stdexcept> // 用于异常处理
#include <iomanip>   // 用于格式化输出 (setw)
#include <algorithm> // 用于 max

// 使用命名空间
using namespace std;

// --- (print_values, print_metadata 实现保持不变) ---
void print_values(const vector<string>& values) { /* ... */ }
void print_metadata(const string& dbName, const string& tableName, SQLInterface& db) { /* ... */ }

// --- 新增：打印 SELECT 结果的辅助函数 ---
void print_select_result(const SelectResult& result) {
    if (!result.success) {
        cerr << "查询失败: " << result.errorMessage << endl;
        return;
    }

    // 检查是否有有效的列头或数据行
    if (result.header.empty() && result.data.empty()) {
        cout << "查询成功，结果为空集。" << endl;
        return;
    }

    size_t num_columns = 0;
    if (!result.header.empty()) {
        num_columns = result.header.size();
    }
    else if (!result.data.empty()) {
        num_columns = result.data[0].size(); // 假设所有数据行列数一致
    }
    else {
        cout << "查询成功，但无列头也无数据。" << endl; // 理论上不应发生，除非 select 了空表且无列
        return;
    }

    if (num_columns == 0) {
        cout << "查询成功，但选择的列数为 0。" << endl;
        return;
    }


    // 1. 计算每列的最大宽度以便对齐
    vector<size_t> widths(num_columns);
    // 先用列头初始化宽度
    if (!result.header.empty()) {
        for (size_t i = 0; i < num_columns; ++i) {
            widths[i] = result.header[i].length();
        }
    }
    else {
        // 如果没有列头，给一个最小默认宽度
        fill(widths.begin(), widths.end(), 5); // 默认宽度 5
    }

    // 再用数据行更新宽度
    for (const auto& row : result.data) {
        // 确保数据行的列数与预期一致
        if (row.size() == num_columns) {
            for (size_t i = 0; i < num_columns; ++i) {
                widths[i] = max(widths[i], row[i].length());
            }
        }
        else {
            cerr << "警告: 发现数据行列数 (" << row.size() << ") 与列头 (" << num_columns << ") 不匹配，可能导致显示错乱。" << endl;
            // 可以尝试调整 widths 大小或跳过此行
        }
    }

    // 2. 打印顶部分隔线
    cout << "+";
    for (size_t w : widths) { cout << string(w + 2, '-') << "+"; } cout << endl; // +2 为了左右各留一个空格

    // 3. 打印表头 (如果有)
    if (!result.header.empty()) {
        cout << "|";
        for (size_t i = 0; i < num_columns; ++i) {
            // left 表示左对齐, setw 设置字段宽度
            cout << " " << left << setw(widths[i]) << result.header[i] << " |";
        }
        cout << endl;
        // 打印表头下的分隔线
        cout << "+";
        for (size_t w : widths) { cout << string(w + 2, '-') << "+"; } cout << endl;
    }

    // 4. 打印数据行
    if (result.data.empty()) {
        // 如果有表头但无数据，可以打印提示
        if (!result.header.empty()) {
            // cout << "(空集)" << endl;
        }
    }
    else {
        for (const auto& row : result.data) {
            // 只打印列数匹配的行
            if (row.size() == num_columns) {
                cout << "|";
                for (size_t i = 0; i < num_columns; ++i) {
                    cout << " " << left << setw(widths[i]) << row[i] << " |";
                }
                cout << endl;
            }
        }
        // 5. 打印底部分隔线
        cout << "+";
        for (size_t w : widths) { cout << string(w + 2, '-') << "+"; } cout << endl;
    }
    // 打印总行数
    cout << "(" << result.data.size() << " 行)" << endl;
}


// --- 核心 SQL 处理函数 ---
bool process_sql(const string& sql,
    SQLInterface& db,
    SQLParser& parser,
    const string& currentDbName)
{
    cout << "\n>> 正在处理 SQL: " << sql << endl;
    if (sql.empty()) return true;
    SQLCommand cmd = parser.parse(sql);
    cmd.dbName = currentDbName;

    bool success = false;

    try {
        switch (cmd.type) {
            // --- (Cases for CREATE, DROP, INSERT, UPDATE, DELETE, ALTER 保持不变) ---
        case SQLCommand::CREATE: {
            if (cmd.tableName.empty() || cmd.fieldDefinitionsWithType.empty()) { cerr << "错误: 无效的 CREATE TABLE 语句 (缺少表名或字段定义)。" << endl; success = false; }
            else { cout << "尝试在数据库 '" << cmd.dbName << "' 中创建表 '" << cmd.tableName << "'" << endl; /* ... (打印字段和约束) ... */ success = db.create_table(cmd.dbName, cmd.tableName, cmd.fieldDefinitionsWithType, cmd.constraints); if (success) { cout << "成功: 表 '" << cmd.tableName << "' 已创建。" << endl; print_metadata(cmd.dbName, cmd.tableName, db); } else { cerr << "失败: 无法创建表 '" << cmd.tableName << "' (可能已存在或发生错误)。" << endl; } }
            break;
        }
        case SQLCommand::DROP: {
            if (cmd.tableName.empty()) { cerr << "错误: 无效的 DROP TABLE 语句 (缺少表名)。" << endl; success = false; }
            else { cout << "尝试从数据库 '" << cmd.dbName << "' 中删除表 '" << cmd.tableName << "'" << endl; success = db.drop_table(cmd.dbName, cmd.tableName); if (success) { cout << "成功: 表 '" << cmd.tableName << "' 已删除。" << endl; } else { cerr << "失败: 无法删除表 '" << cmd.tableName << "' (可能不存在或发生错误)。" << endl; } }
            break;
        }
        case SQLCommand::INSERT: {
            if (cmd.tableName.empty() || cmd.values.empty()) { cerr << "错误: 无效的 INSERT 语句 (缺少表名或值)。" << endl; success = false; }
            else { cout << "尝试向表 '" << cmd.tableName << "' 插入数据" << endl; print_values(cmd.values); success = db.insert_into_table(cmd.dbName, cmd.tableName, cmd.values); if (success) { cout << "成功: 数据已插入。" << endl; } else { cerr << "失败: 无法插入数据 (请检查类型、约束和表是否存在)。" << endl; } }
            break;
        }
        case SQLCommand::UPDATE: {
            if (cmd.tableName.empty() || cmd.setClauses.empty() || !cmd.hasWhere) { cerr << "错误: 无效的 UPDATE 语句 (缺少表名、SET 或 WHERE 子句)。" << endl; success = false; } // 确保 hasWhere 为 true
            else { cout << "尝试更新表 '" << cmd.tableName << "' 中 WHERE " << cmd.whereColumn << (cmd.useInClause ? " IN (...)" : " = '" + cmd.whereValue + "'") << " 的行" << endl; /* ... (打印 SET 子句) ... */ success = db.update_table_row(cmd.dbName, cmd.tableName, cmd.setClauses, cmd.whereColumn, cmd.whereValue); }
            break;
        }
        case SQLCommand::DELETE_NEW: {
            if (cmd.tableName.empty() || !cmd.hasWhere) { cerr << "错误: 无效的 DELETE 语句 (缺少表名或 WHERE 子句)。" << endl; success = false; } // 确保 hasWhere 为 true
            else { cout << "尝试从表 '" << cmd.tableName << "' 中删除 WHERE " << cmd.whereColumn << (cmd.useInClause ? " IN (...)" : " = '" + cmd.whereValue + "'") << " 的行" << endl; success = db.delete_table_row(cmd.dbName, cmd.tableName, cmd.whereColumn, cmd.whereValue); }
            break;
        }
        case SQLCommand::ALTER: {
            if (cmd.tableName.empty() || cmd.alterAction == SQLCommand::INVALID_ALTER) { cerr << "错误: 无效的 ALTER TABLE 语句 (缺少表名或无效的操作)。" << endl; success = false; }
            else { cout << "尝试修改表 '" << cmd.tableName << "'" << endl; success = db.alter_table(cmd); }
            break;
        }

                              // --- 新增 SELECT 处理 Case ---
        case SQLCommand::SELECT:
            // 基本检查由解析器完成，这里直接调用执行
            if (cmd.fromTable.empty() || cmd.selectColumns.empty()) {
                // 理论上解析器会处理，但加一层保险
                cerr << "错误: 无效的 SELECT 语句 (缺少 FROM 或选择列)。" << endl;
                success = false;
            }
            else {
                cout << "尝试从表 '" << cmd.fromTable << "' 查询数据..." << endl;
                SelectResult result = db.select_from_table(cmd); // 执行查询
                print_select_result(result); // 打印结果表格
                success = result.success; // 获取执行状态
            }
            break; // 不要忘记 break!

        case SQLCommand::UNKNOWN:
        default:
            cerr << "错误: 无法解析 SQL 语句或命令类型未知。" << endl;
            success = false;
            break;
        }
    }
    catch (const runtime_error& e) { success = false; cerr << "运行时错误: " << e.what() << endl; }
    catch (const exception& e) { success = false; cerr << "发生异常: " << e.what() << endl; }
    catch (...) { success = false; cerr << "发生未知异常。" << endl; }

    cout << "<< 处理结束。" << (success ? " (成功)" : " (失败)") << endl;
    return success;
}

// --- (main 函数保持不变，但可以加入更多 SELECT 测试) ---
void test() {
    //SQLInterface db;
    //SQLParser parser;
    //string currentDbName = "myTestDB"; // 使用之前的数据库名

    //cout << "--- 微型数据库管理系统初始化 ---" << endl;
    //FileManager& fm = db.getFileManager(); // 获取文件管理器
    //// (确保目录存在...)
    //db.create_database(currentDbName); // 确保数据库目录存在
    //cout << "当前数据库设置为: " << currentDbName << endl;

    //// === 测试用例 ===
    //// --- (Setup: DROP, CREATE, INSERT as before) ---
    //process_sql("DROP TABLE students;", db, parser, currentDbName);
    //process_sql("DROP TABLE courses;", db, parser, currentDbName);
    //process_sql("CREATE TABLE students (sid INT PRIMARY KEY, sname CHAR(50), age INT, major CHAR(30));", db, parser, currentDbName);
    //process_sql("INSERT INTO students VALUES (101, '爱丽丝', 21, '计算机');", db, parser, currentDbName);
    //process_sql("INSERT INTO students VALUES (103, '查理', 21, '艺术');", db, parser, currentDbName); // 年龄 21
    //process_sql("INSERT INTO students VALUES (102, '鲍勃', 23, '土木');", db, parser, currentDbName);
    //process_sql("INSERT INTO students VALUES (104, '戴安娜', 19, '国关');", db, parser, currentDbName);
    //process_sql("INSERT INTO students VALUES (105, '爱丽丝', 22, '数学');", db, parser, currentDbName); // 另一个爱丽丝

    //// --- 测试 SELECT ---
    //cout << "\n--- 测试 SELECT ---" << endl;
    //process_sql("SELECT * FROM students;", db, parser, currentDbName);
    //process_sql("SELECT sid, sname FROM students;", db, parser, currentDbName);
    //process_sql("SELECT major, age, sid FROM students WHERE age = 21;", db, parser, currentDbName);
    //process_sql("SELECT * FROM students WHERE sname = '鲍勃';", db, parser, currentDbName);
    //process_sql("SELECT sname, major FROM students WHERE sid = 999;", db, parser, currentDbName); // 应返回空集
    //process_sql("SELECT sname, major FROM students WHERE sname IN ('爱丽丝', '戴安娜');", db, parser, currentDbName);
    //process_sql("SELECT sid FROM students WHERE sid IN (101, 103, 105, 109);", db, parser, currentDbName); // 包含不存在的 ID
    //process_sql("SELECT sid FROM students WHERE sname IN ('不存在');", db, parser, currentDbName); // IN 值不存在

    //// 测试 ORDER BY (仅 INT)
    //process_sql("SELECT sid, sname, age FROM students ORDER BY age;", db, parser, currentDbName); // 默认 ASC
    //process_sql("SELECT sid, sname, age FROM students ORDER BY age DESC;", db, parser, currentDbName);
    //process_sql("SELECT * FROM students ORDER BY sid DESC;", db, parser, currentDbName);
    //process_sql("SELECT * FROM students ORDER BY sname;", db, parser, currentDbName); // 尝试按非 INT 排序 (应失败或忽略)

    //// 测试 DISTINCT
    //process_sql("SELECT DISTINCT major FROM students;", db, parser, currentDbName);
    //process_sql("SELECT DISTINCT age FROM students ORDER BY age;", db, parser, currentDbName); // DISTINCT + ORDER BY
    //process_sql("SELECT DISTINCT sname, age FROM students;", db, parser, currentDbName); // 多列 DISTINCT

    //// 测试不存在的表或列
    //process_sql("SELECT * FROM non_existent_table;", db, parser, currentDbName);
    //process_sql("SELECT non_existent_column FROM students;", db, parser, currentDbName);
    //process_sql("SELECT sid FROM students WHERE non_existent_column = 1;", db, parser, currentDbName);
    //process_sql("SELECT sid FROM students ORDER BY non_existent_column;", db, parser, currentDbName);

    //// --- (交互式输入循环) ---
    //cout << "\n--- 进入交互模式 (输入空行或 EOF 退出) ---" << endl;
    //string line;
    //cout << currentDbName << "> ";
    //while (getline(cin, line) && !line.empty()) {
    //    process_sql(line, db, parser, currentDbName);
    //    cout << currentDbName << "> ";
    //}

    //cout << "\n--- 微型数据库管理系统关闭 ---" << endl;
    //return 0;
}

// 注意: FileManager.h 和 FileManager.cpp 保持不变，无需重新生成。