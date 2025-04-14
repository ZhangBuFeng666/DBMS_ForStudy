#ifndef SQLINTERFACE_H
#define SQLINTERFACE_H

#include "Storage.h"
#include "Tools.h"
#include <string>
#include <vector>


// SQL 接口类，提供用户管理、数据库管理、表操作、记录操作等功能
class SQLInterface {

private:

    FileManager fileManager;
    // 修改后：
    const std::string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/"; // 用户元数据路径
    const std::string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";     // 数据库元数据路径
    const std::string COMMONDATA_ROOT = "DATA/COMMONDATA/";

public:
    bool create_user(const std::string& username, const std::string& password, const std::string& privilege);
    bool create_database(const std::string& username);
    bool create_table(const std::string& dbName, const std::string& tableName,
        const std::vector<std::string>& fields, const std::map<std::string, int>& constraints);
    bool drop_table(const std::string& dbName, const std::string& tableName);
    bool alter_table_add_field(const std::string& dbName, const std::string& tableName, const std::string& fieldDefinition);
    std::string grant_privilege_sql(const std::string& username, const std::string& privilegeType);
    bool insert_into_table(const std::string& dbName, const std::string& tableName, const std::string& rowData);
    bool update_table_row(const std::string& dbName, const std::string& tableName, int rowIndex, const std::string& newRow);
    bool delete_table_row(const std::string& dbName, const std::string& tableName, int rowIndex);
};

#endif // SQLINTERFACE_H
