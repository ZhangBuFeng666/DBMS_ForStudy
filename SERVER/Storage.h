#pragma once
#include <fstream>
#include <vector>
#include <map>
#include <string>

/*架构：DATA|-METADATA   |-用户属性表（定义，记录）
            |            |-数据库表（含数据库名、拥有的表的名）（定义，记录）
            |            |-数据库DB1文件夹     |-表TABLE1元数据（定义，约束，索引）
            |            |                     |-表TABLE2元数据（定义，约束，索引）
            |            |          .
            |            |          .
            |            |          .
            |            |-数据库DB2文件夹-表TABLE9元数据（定义，约束，索引）
            |            .
            |            .
            |            .
            |
            |-COMMONDATA |-数据库DB1文件夹    |-表TABLE1记录数据
                         |                    |-表TABLE2记录数据
                         |                .
                         |                .
                         |                .
                         |-数据库DB2文件夹-表TABLE9记录数据
                         .
                         .
                         .
*/

/* 架构更新说明：
   METADATA 目录结构：
   |- DBATTER/
      |- [数据库名]/
         |- [表名].tdf    -- 字段名文件 (空格分隔)
         |- [表名].tic    -- 字段类型文件 (空格分隔)
         |- [表名].tid    -- 约束文件 (每行一个约束)

         
   COMMONDATA 目录结构：
   |- [数据库名]/
      |- [表名].trd       -- 表数据文件
*/

class FileManager {
private:
    const std::string METADATA_ROOT = "DATA/METADATA/DBATTER/"; // 数据库元数据路径
    const std::string COMMON_ROOT = "DATA/COMMONDATA/";

    // 写入二进制文件的模板函数，path是文件路径+名
    template<typename T>
    int write_to_file(const std::string& path, const T& data) {
        std::ofstream file(path, std::ios::binary|| std::ios::in | std::ios::out);
        if (!file) return false;
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        return file.good();
    }

    //创建二进制文件的函数，path是文件路径+名
    //0:失败
    //1:成功
    //-1:文件已存在
    int write_to_file(const std::string& path);

public:
    // 增强的表创建接口
    bool create_table(const std::string& dbName,
        const std::string& tableName,
        const std::vector<std::string>& fields,
        const std::map<std::string, int>& constraints);
    // 删除表结构
    bool delete_table(const std::string& dbName, const std::string& tableName);

    // 创建文件夹
    bool create_directory(const std::string& path);
    // 删除文件
    bool delete_file(const std::string& path);
    
    
};

