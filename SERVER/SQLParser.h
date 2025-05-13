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

    // 首先，定义单个条件表达式的结构
    struct Condition {
        std::string columnName;
        std::string op;         // 比较运算符: =, <>, >, <, >=, <=
        std::string value;      // 比较的值 (如果是字符串，应去除引号)
        bool isValueQuoted = false; // 标记原始值是否有引号

        bool useInClause = false;
        std::vector<std::string> inValues;

        bool useIsNullClause = false;
        bool isNotNull = false; // 如果 useIsNullClause 为 true, isNotNull 为 true 表示 IS NOT NULL

        // (未来可以扩展支持 LIKE 等)
    };

    // 然后，定义条件组，用于支持 AND/OR
    struct ConditionGroup {
        std::vector<Condition> conditions;          // 该组内的简单条件
        std::vector<std::string> logicalOperators;  // 连接 conditions 的逻辑运算符 ("AND", "OR")
        // logicalOperators.size() == conditions.size() - 1

    // 为了支持更复杂的嵌套 (例如 WHERE (A AND B) OR C)，您可能需要递归结构:
    // std::vector<std::variant<Condition, ConditionGroup>> operands;
    // std::vector<std::string> logicalConnectors; // "AND", "OR"
    // 但初期可以先从简单的线性 AND/OR 开始
    };
    // 基本命令类型枚举
    enum CommandType {
        CREATE_TABLE,CREATE_USER, ALTER, DROP,
        INSERT, UPDATE, DELETE_NEW,
        SELECT,
        UNKNOWN
    } type = UNKNOWN; // 操作类型, 默认为未知

    // --- 通用字段 ---
    std::string tableName;  // 主要用于 DDL (CREATE, DROP, ALTER) 和部分 DML
    std::string dbName;     // 数据库上下文

    // --- INSERT 特定字段 ---
    std::vector<std::string> values; // 插入时的字段值列表

    // --- CREATE_TABLE 特定字段 ---
    std::vector<std::pair<std::string, std::string>> fieldDefinitionsWithType; // 字段定义列表 {字段名, 字段类型}
    std::map<std::string, int> constraints; // 约束条件映射 (约束类型 -> 字段索引)
    std::vector<ColumnConstraintInfo> columnConstraintsInfo;  // 新增：存储每列的约束标志



    // --- CREATE_USER 特定字段 ---
    std::string userID = "";
    std::string userPassword = "";
    int right = -1;
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

    // 新增：存储解析后的WHERE条件组
    ConditionGroup whereConditions; // 对于简单的实现，一个 ConditionGroup 可能就够了

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

    // 新增: JOIN 操作相关字段
    bool hasJoin = false;                   // 标记是否有 JOIN 子句
    std::string joinType;                   // JOIN 类型 (例如 "INNER", "LEFT" - 初期可先实现 "INNER")
    std::string joinTable;                  // JOIN 的第二个表名
    std::string joinOnConditionLeft;        // ON 条件的左侧列 (例如 "table1.columnA" 或 "columnA")
    std::string joinOperator;               // **** 新增字段: 用于存储 JOIN ON 条件中的比较运算符 ****
    std::string joinOnConditionRight;       // ON 条件的右侧列 (例如 "table2.columnB" 或 "columnB")
    // 注意: 为了处理 table.column 格式，selectColumns, whereColumn, orderByColumn,
    // joinOnConditionLeft, joinOnConditionRight 中的字符串可能需要存储或能够解析这种格式。

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
    void parse_table_definitions(const std::string& tableDefs, SQLCommand& cmd);
    void parse_user_definitions(const std::string& userDefs, SQLCommand& cmd);
    void parse_set_clause(const std::string& setClauseStr, SQLCommand& cmd);

    // 修改 parse_where_clause 以支持 IN 子句
    bool parse_where_clause(const std::string& whereClauseStr, SQLCommand& cmd);

    // 新增 SELECT 解析辅助函数
    void parse_select_list(const std::string& selectListStr, SQLCommand& cmd); // 解析 SELECT 的列列表
    bool parse_order_by_clause(const std::string& orderByStr, SQLCommand& cmd); // 解析 ORDER BY 子句
};

#endif // SQLPARSER_H