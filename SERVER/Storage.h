#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream> // 用于文件流操作
#include <regex>
#include <filesystem> // C++17 文件系统库
#include <iostream> // 用于调试输出
#include "SQLParser.h"

// Windows 特定的头文件和库，用于文件/目录操作 (如果你只在 Windows 上运行)
#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h> // PathCanonicalizeA 需要
#include <AccCtrl.h> // 可能需要，如果处理权限
#include <Aclapi.h>  // 可能需要，如果处理权限
#pragma comment(lib, "Shlwapi.lib") // 链接 Shlwapi 库
#endif

/*
文件系统架构说明 (与 SQLInterface 中的一致):
DATA
|- METADATA
|  |- USERATTER/        -- 用户元数据目录
|  |  |- user_header.txt -- 用户表头定义文件
|  |  |- user_data.txt   -- 用户数据记录文件
|  |- DBATTER/          -- 数据库元数据根目录
|  |  |- [用户名]/       -- 每个用户一个数据库目录 (作为数据库名)
|  |  |  |- [数据库名]/   -- 数据库目录 (这里使用用户名作为数据库名，实际应用可能不同)
|  |  |    |- [表名]/     -- 新增：每个表有独立的元数据目录
|  |  |      |- [表名].tdf -- 字段名文件 (每行一个字段名)
|  |  |      |- [表名].tic -- 字段类型文件 (每行一个字段类型)
|  |  |      |- [表名].tid -- 约束文件 (每行一个约束)
|
|- COMMONDATA           -- 通用数据根目录 (数据文件放在这里)
            |- [表名].trd -- 表数据文件 (每行一条记录)
*/


// 文件管理器类，负责底层的文件和目录操作
class FileManager {
private:
    // 数据库元数据根路径
    const std::string METADATA_ROOT = "DATA/METADATA/DBATTER/";
    // 通用数据根路径
    const std::string COMMON_ROOT = "DATA/COMMONDATA/";
    // 用户元数据根路径 (用于创建用户相关的目录)
    const std::string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/"; // 新增或确认

    // 写入二进制文件的模板函数 (当前未使用，保留)
    template<typename T>
    int write_to_file(const std::string& path, const T& data) {
        std::ofstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) return 0; // 失败
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        return file.good() ? 1 : 0; // 成功返回 1，失败返回 0
    }

    // 创建（空的）二进制文件的函数 (当前未使用，保留)
    // 0:失败, 1:成功, -1:文件已存在
    int write_to_file(const std::string& path);

public:

    // 构造函数 (可以用来进行一些初始化检查)
    FileManager();

    // 创建新表所需的所有文件和目录
    bool create_table(
        const std::string& dbName,
        const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
        const std::map<std::string, int>& tableLevelConstraints,
        const std::vector<ColumnConstraintInfo>& columnConstraints);


    // 删除指定数据库中的表的所有相关文件和目录
    bool delete_table(const std::string& dbName, const std::string& tableName);

    // 创建文件夹 (包括其所有不存在的父目录)
    bool create_directory(const std::string& path);


    // 删除文件
    bool delete_file(const std::string& path);
};
