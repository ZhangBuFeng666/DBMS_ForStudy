#include <iostream>
#include "SQLParser.h"
#include "SQLInterface.h"
using namespace std;


void print_values(const vector<string>& values) {
    using namespace std;

    cout << "解析值: ";
    for (const auto& v : values) {
        cout << "[" << v << "] ";
    }
    cout << endl;
}

void print_metadata(const string& dbName, const string& tableName) {

    using namespace std;


    cout << "\n=== 表元数据验证 ===" << endl;
    ifstream tdf("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + ".tdf");
    ifstream tic("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + ".tic");
    ifstream tid("DATA/METADATA/DBATTER/" + dbName + "/" + tableName + ".tid");

    cout << "字段名: ";
    string line;
    getline(tdf, line);
    cout << line << endl;

    cout << "字段类型: ";
    getline(tic, line);
    cout << line << endl;

    cout << "约束条件:\n";
    while (getline(tid, line)) {
        cout << "  " << line << endl;
    }
}

int main() {

    using namespace std;


    SQLInterface db;
    SQLParser parser;

    // === 用户与数据库初始化 ===
    cout << "\n=== 初始化用户 ===" << endl;
    if (db.create_user("alice", "123456", "admin")) {
        cout << "用户 'alice' 创建成功\n";
    }
    else {
        cout << "用户创建失败，已存在\n";
    }

    cout << "\n=== 创建数据库 ===" << endl;
    if (db.create_database("alice")) {
        cout << "数据库创建成功\n";
    }
    else {
        cout << "数据库创建失败\n";
        return 1;
    }

    // === 创建带完整字段定义的表 ===
    cout << "\n=== 创建学生表 ===" << endl;
    string createSQL = R"(
        CREATE TABLE student (
            sno INT PRIMARY KEY,
            sname CHAR(20),
            age INT,
            gender CHAR(1)
        );
    )";

    SQLCommand createCmd = parser.parse(createSQL);
    if (createCmd.type == SQLCommand::CREATE) {
        if (db.create_table("alice",
            createCmd.tableName,
            createCmd.fieldDefinitions,
            createCmd.constraints))
        {
            cout << "表 'student' 创建成功\n";
            print_metadata("alice", "student");
        }
        else {
            cout << "表创建失败\n";
            return 1;
        }
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
            if (db.insert_into_table("alice", cmd.tableName, rowData)) {
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
        "INSERT INTO student VALUES ('abc', 'Alice', 20, 'F');",  // 学号非整数
        "INSERT INTO student VALUES (1003, 'ThisIsALongNameExceedingLimit', 22, 'M');",  // 姓名超长
        "INSERT INTO student VALUES (1004, 'Charlie', 'twenty', 'X');"  // 年龄非整数
    };

    for (const auto& sql : invalidInserts) {
        SQLCommand cmd = parser.parse(sql);
        if (cmd.type == SQLCommand::INSERT) {
            string rowData;
            for (size_t i = 0; i < cmd.values.size(); ++i) {
                rowData += cmd.values[i] + (i < cmd.values.size() - 1 ? "," : "");
            }
            if (!db.insert_into_table("alice", cmd.tableName, rowData)) {
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
    string updateSQL = "UPDATE student SET 1001, 'Alice Smith', 21, 'F' WHERE 0;";
    SQLCommand updateCmd = parser.parse(updateSQL);
    if (updateCmd.type == SQLCommand::UPDATE) {
        if (db.update_table_row("alice",
            updateCmd.tableName,
            updateCmd.rowIndex,
            updateCmd.newRow))
        {
            cout << "行更新成功\n";
        }
        else {
            cout << "行更新失败\n";
        }
    }

    return 0;
}