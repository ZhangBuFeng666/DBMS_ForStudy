#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "SQLParser.h"
#include "SQLInterface.h"

using namespace std;

// 打印解析后的值
void print_values(const vector<string>& values) {
    using namespace std;

    cout << "解析值: ";
    for (const auto& v : values) {
        cout << "[" << v << "] ";
    }
    cout << endl;
}

// 打印表元数据信息
void print_metadata(const string& dbName, const string& tableName) {
    using namespace std;

    cout << "\n=== 表元数据验证 ===" << endl;
    ifstream tdf("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + "/" + tableName + ".tdf");
    ifstream tic("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + "/" + tableName + ".tic");
    ifstream tid("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + "/" + tableName + ".tid");

    cout << "字段名: ";
    string line;
    while (getline(tdf, line)) {
        cout << line << " ";
    }
    cout << endl;

    cout << "字段类型: ";
    while (getline(tic, line)) {
        cout << line << " ";
    }
    cout << endl;

    cout << "约束条件:\n";
    while (getline(tid, line)) {
        cout << "  " << line << endl;
    }
}

int main() {
    using namespace std;

    SQLInterface db;
    SQLParser parser;
    string dbName = "alice";
    string tableName = "student";

    // === 用户与数据库初始化 ===
    cout << "\n=== 初始化用户 ===" << endl;
    string createUserSQL = "CREATE USER alice IDENTIFIED BY '123456' WITH PRIVILEGE 'admin';";
    // 假设用户创建逻辑直接在 main 中测试
    if (db.create_user("alice", "123456", "admin")) {
        cout << "用户 'alice' 创建成功\n";
    }
    else {
        cout << "用户创建失败，已存在\n";
    }

    cout << "\n=== 创建数据库 ===" << endl;
    string createDatabaseSQL = "CREATE DATABASE alice;";
    // 假设数据库创建逻辑直接在 main 中测试
    if (db.create_database("alice")) {
        cout << "数据库创建成功\n";
    }
    else {
        cout << "数据库创建失败，已创建\n";
    }

    // === 创建带完整字段定义的表 ===
    cout << "\n=== 创建学生表 ===" << endl;
    string createTableSQL = R"(
        CREATE TABLE student (
            sno INT PRIMARY KEY,
            sname CHAR(20),
            age INT,
            gender CHAR(1)
        );
    )";


    // 在调用 parse 之后
    SQLCommand createCmd = parser.parse(createTableSQL);

    // 打印字段定义
    std::cout << "Field Definitions:" << std::endl;
    for (const auto& field : createCmd.fieldDefinitionsWithType) {
        std::cout << "  " << field.first << " : " << field.second << std::endl;
    }

    // 完整示例：
    std::cout << "\n=== 打印字段定义 ===" << std::endl;
    if (createCmd.type == SQLCommand::CREATE) {
        std::cout << "表名: " << createCmd.tableName << std::endl;
        std::cout << "字段列表:" << std::endl;

        for (size_t i = 0; i < createCmd.fieldDefinitionsWithType.size(); ++i) {
            const auto& field = createCmd.fieldDefinitionsWithType[i];
            std::cout << "  [" << i << "] " << field.first
                << " (类型: " << field.second << ")";

            // 如果有主键约束也打印出来
            auto it = createCmd.constraints.find("primary_key");
            if (it != createCmd.constraints.end() && it->second == static_cast<int>(i)) {
                std::cout << " [PRIMARY KEY]";
            }

            std::cout << std::endl;
        }
    }
    else {
        std::cout << "不是CREATE TABLE语句" << std::endl;
    }




    if (createCmd.type == SQLCommand::CREATE) {
        if (db.create_table(dbName,
            createCmd.tableName,
            createCmd.fieldDefinitionsWithType,
            createCmd.constraints))
        {
            cout << "表 'student' 创建成功\n";
            print_metadata(dbName, createCmd.tableName);
        }
        else {
            cout << "表创建失败，已存在\n";
        }
    }
    else {
        cout << "解析错误: " << createCmd.type << endl;
    }

    // === 插入合法数据 ===
    cout << "\n=== 测试合法数据插入 ===" << endl;
    vector<string> validInserts = {
        "INSERT INTO student VALUES (1001, 'Alice', 20, 'F');",
        "INSERT INTO student VALUES (1002, 'Bob', 22, 'M');"
    };

    for (const auto& sql : validInserts) {
        SQLCommand cmd = parser.parse(sql);
        if (cmd.type == SQLCommand::INSERT) {
            string rowData;
            for (size_t i = 0; i < cmd.values.size(); ++i) {
                rowData += cmd.values[i] + (i < cmd.values.size() - 1 ? "," : "");
            }
            cout << "[DEBUG] 准备插入数据: " << rowData << endl;
            if (db.insert_into_table(dbName, cmd.tableName, rowData)) {
                cout << "成功插入: ";
                print_values(cmd.values);
            }
            else {
                cout << "插入失败: ";
                print_values(cmd.values);
            }
        }
    }

    // === 插入非法数据 ===
    cout << "\n=== 测试非法数据插入 ===" << endl;
    vector<string> invalidInserts = {
        "INSERT INTO student VALUES ('abc', 'Alice', 20, 'F');",    // 学号非整数
        "INSERT INTO student VALUES (1003, 'ThisIsALongNameExceedingLimit', 22, 'M');", // 姓名超长
        "INSERT INTO student VALUES (1004, 'Charlie', 'twenty', 'X');"      // 年龄非整数
    };

    for (const auto& sql : invalidInserts) {
        SQLCommand cmd = parser.parse(sql);
        if (cmd.type == SQLCommand::INSERT) {
            string rowData;
            for (size_t i = 0; i < cmd.values.size(); ++i) {
                rowData += cmd.values[i] + (i < cmd.values.size() - 1 ? "," : "");
            }
            cout << "[DEBUG] 准备插入非法数据: " << rowData << endl;
            if (!db.insert_into_table(dbName, cmd.tableName, rowData)) {
                cout << "拦截非法数据: ";
                print_values(cmd.values);
            }
            else {
                cout << "错误：非法数据被接受: ";
                print_values(cmd.values);
            }
        }
    }

    // === 更新数据 ===
    cout << "\n=== 测试数据更新 ===" << endl;
    string updateSQL = "UPDATE student SET sno=1001,sname='Alice Smith',age=21,gender='F' WHERE 0;";
    SQLCommand updateCmd = parser.parse(updateSQL);
    if (updateCmd.type == SQLCommand::UPDATE) {
        string newRowData = "1001,'Alice Smith',21,'F'";
        if (db.update_table_row(dbName,
            updateCmd.tableName,
            updateCmd.rowIndex,
            newRowData)) {
            cout << "行更新成功\n";
        }
        else {
            cout << "行更新失败\n";
        }
    }

    //// === 删除数据 ===
    //cout << "\n=== 测试数据删除 ===" << endl;
    //string deleteSQL = "DELETE FROM student WHERE 0;";
    //SQLCommand deleteCmd = parser.parse(deleteSQL);
    //if (deleteCmd.type == SQLCommand::DELETE) {
    //    if (db.delete_table_row(dbName, deleteCmd.tableName, deleteCmd.rowIndex)) {
    //        cout << "行删除成功\n";
    //    }
    //    else {
    //        cout << "行删除失败\n";
    //    }
    //}

    // === 删除表 ===
    cout << "\n=== 测试删除表 ===" << endl;
    string dropTableSQL = "DROP TABLE student;";
    SQLCommand dropCmd = parser.parse(dropTableSQL);
    if (dropCmd.type == SQLCommand::DROP) {
        if (db.drop_table(dbName, dropCmd.tableName)) {
            cout << "表 'student' 删除成功\n";
        }
        else {
            cout << "表删除失败\n";
        }
    }

    return 0;
}