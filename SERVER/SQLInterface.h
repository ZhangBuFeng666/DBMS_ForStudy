#ifndef SQLINTERFACE_H
#define SQLINTERFACE_H

#include "Storage.h"     // 包含 FileManager 定义
#include "SQLParser.h"   // 包含 SQLCommand 定义
#include <string>
#include <vector>
#include <map>

// --- 定义 SELECT 查询结果的结构体 ---
struct SelectResult {
    bool success = false;                           // 操作是否成功执行
    std::vector<std::string> header;              // 查询结果的列名列表 (投影后的)
    std::vector<std::vector<std::string>> data; // 查询结果数据 (行的列表，每行是值的列表)
    std::string errorMessage;                     // 如果 success 为 false，存储错误信息
};


// SQL 接口类，封装数据库操作逻辑
class SQLInterface {
private:
    FileManager fileManager; // 文件管理器实例
    // 元数据和数据根路径常量
    const std::string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/";
    const std::string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";
    const std::string COMMONDATA_ROOT = "DATA/COMMONDATA/";

public:
    // --- (构造函数, 用户/数据库管理, DDL, INSERT, UPDATE, DELETE 方法保持不变) ---
    SQLInterface() {} // 默认构造函数
    bool create_user(const std::string& username, const std::string& password, const std::string& privilege);
    bool create_database(const std::string& username);
    bool create_table(const std::string& dbName, const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
        const std::map<std::string, int>& constraints);
    bool drop_table(const std::string& dbName, const std::string& tableName);
    bool alter_table(const SQLCommand& command);
    bool insert_into_table(const std::string& dbName, const std::string& tableName, const std::vector<std::string>& values);
    bool update_table_row(const std::string& dbName, const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& setClauses,
        const std::string& whereColumn, const std::string& whereValue);
    bool delete_table_row(const std::string& dbName, const std::string& tableName,
        const std::string& whereColumn, const std::string& whereValue);
    std::string grant_privilege_sql(const std::string& username, const std::string& privilegeType);
    FileManager& getFileManager() { return fileManager; }

    // --- 新增 SELECT 操作方法 ---
    // 参数: 解析后的 SELECT 命令结构体
    // 返回: 包含查询结果或错误信息的 SelectResult 结构体
    SelectResult select_from_table(const SQLCommand& command);

private:
    // --- (私有辅助函数声明保持不变) ---
    bool validateFieldType(const std::string& typeStr);
    bool validateValueType(const std::string& type, const std::string& value);
    std::vector<std::string> parseFieldTypes(const std::string& dbName, const std::string& tableName);
    // (内部辅助函数如 findColumnIndex 等定义在 .cpp 的匿名空间)
};

#endif // SQLINTERFACE_H