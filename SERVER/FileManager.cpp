#include "Storage.h"
#include "Tools.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <regex>
#include <filesystem> // 引入 filesystem

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// 使用 ASCII 编码

// 创建二进制文件（未使用）
int FileManager::write_to_file(const std::string& path)
{
    using namespace std;

    std::ifstream file0(path);
    if (file0.good()) {
        return -1; // 文件已存在
    }
    std::ofstream file(path, std::ios::binary);
    if (!file) return 0; // 创建失败
    return file.good() ? 1 : 0; // 创建成功返回 1，否则返回 0
}

// 创建新表，包括定义文件、类型文件、约束文件和数据文件
bool FileManager::create_table(const std::string& dbName, const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
    const std::map<std::string, int>& constraints) {
    using namespace std;

    // 构建元数据和数据文件路径
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";
    std::string dbDataPath = COMMON_ROOT + "";
    std::string tableMetaPath = dbMetaPath + tableName + "/"; // 为表创建单独的目录
    std::string tableDataPath = dbDataPath ;

    cout << "[DEBUG] dbMetaPath: " << dbMetaPath << endl;
    cout << "[DEBUG] dbDataPath: " << dbDataPath << endl;
    cout << "[DEBUG] tableMetaPath: " << tableMetaPath << endl;
    cout << "[DEBUG] tableDataPath: " << tableDataPath << endl;

    // 创建数据库和表的目录
    if (!create_directory(dbMetaPath)) {
        cerr << "[DEBUG] Error creating dbMetaPath directory." << endl;
        return false;
    }
    if (!create_directory(dbDataPath)) {
        cerr << "[DEBUG] Error creating dbDataPath directory." << endl;
        return false;
    }
    if (!create_directory(tableMetaPath)) {
        cerr << "[DEBUG] Error creating tableMetaPath directory." << endl;
        return false;
    }

    cout << "创建目录 " << endl;

    // 检查表是否已存在
    if (filesystem::exists(tableMetaPath + tableName + ".tdf")) {
        cerr << "Error: Table '" << tableName << "' already exists in database '" << dbName << "'" << endl;
        return false;
    }

    // 初始化空文件
    ofstream tdfFile(tableMetaPath + tableName + ".tdf"); // 字段名
    ofstream ticFile(tableMetaPath + tableName + ".tic"); // 字段类型
    ofstream tidFile(tableMetaPath + tableName + ".tid"); // 约束
    ofstream trdFile(tableDataPath + tableName + ".trd"); // 数据

    // 检查文件是否成功打开
    if (!tdfFile.is_open()) {
        cerr << "[DEBUG] Error: Failed to open tdfFile: " << tableMetaPath + tableName + ".tdf" << endl;
        return false;
    }
    if (!ticFile.is_open()) {
        cerr << "[DEBUG] Error: Failed to open ticFile: " << tableMetaPath + tableName + ".tic" << endl;
        return false;
    }
    if (!tidFile.is_open()) {
        cerr << "[DEBUG] Error: Failed to open tidFile: " << tableMetaPath + tableName + ".tid" << endl;
        return false;
    }
    if (!trdFile.is_open()) {
        cerr << "[DEBUG] Error: Failed to open trdFile: " << tableDataPath + ".trd" << endl;
        return false;
    }

    // 写入数据
    try {
        // 写入字段名
        cout << "[DEBUG] Writing to .tdf" << endl;
        for (const auto& field : fieldsWithType) {
            cout << "[DEBUG] .tdf Data: " << field.first << endl; // 打印将要写入 .tdf 的数据
            tdfFile << field.first << '\n';
            if (tdfFile.fail()) throw runtime_error(".tdf write failed");
            cout << "[DEBUG] Wrote field: " << field.first << endl;
        }

        // 写入字段类型
        cout << "[DEBUG] Writing to .tic" << endl;
        for (const auto& field : fieldsWithType) {
            cout << "[DEBUG] .tic Data: " << field.second << endl; // 打印将要写入 .tic 的数据
            ticFile << field.second << '\n';
            if (ticFile.fail()) throw runtime_error(".tic write failed");
            cout << "[DEBUG] Wrote type: " << field.second << endl;
        }

        // 写入约束
        cout << "[DEBUG] Writing to .tid" << endl;
        for (const auto& [type, pos] : constraints) {
            cout << "[DEBUG] .tid Data: Type=" << type << ", Position=" << pos << endl; // 打印将要写入 .tid 的数据
            tidFile << type << " " << pos << '\n';
            if (tidFile.fail()) throw runtime_error(".tid write failed");
            cout << "[DEBUG] Wrote constraint: " << type << " " << pos << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Error writing table data: " << e.what() << endl;
        return false;
    }

    // 显式关闭文件
    tdfFile.close();
    ticFile.close();
    tidFile.close();
    trdFile.close();

    cout << "Table created successfully" << endl;


    return true;
}

// 删除指定数据库中的表结构及其数据
bool FileManager::delete_table(const std::string& dbName, const std::string& tableName) {
    using namespace std;

    std::string dbDataPath = COMMON_ROOT + dbName + "/";
    std::string dbMetaPath = METADATA_ROOT ;
    std::string tableMetaPath = dbMetaPath + tableName + "/";
    std::string tableDataPath = dbDataPath + "/";

    // 删除所有关联文件
    delete_file(tableMetaPath + tableName + ".tdf");
    delete_file(tableMetaPath + tableName + ".tic");
    delete_file(tableMetaPath + tableName + ".tid");
    delete_file(tableDataPath + tableName + ".trd");

    // 删除表目录（如果存在且为空）
    filesystem::remove(tableMetaPath);
    filesystem::remove(tableDataPath);

    // 更新数据库元数据（当前未实现）
    return true;
}

// 创建文件夹，如果父目录不存在则一并创建
bool FileManager::create_directory(const string& path) {
    using namespace std;

    char normalizedPath[MAX_PATH];
    if (!PathCanonicalizeA(normalizedPath, path.c_str())) {
        throw runtime_error("Invalid directory path: " + path);
    }

    string pathStr = normalizedPath;
    size_t startPos = 0;
    while (startPos < pathStr.length()) {
        size_t endPos = pathStr.find_first_of("\\/", startPos);
        if (endPos == string::npos) {
            endPos = pathStr.length();
        }

        string subPath = pathStr.substr(0, endPos);

        if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS && error != ERROR_SUCCESS) {
                throw runtime_error("Failed to create directory: " + subPath + ", error code: " + to_string(error));
            }
        }
        startPos = endPos + 1;
    }
    return true;
}

#include <AccCtrl.h>
#include <Aclapi.h>

// 删除文件
bool FileManager::delete_file(const string& path) {
    using namespace std;

    // 获取文件属性
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return (GetLastError() == ERROR_FILE_NOT_FOUND); // 文件不存在返回 true
    }

    // 解除只读属性
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }

    // 设置完全控制权限（可能不需要在删除前设置）
    // PACL oldDacl = nullptr;
    // PSECURITY_DESCRIPTOR sd = nullptr;
    // GetNamedSecurityInfoA(path.c_str(), SE_FILE_OBJECT,
    //     DACL_SECURITY_INFORMATION, nullptr, nullptr,
    //     &oldDacl, nullptr, &sd);

    // EXPLICIT_ACCESS_A ea = { 0 }; // 使用 A 版本的 EXPLICIT_ACCESS
    // ea.grfAccessPermissions = GENERIC_ALL;
    // ea.grfAccessMode = SET_ACCESS;
    // ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    // ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    // ea.Trustee.ptstrName = const_cast<char*>("CURRENT_USER"); // 使用 ANSI 字符串

    // PACL newDacl = nullptr;
    // SetEntriesInAclA(1, &ea, oldDacl, &newDacl);
    // SetNamedSecurityInfoA(const_cast<char*>(path.c_str()), SE_FILE_OBJECT,
    //     DACL_SECURITY_INFORMATION, nullptr, nullptr,
    //     newDacl, nullptr);

    // 执行删除操作
    BOOL success = DeleteFileA(path.c_str());
    // LocalFree(newDacl);
    // LocalFree(sd);

    return success;
}