#ifndef SQLINTERFACE_H
#define SQLINTERFACE_H

#include "Storage.h" // 包含 FileManager 定义
// #include "Tools.h" // 如果 Tools.h 存在且需要
#include "SQLParser.h" // 包含 SQLCommand 定义
#include <string>
#include <vector>
#include <map>

// SQL 接口类，封装数据库操作逻辑
class SQLInterface {

private:
    FileManager fileManager; // 文件管理器实例，用于底层文件操作
    // 定义元数据和数据根路径常量 (与 FileManager 中的应保持一致或从其获取)
    const std::string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/";
    const std::string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";
    const std::string COMMONDATA_ROOT = "DATA/COMMONDATA/";

public:
    // --- 构造函数 (如果需要初始化) ---
    // SQLInterface() {} // 默认构造函数

    // --- 用户和数据库管理 ---
    bool create_user(const std::string& username, const std::string& password, const std::string& privilege);
    bool create_database(const std::string& username); // 注意：当前实现是为用户创建同名目录，更像是 schema

    // --- 表结构操作 (DDL) ---
    bool create_table(const std::string& dbName, const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
        const std::map<std::string, int>& constraints);
    bool drop_table(const std::string& dbName, const std::string& tableName);
    // 新增：处理 ALTER TABLE 命令
    bool alter_table(const SQLCommand& command);
    // 旧的 alter_table_add_field 已被新的 alter_table 取代
    // bool alter_table_add_field(const std::string& dbName, const std::string& tableName, const std::string& fieldDefinition);

    // --- 数据操作 (DML) ---
     // 插入数据 (修改为接收值列表)
    bool insert_into_table(const std::string& dbName, const std::string& tableName, const std::vector<std::string>& values);
    // 更新数据 (使用 WHERE 字段=值)
    bool update_table_row(const std::string& dbName, const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& setClauses,
        const std::string& whereColumn, const std::string& whereValue);
    // 删除数据 (使用 WHERE 字段=值)
    bool delete_table_row(const std::string& dbName, const std::string& tableName,
        const std::string& whereColumn, const std::string& whereValue);

    // --- 权限管理 (示例) ---
    std::string grant_privilege_sql(const std::string& username, const std::string& privilegeType);

    // --- 暴露 FileManager (如果 main 中需要直接访问) ---
    FileManager& getFileManager() { return fileManager; } // 提供对 FileManager 的访问


    // --- 私有辅助函数声明 (如果很多，可以移到 .cpp 的匿名空间) ---
private:
    // 校验字段类型是否合法 (例子)
    bool validateFieldType(const std::string& typeStr);
    // 校验值是否符合给定类型 (例子)
    bool validateValueType(const std::string& type, const std::string& value);
    // 解析字段类型文件 (.tic)
    std::vector<std::string> parseFieldTypes(const std::string& dbName, const std::string& tableName);
    // 其他可能需要的私有辅助函数...

};

#endif // SQLINTERFACE_H