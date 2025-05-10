#include "SQLParser.h"
#include "SQLInterface.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits> // 用于 numeric_limits (输入循环退出)
#include <stdexcept> // 用于异常处理 (如果 SQLInterface 可能抛出)



// 使用命名空间
using namespace std;

// --- 前向声明辅助函数 ---
void print_values(const vector<string>& values);
void print_metadata(const string& dbName, const string& tableName, SQLInterface& db); // 传递 db 访问路径

// --- 核心 SQL 处理函数 ---
// 参数: sql语句, 数据库接口实例, SQL解析器实例, 当前数据库名
// 返回: true 如果成功执行, false 如果失败
bool process_sql(const string& sql,
    SQLInterface& db,
    SQLParser& parser,
    const string& currentDbName)
{
    cout << "\n>> 正在处理 SQL: " << sql << endl;
    if (sql.empty()) {
        return true; // 空命令直接返回成功
    }
    SQLCommand cmd = parser.parse(sql); // 解析 SQL 语句
    cmd.dbName = currentDbName; // 设置命令的数据库上下文

    bool success = false; // 操作成功标志

    try { // 包裹数据库操作，捕获可能的异常
        switch (cmd.type) {
        case SQLCommand::CREATE:
            if (cmd.tableName.empty() || cmd.fieldDefinitionsWithType.empty()) {
                cerr << "错误: 无效的 CREATE TABLE 语句 (缺少表名或字段定义)。" << endl;
                success = false;
            }
            else {
                cout << "尝试在数据库 '" << cmd.dbName << "' 中创建表 '" << cmd.tableName << "'" << endl;
                // 打印解析到的字段和约束 (调试用)
                cout << "   字段定义:" << endl;
                for (const auto& f : cmd.fieldDefinitionsWithType) cout << "    - " << f.first << " " << f.second << endl;
                cout << "   约束条件:" << endl;
                for (const auto& c : cmd.constraints) cout << "    - " << c.first << " 在索引 " << c.second << endl;

                success = db.create_table(cmd.dbName, cmd.tableName, cmd.fieldDefinitionsWithType, cmd.constraints);

                if (success) {
                    cout << "成功: 表 '" << cmd.tableName << "' 已创建。" << endl;
                    print_metadata(cmd.dbName, cmd.tableName, db); // 创建后打印元数据验证
                }
                else {
                    cerr << "失败: 无法创建表 '" << cmd.tableName << "' (可能已存在或发生错误)。" << endl;
                }
            }
            break;

        case SQLCommand::DROP:
            if (cmd.tableName.empty()) {
                cerr << "错误: 无效的 DROP TABLE 语句 (缺少表名)。" << endl;
                success = false;
            }
            else {
                cout << "尝试从数据库 '" << cmd.dbName << "' 中删除表 '" << cmd.tableName << "'" << endl;
                success = db.drop_table(cmd.dbName, cmd.tableName);
                if (success) {
                    cout << "成功: 表 '" << cmd.tableName << "' 已删除。" << endl;
                }
                else {
                    cerr << "失败: 无法删除表 '" << cmd.tableName << "' (可能不存在或发生错误)。" << endl;
                }
            }
            break;

        case SQLCommand::INSERT:
            if (cmd.tableName.empty() || cmd.values.empty()) {
                cerr << "错误: 无效的 INSERT 语句 (缺少表名或值)。" << endl;
                success = false;
            }
            else {
                cout << "尝试向表 '" << cmd.tableName << "' 插入数据" << endl;
                print_values(cmd.values); // 打印要插入的值 (调试用)
                // 调用修改后的 insert_into_table，传递值向量
                success = db.insert_into_table(cmd.dbName, cmd.tableName, cmd.values);
                if (success) {
                    cout << "成功: 数据已插入。" << endl;
                }
                else {
                    cerr << "失败: 无法插入数据 (请检查类型、约束和表是否存在)。" << endl;
                }
            }
            break;

        case SQLCommand::UPDATE:
            if (cmd.tableName.empty() || cmd.setClauses.empty() || cmd.whereColumn.empty()) {
                cerr << "错误: 无效的 UPDATE 语句 (缺少表名、SET 或 WHERE 子句)。" << endl;
                success = false;
            }
            else {
                cout << "尝试更新表 '" << cmd.tableName << "' 中 WHERE " << cmd.whereColumn << " = '" << cmd.whereValue << "' 的行" << endl;
                cout << "   SET 子句:" << endl;
                for (const auto& p : cmd.setClauses) cout << "    - " << p.first << " = '" << p.second << "'" << endl;

                success = db.update_table_row(cmd.dbName, cmd.tableName, cmd.setClauses, cmd.whereColumn, cmd.whereValue);
                // 成功/失败消息已在 update_table_row 内部打印
            }
            break;

        case SQLCommand::DELETE:
            if (cmd.tableName.empty() || cmd.whereColumn.empty()) {
                cerr << "错误: 无效的 DELETE 语句 (缺少表名或 WHERE 子句)。" << endl;
                success = false;
            }
            else {
                cout << "尝试从表 '" << cmd.tableName << "' 中删除 WHERE " << cmd.whereColumn << " = '" << cmd.whereValue << "' 的行" << endl;
                success = db.delete_table_row(cmd.dbName, cmd.tableName, cmd.whereColumn, cmd.whereValue);
                // 成功/失败消息已在 delete_table_row 内部打印
            }
            break;

        case SQLCommand::ALTER:
            if (cmd.tableName.empty() || cmd.alterAction == SQLCommand::INVALID_ALTER) {
                cerr << "错误: 无效的 ALTER TABLE 语句 (缺少表名或无效的操作)。" << endl;
                success = false;
            }
            else {
                cout << "尝试修改表 '" << cmd.tableName << "'" << endl;
                success = db.alter_table(cmd); // 将整个命令结构传递给 alter_table
                // 成功/失败消息已在 alter_table 内部打印 (对于每个子操作)
            }
            break;

        case SQLCommand::UNKNOWN:
        default:
            cerr << "错误: 无法解析 SQL 语句或命令类型未知。" << endl;
            success = false;
            break;
        }
    }
    catch (const runtime_error& e) { // 捕获可能的运行时错误
        cerr << "运行时错误: " << e.what() << endl;
        success = false;
    }
    catch (const exception& e) { // 捕获其他标准异常
        cerr << "发生异常: " << e.what() << endl;
        success = false;
    }
    catch (...) { // 捕获所有其他未知异常
        cerr << "发生未知异常。" << endl;
        success = false;
    }

    cout << "<< 处理结束。" << (success ? " (成功)" : " (失败)") << endl;
    return success;
}


// --- 辅助函数实现 ---

// 打印值列表 (用于调试)
void print_values(const vector<string>& values) {
    cout << "   值: ";
    for (const auto& v : values) {
        cout << "[" << v << "] ";
    }
    cout << endl;

}

// 打印表的元数据 (需要访问路径，通过 SQLInterface 获取)
void print_metadata(const string& dbName, const string& tableName, SQLInterface& db) {
    // 从 SQLInterface 获取路径 (或者直接使用常量，但通过接口更好封装)
    // 注意: 这里的路径构造需要与 SQLInterface 内部一致
    const string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/"; // 假设路径常量
    string tableMetaDir = METADATA_DB_ROOT + dbName + "/" + tableName + "/";
    string tdfPath = tableMetaDir + tableName + ".tdf";
    string ticPath = tableMetaDir + tableName + ".tic";
    string tidPath = tableMetaDir + tableName + ".tid";

    cout << "\n   --- 验证表 '" << tableName << "' 的元数据 ---" << endl;

    ifstream tdf(tdfPath);
    if (!tdf.is_open()) { cout << "   错误: 无法打开 .tdf 文件: " << tdfPath << endl; }
    else {
        cout << "   字段名 (.tdf): "; string line;
        while (getline(tdf, line)) { cout << trim(line) << " | "; } cout << endl;
        tdf.close();
    }

    ifstream tic(ticPath);
    if (!tic.is_open()) { cout << "   错误: 无法打开 .tic 文件: " << ticPath << endl; }
    else {
        cout << "   字段类型 (.tic): "; string line;
        while (getline(tic, line)) { cout << trim(line) << " | "; } cout << endl;
        tic.close();
    }

    ifstream tid(tidPath);
    if (!tid.is_open()) { /* cout << "   信息: 未找到 .tid 文件 (无约束或错误): " << tidPath << endl; */ } // tid 文件可能不存在
    else {
        cout << "   约束条件 (.tid):"; bool constraints_found = false; string line;
        while (getline(tid, line)) {
            if (!trim(line).empty()) { // 只打印非空行
                if (!constraints_found) cout << endl; // 只有找到约束才换行
                cout << "     " << trim(line) << endl;
                constraints_found = true;
            }
        }
        if (!constraints_found) cout << " (无)" << endl;
        tid.close();
    }
    cout << "   -------------------------------------" << endl;
}


// --- 主函数 ---
int main() {
    SQLInterface db;    // 创建数据库接口实例
    SQLParser parser;   // 创建 SQL 解析器实例
    string currentDbName = "myTestDB"; // 设置当前操作的数据库名 (需要先创建)

    cout << "--- 微型数据库管理系统初始化 ---" << endl;

    // --- 确保基础目录存在 ---
    cout << "检查/创建基础目录..." << endl;
    // 使用 SQLInterface 提供的 FileManager (假设有访问器或公共成员)
    FileManager& fm = db.getFileManager(); // 获取 FileManager 引用
    const string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/";
    const string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";
    const string COMMONDATA_ROOT = "DATA/COMMONDATA/";
    fm.create_directory(METADATA_USER_ROOT);
    fm.create_directory(METADATA_DB_ROOT);
    fm.create_directory(COMMONDATA_ROOT);
    // 尝试创建当前数据库目录 (如果 create_database 没被调用)
    if (!db.create_database(currentDbName)) {
        // 如果创建失败，可能已存在，继续执行，或者处理错误
        // cerr << "警告: 无法创建或确认数据库目录 " << currentDbName << endl;
    }

    cout << "当前数据库设置为: " << currentDbName << endl;

    // === 执行一系列 SQL 命令进行测试 ===

    // --- 清理旧表 (如果存在) ---
    process_sql("DROP TABLE students;", db, parser, currentDbName);
    process_sql("DROP TABLE courses;", db, parser, currentDbName);
    process_sql("DROP TABLE pupils;", db, parser, currentDbName); // 清理上次重命名的表

    // --- 创建表 ---
    process_sql("CREATE TABLE students (sid INT PRIMARY KEY, sname CHAR(50), age INT, major CHAR(30));", db, parser, currentDbName);
    process_sql("CREATE TABLE courses (cid CHAR(10) PRIMARY KEY, cname CHAR(50), credits INT);", db, parser, currentDbName);
    process_sql("CREATE TABLE empty_table (id INT);", db, parser, currentDbName); // 创建一个空表测试

    // --- 插入数据 ---
    process_sql("INSERT INTO students VALUES (101, '爱丽丝', 20, '计算机科学');", db, parser, currentDbName);
    process_sql("INSERT INTO students VALUES (102, '鲍勃', 22, '土木工程');", db, parser, currentDbName);
    process_sql("INSERT INTO students VALUES (103, '查理', 21, '表演艺术');", db, parser, currentDbName);
    process_sql("INSERT INTO students VALUES (104, '戴安娜', 19, '国际关系');", db, parser, currentDbName);
    // 测试带引号和逗号的值
    process_sql("INSERT INTO students VALUES (105, '爱德华 ''艾迪''', 23, '哲学, 历史');", db, parser, currentDbName);
    // 插入非法数据 (类型错误)
    process_sql("INSERT INTO students VALUES ('一百零六', '弗兰克', 24, '物理');", db, parser, currentDbName); // sid 类型错误
    // 插入非法数据 (长度错误)
    process_sql("INSERT INTO students VALUES (107, '格蕾丝', 25, '一个非常非常非常非常长的专业名称肯定会超过三十个字符的限制');", db, parser, currentDbName); // major 长度错误


    // --- 删除数据 ---
    process_sql("DELETE FROM students WHERE sid = 103;", db, parser, currentDbName); // 删除查理
    process_sql("DELETE FROM students WHERE major = '不存在的专业';", db, parser, currentDbName); // 应该影响 0 行

    // --- 更新数据 ---
    process_sql("UPDATE students SET age = 23 WHERE sname = '鲍勃';", db, parser, currentDbName); // 更新鲍勃年龄
    process_sql("UPDATE students SET major = '国际政治' WHERE sid = 104;", db, parser, currentDbName); // 更新戴安娜专业
    process_sql("UPDATE students SET age = 21, major = '人工智能' WHERE sid = 101;", db, parser, currentDbName); // 更新爱丽丝多个字段
    process_sql("UPDATE students SET age = 99 WHERE sid = 999;", db, parser, currentDbName); // 更新不存在的行，应影响 0 行

    // --- 修改表结构 (ALTER TABLE) ---
    cout << "\n--- 测试 ALTER TABLE ---" << endl;
    process_sql("ALTER TABLE students ADD COLUMN gpa INT;", db, parser, currentDbName); // 添加 gpa 列
    process_sql("UPDATE students SET gpa = 4 WHERE sid = 101;", db, parser, currentDbName); // 更新新列的值
    process_sql("UPDATE students SET gpa = 3 WHERE sid = 102;", db, parser, currentDbName);
    print_metadata(currentDbName, "students", db); // 验证添加列后的元数据

    process_sql("ALTER TABLE students RENAME COLUMN sname TO student_name;", db, parser, currentDbName); // 重命名列
    print_metadata(currentDbName, "students", db); // 验证重命名列后的元数据

    process_sql("ALTER TABLE students MODIFY COLUMN major CHAR(40);", db, parser, currentDbName); // 修改 CHAR 长度 (增加) - 应该成功
    process_sql("ALTER TABLE students MODIFY COLUMN credits INT;", db, parser, currentDbName); // 尝试修改不存在的列 - 应该失败
    process_sql("ALTER TABLE students MODIFY COLUMN age CHAR(3);", db, parser, currentDbName); // 尝试修改 INT 到 CHAR (有数据) - 应该失败
    process_sql("ALTER TABLE empty_table MODIFY COLUMN id CHAR(10);", db, parser, currentDbName); // 尝试修改空表的列类型 - 应该成功
    print_metadata(currentDbName, "students", db); // 验证修改列后的元数据
    print_metadata(currentDbName, "empty_table", db);

    process_sql("ALTER TABLE students DROP COLUMN age;", db, parser, currentDbName); // 删除列
    print_metadata(currentDbName, "students", db); // 验证删除列后的元数据

    process_sql("ALTER TABLE students RENAME TO pupils;", db, parser, currentDbName); // 重命名表
    // 尝试对旧表名操作 - 应该失败
    process_sql("INSERT INTO students VALUES (108, '汉娜', 20, '生物', 4);", db, parser, currentDbName);
    // 对新表名操作
    print_metadata(currentDbName, "pupils", db); // 验证新表名的元数据
    // 注意：因为删除了 age 列，插入时需要少一个值
    process_sql("INSERT INTO pupils VALUES (108, '汉娜', '生物', 4);", db, parser, currentDbName); // 向重命名后的表插入

    // --- 交互式输入循环 (可选) ---
    cout << "\n--- 进入交互模式 (输入空行或 EOF 退出) ---" << endl;
    string line;
    cout << currentDbName << "> ";
    // 使用 cin.eof() 和 getline 读取，允许空行退出
    while (getline(cin, line) && !line.empty()) {
        process_sql(line, db, parser, currentDbName); // 处理输入的 SQL
        cout << currentDbName << "> "; // 显示下一个提示符
    }

    cout << "\n--- 微型数据库管理系统关闭 ---" << endl;

    return 0; // 主函数正常退出
}