#pragma once
#include <fstream>

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

#pragma pack(push, 1) // 1字节对齐
struct TableBlock {
    char* name;
    int32_t record_num;
    int32_t field_num;
    char* tdf_path;//表格定义文件路径
    char* trd_path;//表格记录文件路径
    std::string crtime;
    std::string mtime;
};

struct FieldBlock {
    int32_t order;
    char name[128];
    int32_t type;
    int32_t param;
    std::string mtime;
    int32_t integrities; // 位掩码表示约束
};

#pragma pack(pop)

class FileManager {
private:
    const std::string METADATA_ROOT = "DATA/METADATA/";
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
    // 创建表结构
    bool create_table(const std::string& dbName, const std::string& tableName);
    // 删除表结构
    bool delete_table(const std::string& dbName, const std::string& tableName);

    // 创建文件夹
    bool create_directory(const std::string& path);
    // 删除文件
    bool delete_file(const std::string& path);
    //更新元数据
    bool update_databaseCatalog(const std::string& dbName, const TableBlock& tb);
    //删除元数据
    bool remove_from_catalog(const std::string& dbName,const std::string& tableName);
};

