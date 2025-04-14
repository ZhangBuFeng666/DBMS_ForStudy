#include "Storage.h"
#include"Tools.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <regex>
#pragma comment(lib, "Shlwapi.lib")


using namespace std;

//使用ASCII编码

int FileManager::write_to_file(const std::string& path) 
{
	using namespace std;

    std::ifstream file0(path);
    if (file0.good()) {
        return -1;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file) return 0;
    return file.good();
}

bool FileManager::create_table(const std::string& dbName, const std::string& tableName,
    const vector<string>& fields, const map<string, int>& constraints) {
    using namespace std;


    // 1. 创建表描述文件
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";
    create_directory(dbMetaPath);

    // 2. 创建数据文件
    std::string dbDataPath = COMMON_ROOT + dbName + "/";
    create_directory(dbDataPath);

    // 初始化空文件
    std::ofstream(dbMetaPath + tableName + ".tdf").close();
    std::ofstream(dbMetaPath + tableName + ".tic").close();
    std::ofstream(dbMetaPath + tableName + ".tid").close();

    std::ofstream(dbDataPath + tableName + ".trd").close();

    // 写入.tdf文件（字段名）
    ofstream tdfFile(dbMetaPath + tableName + ".tdf");
    for (const auto& field : fields) tdfFile << field << " ";

    // 写入.tic文件（字段类型）
    ofstream ticFile(dbMetaPath + tableName + ".tic");
    regex typeExtract(R"((\w+)\s*\w*)"); // 提取类型部分
    for (const auto& field : fields) {
        smatch m;
        regex_search(field, m, typeExtract);
        ticFile << m[1] << " ";
    }

    // 写入.tid文件（约束）
    ofstream tidFile(dbMetaPath + tableName + ".tid");
    for (const auto& [type, pos] : constraints) {
        tidFile << type << " " << pos << "\n";
    }


    return TRUE;
}

// 删除表结构
bool FileManager::delete_table(const std::string& dbName, const std::string& tableName) {

    using namespace std;


    std::string dbDataPath = COMMON_ROOT + dbName + "/";
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";

    // 删除所有关联文件
    delete_file(dbMetaPath + tableName + ".tdf");
    delete_file(dbMetaPath + tableName + ".tic");
    delete_file(dbMetaPath + tableName + ".tid");

    delete_file(dbDataPath + tableName + ".trd");


    // 更新数据库元数据
    return TRUE;
}
bool FileManager::create_directory(const string& path) {

    using namespace std;


    char normalizedPath[MAX_PATH];
    PathCanonicalizeA(normalizedPath, path.c_str());

    if (normalizedPath[0] == '\0') {
        throw runtime_error("Invalid directory path: " + path);
    }

    string pathStr = normalizedPath;
    size_t startPos = 0;
    while (startPos < pathStr.length()) {
        size_t endPos = pathStr.find_first_of("\\/", startPos); // 修复分隔符匹配逻辑
        if (endPos == string::npos) {
            endPos = pathStr.length();
        }

        string subPath = pathStr.substr(0, endPos);

        if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                throw runtime_error("Failed to create directory: " + subPath);
            }
        }

        startPos = endPos + 1;
    }
    return true;
}

#include <AccCtrl.h>
#include <Aclapi.h>

bool FileManager::delete_file(const string& path) {

    using namespace std;

    // 获取文件属性
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return (GetLastError() == ERROR_FILE_NOT_FOUND); // 文件不存在返回true
    }

    // 解除只读属性
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }

    // 设置完全控制权限
    PACL oldDacl = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    GetNamedSecurityInfoA(path.c_str(), SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION, nullptr, nullptr,
        &oldDacl, nullptr, &sd);

    EXPLICIT_ACCESS_A ea = { 0 }; // 使用 A 版本的 EXPLICIT_ACCESS
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName = const_cast<char*>("CURRENT_USER"); // 使用 ANSI 字符串

    PACL newDacl = nullptr;
    SetEntriesInAclA(1, &ea, oldDacl, &newDacl);
    SetNamedSecurityInfoA(const_cast<char*>(path.c_str()), SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION, nullptr, nullptr,
        newDacl, nullptr);

    // 执行删除操作
    BOOL success = DeleteFileA(path.c_str());
    LocalFree(newDacl);
    LocalFree(sd);

    return success;
}