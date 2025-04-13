#ifndef SQLINTERFACE_H
#define SQLINTERFACE_H

#include "Storage.h"
#include "Tools.h"
#include <string>
#include <vector>

using namespace std;

// SQL 接口类，提供用户管理、数据库管理、表操作、记录操作等功能
class SQLInterface {
private:
    FileManager fileManager;
    // 修改后：
    const string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/"; // 用户元数据路径
    const string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";     // 数据库元数据路径
    const string COMMONDATA_ROOT = "DATA/COMMONDATA/";

public:
    bool create_user(const string& username, const string& password, const string& privilege);
    bool create_database(const string& username);
    bool create_table(const std::string& dbName, const std::string& tableName,
        const vector<string>& fields, const map<string, int>& constraints);
    bool drop_table(const string& dbName, const string& tableName);
    bool alter_table_add_field(const string& dbName, const string& tableName, const string& fieldDefinition);
    string grant_privilege_sql(const string& username, const string& privilegeType);
    bool insert_into_table(const string& dbName, const string& tableName, const string& rowData);
    bool update_table_row(const string& dbName, const string& tableName, int rowIndex, const string& newRow);
    bool delete_table_row(const string& dbName, const string& tableName, int rowIndex);
};

#endif // SQLINTERFACE_H
