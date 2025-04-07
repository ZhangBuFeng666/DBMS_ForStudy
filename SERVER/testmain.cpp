#include <iostream>
#include "SQLParser.h"
#include "SQLInterface.h"

using namespace std;

void print_values(const vector<string>& values) {
    cout << "Parsed values: ";
    for (const auto& v : values) {
        cout << "[" << v << "] ";
    }
    cout << endl;
}

int main() {
    SQLInterface db;
    SQLParser parser;

    // === 用户与数据库初始化 ===
    cout << "Creating user..." << endl;
    if (db.create_user("alice", "123456", "admin")) {
        cout << "User 'alice' created successfully.\n";
    }
    else {
        cout << "Failed to create user 'alice'.\n";
    }

    cout << "Creating database for user..." << endl;
    if (db.create_database("alice")) {
        cout << "Database for 'alice' created successfully.\n";
    }
    else {
        cout << "Failed to create database for 'alice'.\n";
    }

    // === 创建表 ===
    cout << "Creating table 'student'...\n";
    if (db.create_table("alice", "student")) {
        cout << "Table 'student' created successfully.\n";
    }
    else {
        cout << "Failed to create table 'student'.\n";
    }

    // === 插入数据 ===
    string sql_insert = "INSERT INTO student VALUES ('Tom', '20', 'CS');";
    SQLCommand cmd = parser.parse(sql_insert);

    if (cmd.type == SQLCommand::INSERT) {
        print_values(cmd.values);
        string rowData = cmd.values[0] + "," + cmd.values[1] + "," + cmd.values[2];
        if (db.insert_into_table("alice", cmd.tableName, rowData)) {
            cout << "Data inserted into 'student'.\n";
        }
        else {
            cout << "Failed to insert data.\n";
        }
    }

    // === 更新数据行 ===
    cout << "Updating row 0 in table 'student'...\n";
    if (db.update_table_row("alice", "student", 0, "Tom,21,CS")) {
        cout << "Row updated successfully.\n";
    }
    else {
        cout << "Failed to update row.\n";
    }

    // === 删除数据行 ===
    cout << "Deleting row 0 from table 'student'...\n";
    if (db.delete_table_row("alice", "student", 0)) {
        cout << "Row deleted successfully.\n";
    }
    else {
        cout << "Failed to delete row.\n";
    }

    // === 删除表 ===
    cout << "Dropping table 'student'...\n";
    if (db.drop_table("alice", "student")) {
        cout << "Table dropped successfully.\n";
    }
    else {
        cout << "Failed to drop table.\n";
    }

    return 0;
}
