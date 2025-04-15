#pragma once
#include <fstream>
#include <vector>
#include <map>
#include <string>

/*
架构说明：
DATA
|- METADATA
|  |- USERATTER/
|  |  |- user_header.txt   -- 用户表头定义
|  |  |- user_data.txt     -- 用户数据记录
|  |- DBATTER/
|  |  |- [用户名]/
|  |     |- [数据库名]/
|  |        |- [表名]/      -- 新增表目录
|  |           |- [表名].tdf   -- 字段名文件 (每行一个字段名)
|  |           |- [表名].tic   -- 字段类型文件 (每行一个字段类型)
|  |           |- [表名].tid   -- 约束文件 (每行一个约束)
|
|- COMMONDATA
            |- [表名].trd     -- 表数据文件
*/

class FileManager {
private:
    // 数据库元数据根路径
    const std::string METADATA_ROOT = "DATA/METADATA/DBATTER/";
    // 通用数据根路径
    const std::string COMMON_ROOT = "DATA/COMMONDATA/";

    // 写入二进制文件的模板函数（未使用）
    template<typename T>
    int write_to_file(const std::string& path, const T& data) {
        std::ofstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) return false;
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        return file.good();
    }

    // 创建二进制文件的函数（未使用）
    // 0:失败
    // 1:成功
    // -1:文件已存在
    int write_to_file(const std::string& path);

public:
    // 创建新表，包括定义文件、类型文件、约束文件和数据文件
    bool create_table(const std::string& dbName,
        const std::string& tableName,
        const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
        const std::map<std::string, int>& constraints);
    // 删除指定数据库中的表结构及其数据
    bool delete_table(const std::string& dbName, const std::string& tableName);
    // 创建文件夹，如果父目录不存在则一并创建
    bool create_directory(const std::string& path);
    // 删除文件
    bool delete_file(const std::string& path);
};