#ifndef SQLINTERFACE_H
#define SQLINTERFACE_H

#include "Storage.h"
#include "Tools.h"
#include <string>
#include <vector>
#include <map>

// SQL 接口类，提供用户管理、数据库管理、表操作、记录操作等功能
class SQLInterface {

private:
    FileManager fileManager;
    // 用户元数据根路径
    const std::string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/";
    // 数据库元数据根路径
    const std::string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";
    // 通用数据根路径
    const std::string COMMONDATA_ROOT = "DATA/COMMONDATA/";

public:
    // 创建新用户
    bool create_user(const std::string& username, const std::string& password, const std::string& privilege);
    // 为指定用户创建数据库
    bool create_database(const std::string& username);
    // 在指定数据库中创建新表
    bool create_table(const std::string& dbName, const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
        const std::map<std::string, int>& constraints);
    // 删除指定数据库中的表
    bool drop_table(const std::string& dbName, const std::string& tableName);
    // 向指定表中添加新字段
    bool alter_table_add_field(const std::string& dbName, const std::string& tableName, const std::string& fieldDefinition);
    // 生成授权指定用户特定权限的 SQL 语句（当前未实现具体授权逻辑）
    std::string grant_privilege_sql(const std::string& username, const std::string& privilegeType);
    // 向指定表中插入一行数据
    bool insert_into_table(const std::string& dbName, const std::string& tableName, const std::string& rowData);
    // 更新指定表中指定行的数据
    bool update_table_row(const std::string& dbName, const std::string& tableName, int rowIndex, const std::string& newRow);
    // 删除指定表中指定行的数据
    bool delete_table_row(const std::string& dbName, const std::string& tableName, int rowIndex);
};

#endif // SQLINTERFACE_H