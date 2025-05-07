#pragma once
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <map>
#include <variant>
#include <algorithm> // 需要包含 <algorithm> for find_if_not
#include <cctype>    // 需要包含 <cctype> for ::isspace

// 前向声明 SelectResult (如果其他地方需要，但通常在接口头文件中定义)
// namespace dbms { struct SelectResult; } // 这里暂时不需要

// (保持之前的内联 trim 函数)
inline std::string trim(const std::string& str) {
    auto is_space_safe = [](unsigned char ch) { return ::isspace(ch); };
    auto first = std::find_if_not(str.begin(), str.end(), is_space_safe);
    if (first == str.end()) { return ""; }
    auto last = std::find_if_not(str.rbegin(), str.rend(), is_space_safe).base();
    return std::string(first, last);
}

// 新增结构体：存储单列的约束标志
struct ColumnConstraintInfo {
    bool isPrimaryKey = false;
    bool isNotNull = false;
    bool isUnique = false;
};

// 定义 SQL 命令结构体
struct SQLCommand {
    // 基本命令类型枚举
    enum CommandType {
        CREATE, ALTER, DROP,
        INSERT, UPDATE, DELETE_NEW,
        SELECT,
        UNKNOWN
    } type = UNKNOWN; // 操作类型, 默认为未知

    // --- 通用字段 ---
    std::string tableName;  // 主要用于 DDL (CREATE, DROP, ALTER) 和部分 DML
    std::string dbName;     // 数据库上下文

    // --- INSERT 特定字段 ---
    std::vector<std::string> values; // 插入时的字段值列表

    // --- CREATE 特定字段 ---
    std::vector<std::pair<std::string, std::string>> fieldDefinitionsWithType; // 字段定义列表 {字段名, 字段类型}
    std::map<std::string, int> constraints; // 约束条件映射 (约束类型 -> 字段索引)
    std::vector<ColumnConstraintInfo> columnConstraintsInfo;  // 新增：存储每列的约束标志



    // --- UPDATE 特定字段 ---
    std::vector<std::pair<std::string, std::string>> setClauses; // SET 子句列表 {列名, 新值}

    // --- UPDATE/DELETE/SELECT 的 WHERE 子句字段 ---
    bool hasWhere = false;          // 标记是否有 WHERE 子句
    std::string whereClauseStr;     // 存储原始 WHERE 子句字符串 (用于解析)
    std::string whereColumn;        // WHERE 列名 (用于 = 和 IN)
    std::string whereOperator; //5.2-------------------------------
    bool useIsNullClause; // 新增：是否是 IS NULL 子句
    bool isNot;          // 新增：对于 IS NULL 子句，是否是 IS NOT NULL
    //5.2------------------------------
    std::string whereValue;         // WHERE = 的比较值
    bool useInClause = false;       // 标记 WHERE 子句是否使用 IN
    std::vector<std::string> inValues; // WHERE IN (...) 的值列表

    // --- ALTER 特定字段 ---
    enum AlterAction { // ALTER TABLE 操作的具体类型
        RENAME_TABLE, ADD_COLUMN, DROP_COLUMN, MODIFY_COLUMN, RENAME_COLUMN, INVALID_ALTER
    } alterAction = INVALID_ALTER;  // ALTER 操作类型, 默认为无效
    std::string newTableName;       // 用于 RENAME TABLE
    std::string columnName;         // 用于 ADD, DROP, MODIFY, RENAME (旧列名)
    std::string columnDefinition;   // 用于 ADD (如 "age INT"), MODIFY (新类型定义)
    std::string newColumnName;      // 用于 RENAME COLUMN (新列名)

    // --- SELECT 特定字段 ---
    std::vector<std::string> selectColumns; // 要选择的列名列表 ("*" 表示所有列)
    std::string fromTable;                  // FROM 子句指定的表名
    bool distinct = false;                  // 是否使用 DISTINCT 关键字去重
    std::string orderByColumn;              // ORDER BY 子句指定的排序列名 (如果为空则不排序)
    enum SortOrder { ASC, DESC } sortOrder = ASC; // 排序顺序 (默认 ASC 升序)

};

// SQL 解析器类
class SQLParser {
public:
    // 解析 SQL 字符串并返回 SQLCommand 结构体
    SQLCommand parse(const std::string& sql);

    // 执行 SQL 命令

private:
    // (保持现有的私有辅助函数)
    std::vector<std::string> split_values(const std::string& input);
    void parse_field_definitions(const std::string& fieldDefs, SQLCommand& cmd);
    void parse_set_clause(const std::string& setClauseStr, SQLCommand& cmd);

    // 修改 parse_where_clause 以支持 IN 子句
    bool parse_where_clause(const std::string& whereClauseStr, SQLCommand& cmd);

    // 新增 SELECT 解析辅助函数
    void parse_select_list(const std::string& selectListStr, SQLCommand& cmd); // 解析 SELECT 的列列表
    bool parse_order_by_clause(const std::string& orderByStr, SQLCommand& cmd); // 解析 ORDER BY 子句
};

#endif // SQLPARSER_H