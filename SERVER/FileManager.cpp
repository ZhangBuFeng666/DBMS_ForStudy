#include "Storage.h"
#include"Tools.h"
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

using namespace std;
//使用ASCII编码

int FileManager::write_to_file(const std::string& path) {
    std::ifstream file0(path);
    if (file0.good()) {
        return -1;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file) return 0;
    return file.good();
}

bool FileManager::create_table(const std::string& dbName, const std::string& tableName) {
    // 1. 创建表描述文件
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";
    create_directory(dbMetaPath);

    // 2. 创建数据文件
    std::string dbDataPath = COMMON_ROOT + dbName + "/";
    create_directory(dbDataPath);

    TableBlock tb{
        const_cast<char*>(tableName.c_str()),
        0,
        0,
        const_cast<char*>(dbMetaPath.c_str()),
        const_cast<char*>(dbDataPath.c_str()),
        tools::Timer::getCurrentTime(),
        tools::Timer::getCurrentTime(),
    };

    // 初始化空文件
    std::ofstream(dbMetaPath + tableName + ".tdf").close();
    std::ofstream(dbMetaPath + tableName + ".tic").close();
    std::ofstream(dbMetaPath + tableName + ".tid").close();

    std::ofstream(dbDataPath + tableName + ".trd").close();


    // 3. 更新数据库元数据
    return update_databaseCatalog(dbName, tb);
}

// 删除表结构
bool FileManager::delete_table(const std::string& dbName, const std::string& tableName) {
    std::string dbDataPath = COMMON_ROOT + dbName + "/";
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";

    // 删除所有关联文件
    delete_file(dbMetaPath + tableName + ".tdf");
    delete_file(dbMetaPath + tableName + ".tic");
    delete_file(dbMetaPath + tableName + ".tid");

    delete_file(dbDataPath + tableName + ".trd");


    // 更新数据库元数据
    return remove_from_catalog(dbName, tableName);
}
bool FileManager::create_directory(const string& path) {
    // 规范化路径处理
    char normalizedPath[MAX_PATH];
    PathCanonicalizeA(normalizedPath, path.c_str());

    // 检查路径是否为空
    if (normalizedPath[0] == '\0') {
        throw runtime_error("Invalid directory path: " + path);
    }

    // 将路径转化为可操作的字符串
    string pathStr = normalizedPath;

    // 使用迭代来逐级创建目录
    size_t startPos = 0;
    while (startPos < pathStr.length()) {
        // 获取当前目录的路径
        size_t endPos = pathStr.find('\\/', startPos);
        if (endPos == string::npos) {
            endPos = pathStr.length();
        }

        // 获取当前目录部分
        string subPath = pathStr.substr(0, endPos);

        // 尝试创建目录
        if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                // 如果目录不存在并且创建失败，抛出异常
                if (error == ERROR_PATH_NOT_FOUND) {
                    // 父目录不存在时，递归创建父目录
                    if (startPos > 0) {
                        string parentDir = pathStr.substr(0, pathStr.find_last_of('\\/', startPos));
                        create_directory(parentDir); // 递归创建父目录
                    }
                }
                throw runtime_error("Failed to create directory: " + subPath);
            }
        }

        // 更新 startPos 以继续处理下一个目录部分
        startPos = endPos + 1;
    }

    return true;
}

#include <AccCtrl.h>
#include <Aclapi.h>

bool FileManager::delete_file(const string& path) {
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

#include <WinBase.h>
#include <fileapi.h>
#include <algorithm>
#include <vector>

bool FileManager::update_databaseCatalog(const string& dbName, const TableBlock& tb) {
    const string catalogPath = METADATA_ROOT + dbName + ".db";

    // 创建文件内存映射
    HANDLE hFile = CreateFileA(catalogPath.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 文件锁定（防止并发修改）
    OVERLAPPED offset = { 0 };
    LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &offset);

    // 内存映射处理
    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    LPVOID pData = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    // 解析元数据结构
    DWORD fileSize = GetFileSize(hFile, nullptr);
    const int BLOCK_SIZE = sizeof(TableBlock);
    vector<TableBlock> blocks(fileSize / BLOCK_SIZE);
    memcpy(blocks.data(), pData, fileSize);

    // 查找目标表记录并更新
    auto it = find_if(blocks.begin(), blocks.end(),
        [&](const TableBlock& tbItem) {
            return strncmp(tbItem.name, tb.name, sizeof(TableBlock::name)) == 0;
        });

    // 如果找到了要更新的表，进行更新
    if (it != blocks.end()) {
        *it = tb;  // 更新记录

        // 写入临时文件
        const string tempPath = catalogPath + ".tmp";
        HANDLE hTemp = CreateFileA(tempPath.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        WriteFile(hTemp, blocks.data(), blocks.size() * BLOCK_SIZE, &written, nullptr);
        CloseHandle(hTemp);

        // 文件替换操作
        ReplaceFileA(catalogPath.c_str(), tempPath.c_str(), nullptr,
            REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    }

    // 清理资源
    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &offset);
    CloseHandle(hFile);

    // 如果成功找到并更新表，返回 true
    return it != blocks.end();
}

bool FileManager::remove_from_catalog(const string& dbName, const string& tableName) {
    const string catalogPath = METADATA_ROOT + dbName + ".db";

    // 创建文件内存映射
    HANDLE hFile = CreateFileA(catalogPath.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 文件锁定（防止并发修改）
    OVERLAPPED offset = { 0 };
    LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &offset);

    // 内存映射处理
    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    LPVOID pData = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    // 解析元数据结构
    DWORD fileSize = GetFileSize(hFile, nullptr);
    const int BLOCK_SIZE = sizeof(TableBlock);
    vector<TableBlock> blocks(fileSize / BLOCK_SIZE);
    memcpy(blocks.data(), pData, fileSize);

    // 查找目标表记录
    auto it = find_if(blocks.begin(), blocks.end(),
        [&](const TableBlock& tb) {
            return strncmp(tb.name, tableName.c_str(),
                sizeof(TableBlock::name)) == 0;
        });

    // 原子化更新操作
    if (it != blocks.end()) {
        // 创建临时副本
        vector<TableBlock> newBlocks;
        newBlocks.reserve(blocks.size() - 1);
        copy_if(blocks.begin(), blocks.end(),
            back_inserter(newBlocks),
            [&](const TableBlock& tb) { return &tb != &*it; });

        // 写入临时文件
        const string tempPath = catalogPath + ".tmp";
        HANDLE hTemp = CreateFileA(tempPath.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        WriteFile(hTemp, newBlocks.data(),
            newBlocks.size() * BLOCK_SIZE, &written, nullptr);
        CloseHandle(hTemp);

        // 文件替换操作
        ReplaceFileA(catalogPath.c_str(), tempPath.c_str(), nullptr,
            REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    }

    // 清理资源
    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &offset);
    CloseHandle(hFile);

    return it != blocks.end();
}
