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
        //case SQLCommand::CREATE_USER: {
        //    if (cmd.userID.empty() || cmd.userPassword.empty()) {
        //        cerr << "错误: 无效的 CREATE TABLE 语句 (缺少表名或字段定义)。" << endl; success = false; }
        //    else { cout << "尝试在数据库 '" << cmd.dbName << "' 中创建表 '" << cmd.tableName << "'" << endl;
        //     success = db.(cmd.dbName, cmd.tableName, cmd.fieldDefinitionsWithType, cmd.constraints); if (success) { cout << "成功: 表 '" << cmd.tableName << "' 已创建。" << endl; print_metadata(cmd.dbName, cmd.tableName, db); } else { cerr << "失败: 无法创建表 '" << cmd.tableName << "' (可能已存在或发生错误)。" << endl; } }

        case SQLCommand::CREATE_TABLE: {
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

                cout << cmd.columnConstraintsInfo.size();

                success = db.create_table(
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

void run_constraint_tests() {
    SQLInterface db;
    SQLParser parser;
    std::string currentDbName = "constraintTestDB"; // 使用新的数据库名以避免干扰

    std::cout << "\n\n--- 开始约束功能测试 ---" << std::endl;
    // 确保文件管理器和数据库目录存在
    // FileManager& fm = db.getFileManager(); // 如果需要直接操作
    db.create_database(currentDbName);
    std::cout << "当前数据库设置为: " << currentDbName << std::endl;

    // --- 清理旧表 (如果存在) ---
    process_sql("DROP TABLE products;", db, parser, currentDbName);
    process_sql("DROP TABLE employees;", db, parser, currentDbName);
    process_sql("DROP TABLE departments;", db, parser, currentDbName);

    // === 1. 测试 CREATE TABLE 与约束定义 ===
    std::cout << "\n--- 1. 测试 CREATE TABLE 与约束 ---" << std::endl;
    // 1.1 包含所有类型约束的表
    process_sql("CREATE TABLE products (pid INT PRIMARY KEY, pname CHAR(50) NOT NULL UNIQUE, price INT NOT NULL, category CHAR(30) UNIQUE);", db, parser, currentDbName);
    // 预期: 成功创建。SQLInterface 应该在内部为 pid, pname, category 生成 .tid 条目。
    //       pid: primary_key 0, not_null 0, unique 0
    //       pname: not_null 1, unique 1
    //       price: not_null 2
    //       category: unique 3
    // (可以通过 print_metadata 或手动检查 .tid 文件来验证)
    // print_metadata(currentDbName, "products", db); // 如果你有这个函数

    //// 1.2 测试
    process_sql("CREATE TABLE departments (dept_id INT PRIMARY KEY, dept_name CHAR(20));", db, parser, currentDbName);
    // 预期: SQLParser 层面报错 "Error: Multiple PRIMARY KEY constraints defined for table." 并设置命令为 UNKNOWN，导致 process_sql 失败。

    // 1.3 创建一个简单的表用于后续测试
    process_sql("CREATE TABLE employees (eid INT PRIMARY KEY, ename CHAR(50) NOT NULL, dept_id INT, salary INT UNIQUE);", db, parser, currentDbName);
    // 预期: 成功创建。
    //       eid: primary_key 0, not_null 0, unique 0
    //       ename: not_null 1
    //       salary: unique 3

    // === 2. 插入测试数据 ===
//    std::cout << "\n--- 2. 插入测试数据 ---" << std::endl;
    // 插入部门数
    process_sql("INSERT INTO departments VALUES (101, '研发部');", db, parser, currentDbName);
    process_sql("INSERT INTO departments VALUES (102, '市场部');", db, parser, currentDbName);
    process_sql("INSERT INTO departments VALUES (103, '人事部');", db, parser, currentDbName);
    // === 2. 测试 INSERT INTO 与约束 ===
    std::cout << "\n--- 2. 测试 INSERT INTO 与约束 (表: employees) ---" << std::endl;
    // 2.1 成功插入
    process_sql("INSERT INTO employees VALUES (1, 'Alice', 101, 50000);", db, parser, currentDbName); // 预期: 成功
    process_sql("INSERT INTO employees VALUES (2, 'Bob', 102, 60000);", db, parser, currentDbName);   // 预期: 成功
    process_sql("INSERT INTO employees VALUES (3, 'Carol', 101, NULL);", db, parser, currentDbName); // 预期: 成功 (salary UNIQUE允许NULL)
    process_sql("INSERT INTO employees VALUES (4, 'David', 103, NULL);", db, parser, currentDbName); // 预期: 成功 (salary UNIQUE允许另一个NULL)
    process_sql("SELECT * FROM employees;", db, parser, currentDbName);

    

    // 2.2 主键冲突
    process_sql("INSERT INTO employees VALUES (1, 'Eve', 104, 70000);", db, parser, currentDbName);
    // 预期: 失败 (错误: 列 'eid' 的值 '1' 违反了 PRIMARY KEY 约束。)

    // 2.3 非空约束冲突 (ename)
    process_sql("INSERT INTO employees VALUES (5, NULL, 105, 75000);", db, parser, currentDbName);
    // 预期: 失败 (错误: 列 'ename' 不允许为空 (NOT NULL constraint violated)。)
    process_sql("INSERT INTO employees VALUES (5, '', 105, 75000);", db, parser, currentDbName); // 假设 '' 也是 NULL
    // 预期: 失败 (错误: 列 'ename' 不允许为空 (NOT NULL constraint violated)。)

    // 2.4 唯一约束冲突 (salary) - 非NULL值
    process_sql("INSERT INTO employees VALUES (6, 'Frank', 106, 50000);", db, parser, currentDbName);
    // 预期: 失败 (错误: 列 'salary' 的值 '50000' 违反了 UNIQUE 约束。)

    process_sql("SELECT * FROM employees;", db, parser, currentDbName); // 查看当前数据

    // 3.1 内连接测试
    std::cout << "\n--- 3.1 内连接测试 ---" << std::endl;
    process_sql("SELECT * FROM employees join departments on employees.dept_id = departments.dept_id;", db, parser, currentDbName);
    process_sql("SELECT * FROM employees join departments on employees.dept_id = departments.dept_id where dept_id > 101;", db, parser, currentDbName);
    process_sql("SELECT * FROM employees join departments on employees.dept_id > departments.dept_id where dept_id > 101;", db, parser, currentDbName);
    //// === 3. 测试 UPDATE 与约束 ===
    //std::cout << "\n--- 3. 测试 UPDATE 与约束 (表: employees) ---" << std::endl;
    //// 3.1 成功更新 (不违反约束)
    //process_sql("UPDATE employees SET salary = 65000 WHERE eid = 2;", db, parser, currentDbName); // Bob's salary to 65000
    //// 预期: 成功
    //process_sql("UPDATE employees SET dept_id = 102 WHERE ename = 'Alice';", db, parser, currentDbName); // Alice to dept 102
    //// 预期: 成功
    //process_sql("SELECT * FROM employees;", db, parser, currentDbName);

    //// 3.2 更新导致主键冲突 (尝试将 Bob(eid=2) 的 eid 改为 Alice(eid=1) 的 eid)
    //process_sql("UPDATE employees SET eid = 1 WHERE eid = 2;", db, parser, currentDbName);
    //// 预期: 失败 (错误: 更新操作会导致列 'eid' 的值 '1' 重复，违反了 PRIMARY KEY 约束。)

    //// 3.3 更新导致违反非空约束 (尝试将 Alice(ename='Alice') 的 ename 改为 NULL)
    //process_sql("UPDATE employees SET ename = NULL WHERE eid = 1;", db, parser, currentDbName);
    //// 预期: 失败 (错误: 更新列 'ename' 失败，该列不允许为空 (NOT NULL constraint violated)。)
    //process_sql("UPDATE employees SET ename = '' WHERE eid = 1;", db, parser, currentDbName);
    //// 预期: 失败 (错误: 更新列 'ename' 失败，该列不允许为空 (NOT NULL constraint violated)。)

    //// 3.4 更新导致唯一约束冲突 (尝试将 Alice(eid=1, salary=50000) 的 salary 改为 Bob(eid=2, salary=65000) 的当前 salary)
    //// 先插入一个有确定唯一值的行
    //process_sql("INSERT INTO employees VALUES (7, 'Grace', 107, 80000);", db, parser, currentDbName); // Grace, salary 80000
    //process_sql("UPDATE employees SET salary = 80000 WHERE eid = 1;", db, parser, currentDbName); // Alice's salary to 80000 (Grace's salary)
    //// 预期: 失败 (错误: 更新操作会导致列 'salary' 的值 '80000' 重复，违反了 UNIQUE 约束。)

    //// 3.5 更新 UNIQUE 列为 NULL (应该允许，即使其他行该列也为NULL)
    //process_sql("UPDATE employees SET salary = NULL WHERE eid = 7;", db, parser, currentDbName); // Grace's salary to NULL
    //// 预期: 成功 (现在 Carol, David, Grace 的 salary 都是 NULL)
    //process_sql("SELECT * FROM employees;", db, parser, currentDbName);

    //// 3.6 更新主键列为一个新的唯一值 (应该成功)
    //process_sql("UPDATE employees SET eid = 10 WHERE eid = 1;", db, parser, currentDbName); // Alice's eid from 1 to 10
    //// 预期: 成功
    //process_sql("SELECT * FROM employees join departments on departments.dept_id = employees.dept_id WHERE eid = 10 ;", db, parser, currentDbName);


    //// === 4. 测试 ALTER TABLE DROP COLUMN 与约束 ===
    //std::cout << "\n--- 4. 测试 ALTER TABLE DROP COLUMN 与约束 (表: employees) ---" << std::endl;
    //// 4.1 尝试删除主键列
    //process_sql("ALTER TABLE employees DROP COLUMN eid;", db, parser, currentDbName);
    //// 预期: 失败 (错误: 无法删除列 'eid'，因为它是主键的一部分。)

    //// 4.2 尝试删除带 NOT NULL 约束的列 (非主键)
    //process_sql("ALTER TABLE employees DROP COLUMN ename;", db, parser, currentDbName);
    //// 预期: 成功 (因为我们没有阻止删除带NOT NULL的非主键列，但.tid中与ename的not_null约束应被移除)
    //// 验证：后续插入时，新表结构中不再有 ename，或者如果模拟了占位符，则该占位符可为NULL

    //// 4.3 尝试删除带 UNIQUE 约束的列 (非主键)
    //process_sql("ALTER TABLE employees DROP COLUMN salary;", db, parser, currentDbName);
    //// 预期: 成功 (.tid中与salary的unique约束应被移除)

    //// 查看 employees 表结构是否变化 (例如通过 print_metadata 或尝试查询已删除的列)
    //// print_metadata(currentDbName, "employees", db); // 如果存在
    //process_sql("SELECT dept_id FROM employees;", db, parser, currentDbName); // 应该只剩下 dept_id 和 eid(如果4.1失败)
    //// 重新创建 employees 用于后续测试（如果需要）或使用新表
    //process_sql("DROP TABLE employees;", db, parser, currentDbName);
    //process_sql("CREATE TABLE employees (eid INT PRIMARY KEY, ename CHAR(50) NOT NULL, dept_id INT, salary INT UNIQUE);", db, parser, currentDbName);
    //process_sql("INSERT INTO employees VALUES (1001, 'TestUser', 200, 10000);", db, parser, currentDbName);


    //// === 5. 更多边界情况和组合 ===
    //std::cout << "\n--- 5. 更多边界情况和组合 (表: products) ---" << std::endl;
    //// products (pid INT PRIMARY KEY, pname CHAR(50) NOT NULL UNIQUE, price INT NOT NULL, category CHAR(30) UNIQUE)
    //process_sql("INSERT INTO products VALUES (1, 'Laptop', 1200, 'Electronics');", db, parser, currentDbName); //成功
    //process_sql("INSERT INTO products VALUES (2, 'Mouse', 25, 'Electronics');", db, parser, currentDbName); //成功, category重复但允许
    //process_sql("INSERT INTO products VALUES (3, 'Keyboard', 75, NULL);", db, parser, currentDbName); //成功, category UNIQUE允许NULL

    //// 5.1 pname (NOT NULL UNIQUE)
    //process_sql("INSERT INTO products VALUES (4, NULL, 200, 'Books');", db, parser, currentDbName);
    //// 预期: 失败 (pname NOT NULL)
    //process_sql("INSERT INTO products VALUES (4, 'Laptop', 200, 'Books');", db, parser, currentDbName);
    //// 预期: 失败 (pname UNIQUE, 'Laptop' 已存在)
    //process_sql("INSERT INTO products VALUES (4, 'Desk', NULL, 'Furniture');", db, parser, currentDbName);
    //// 预期: 失败 (price NOT NULL)

    //// 5.2 更新使得 pname 冲突
    //process_sql("INSERT INTO products VALUES (5, 'Monitor', 300, 'Peripherals');", db, parser, currentDbName);
    //process_sql("UPDATE products SET pname = 'Mouse' WHERE pid = 5;", db, parser, currentDbName);
    //// 预期: 失败 (pname UNIQUE, 'Mouse' 已被 pid=2 使用)

    //// 5.3 更新使得 category 冲突 (非NULL时)
    //process_sql("INSERT INTO products VALUES (6, 'Webcam', 50, 'Cameras');", db, parser, currentDbName);
    //process_sql("UPDATE products SET category = 'Electronics' WHERE pid = 6;", db, parser, currentDbName); // 'Electronics'已被pid=1,2使用
    //// 预期: 失败 (category UNIQUE)

    //process_sql("SELECT * FROM products;", db, parser, currentDbName);

    //std::cout << "\n--- 约束功能测试结束 ---" << std::endl;
}

//void run_constraint_tests() {
//    SQLInterface db;
//    SQLParser parser;
//    std::string currentDbName = "constraintTestDB";
//
//    std::cout << "\n\n--- 开始约束功能测试 ---" << std::endl;
//    db.create_database(currentDbName);
//    std::cout << "当前数据库设置为: " << currentDbName << std::endl;
//
//    // 清理旧表
//    process_sql("DROP TABLE products;", db, parser, currentDbName);
//    process_sql("DROP TABLE employees;", db, parser, currentDbName);
//    process_sql("DROP TABLE departments;", db, parser, currentDbName);
//
//    // === 1. 创建测试表 ===
//    std::cout << "\n--- 1. 创建测试表 ---" << std::endl;
//    // 创建部门表
//    process_sql("CREATE TABLE departments (dept_id INT PRIMARY KEY, dept_name CHAR(20) NOT NULL, location CHAR(30));", db, parser, currentDbName);
//
//    // 创建员工表，包含外键关联
//    process_sql("CREATE TABLE employees (eid INT, ename CHAR(50) NOT NULL, dept_id INT, salary INT UNIQUE, "
//        "FOREIGN KEY (dept_id) REFERENCES departments(dept_id));", db, parser, currentDbName);
//
//    // === 2. 插入测试数据 ===
//    std::cout << "\n--- 2. 插入测试数据 ---" << std::endl;
//    // 插入部门数据
//    process_sql("INSERT INTO departments VALUES (101, '研发部', '北京');", db, parser, currentDbName);
//    process_sql("INSERT INTO departments VALUES (102, '市场部', '上海');", db, parser, currentDbName);
//    process_sql("INSERT INTO departments VALUES (103, '人事部', '广州');", db, parser, currentDbName);
//
//    // 插入员工数据
//    process_sql("INSERT INTO employees VALUES (1, 'Alice', 101, 50000);", db, parser, currentDbName);
//    process_sql("INSERT INTO employees VALUES (2, 'Bob', 102, 60000);", db, parser, currentDbName);
//    process_sql("INSERT INTO employees VALUES (3, 'Carol', 101, 55000);", db, parser, currentDbName);
//    process_sql("INSERT INTO employees VALUES (4, 'David', 103, 45000);", db, parser, currentDbName);
//    process_sql("INSERT INTO employees VALUES (5, 'Eve', NULL, 70000);", db, parser, currentDbName); // 无部门的员工
//
//    // === 3. 测试连接查询 ===
//    std::cout << "\n--- 3. 测试连接查询 ---" << std::endl;
//
//    // 3.1 内连接测试
//    std::cout << "\n--- 3.1 内连接测试 ---" << std::endl;
//    process_sql("SELECT * FROM employees join departments on employees.dept_id = departments.dept_id;", db, parser, currentDbName);

    //// 3.2 左外连接测试
    //std::cout << "\n--- 3.2 左外连接测试 ---" << std::endl;
    //process_sql("SELECT e.eid, e.ename, d.dept_name, d.location "
    //    "FROM employees e LEFT OUTER JOIN departments d ON e.dept_id = d.dept_id;",
    //    db, parser, currentDbName);

    //// 3.3 右外连接测试
    //std::cout << "\n--- 3.3 右外连接测试 ---" << std::endl;
    //process_sql("SELECT e.eid, e.ename, d.dept_name, d.location "
    //    "FROM employees e RIGHT OUTER JOIN departments d ON e.dept_id = d.dept_id;",
    //    db, parser, currentDbName);

    //// 3.4 全外连接测试
    //std::cout << "\n--- 3.4 全外连接测试 ---" << std::endl;
    //process_sql("SELECT e.eid, e.ename, d.dept_name, d.location "
    //    "FROM employees e FULL OUTER JOIN departments d ON e.dept_id = d.dept_id;",
    //    db, parser, currentDbName);

    //// 3.5 带条件的连接查询
    //std::cout << "\n--- 3.5 带条件的连接查询 ---" << std::endl;
    //process_sql("SELECT e.ename, e.salary, d.dept_name "
    //    "FROM employees e JOIN departments d ON e.dept_id = d.dept_id "
    //    "WHERE e.salary > 50000;",
    //    db, parser, currentDbName);

    //// 3.6 多表连接
    //std::cout << "\n--- 3.6 多表连接 ---" << std::endl;
    //// 先创建一个项目表
    //process_sql("CREATE TABLE projects (pid INT PRIMARY KEY, pname CHAR(50), dept_id INT, "
    //    "FOREIGN KEY (dept_id) REFERENCES departments(dept_id));", db, parser, currentDbName);
    //process_sql("INSERT INTO projects VALUES (1, '数据库系统', 101);", db, parser, currentDbName);
    //process_sql("INSERT INTO projects VALUES (2, '市场推广', 102);", db, parser, currentDbName);

    //process_sql("SELECT e.ename, d.dept_name, p.pname "
    //    "FROM employees e "
    //    "JOIN departments d ON e.dept_id = d.dept_id "
    //    "JOIN projects p ON d.dept_id = p.dept_id;",
    //    db, parser, currentDbName);

    //// === 4. 测试连接查询中的约束 ===
    //std::cout << "\n--- 4. 测试连接查询中的约束 ---" << std::endl;

    //// 4.1 测试连接查询中的主键约束
    //process_sql("SELECT d.dept_id, COUNT(e.eid) as employee_count "
    //    "FROM departments d LEFT JOIN employees e ON d.dept_id = e.dept_id "
    //    "GROUP BY d.dept_id;",
    //    db, parser, currentDbName);

    //// 4.2 测试连接查询中的唯一约束
    //process_sql("SELECT e1.ename as employee1, e2.ename as employee2, e1.salary "
    //    "FROM employees e1 JOIN employees e2 ON e1.salary = e2.salary AND e1.eid < e2.eid;",
    //    db, parser, currentDbName);

    //// 4.3 测试连接查询中的外键约束
    //process_sql("SELECT e.ename, d.dept_name "
    //    "FROM employees e JOIN departments d ON e.dept_id = d.dept_id "
    //    "WHERE e.dept_id IS NOT NULL;",
    //    db, parser, currentDbName);

//    // 清理测试数据库
//    std::cout << "\n--- 测试完成，清理数据库 ---" << std::endl;
//    process_sql("DROP TABLE projects;", db, parser, currentDbName);
//    process_sql("DROP TABLE employees;", db, parser, currentDbName);
//    process_sql("DROP TABLE departments;", db, parser, currentDbName);
//}
 //在你的 main 函数中调用:
int main() {
    // ... (你可能有的其他测试或初始化) ...
    run_constraint_tests(); // 调用新的测试函数
    // ... (交互式循环等) ...
    return 0;
}