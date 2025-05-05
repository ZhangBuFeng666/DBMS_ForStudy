#include "Storage.h" // 通常你的 FileManager.h 文件可能命名为 Storage.h，这里保持一致
#include "Tools.h" // 如果 Tools.h 中有需要的功能，请包含

// 确保命名空间被使用
using namespace std;
namespace fs = std::filesystem; // 文件系统命名空间别名

// === 构造函数 ===
FileManager::FileManager() {
    // 在构造时确保所有根目录存在 (更安全的做法是在应用启动时做一次)
    // create_directory("DATA/"); // 如果 DATA/ 不存在，也需要创建
    // create_directory(METADATA_ROOT);
    // create_directory(COMMON_ROOT);
    // create_directory(METADATA_USER_ROOT);
}


// === 私有函数实现 ===

// 创建（空的）二进制文件 (当前未使用，保留)
int FileManager::write_to_file(const std::string& path) {
    std::ifstream file0(path);
    if (file0.good()) {
        file0.close(); // 关闭文件
        return -1; // 文件已存在
    }
    file0.close(); // 确保文件已关闭

    std::ofstream file(path, std::ios::binary); // 以二进制模式创建
    if (!file.is_open()) {
        cerr << "错误: 无法创建文件: " << path << endl;
        return 0; // 创建失败
    }
    file.close(); // 立即关闭，创建一个空文件
    return 1; // 创建成功
}

// === 公共函数实现 ===

// 创建新表，包括定义文件(.tdf), 类型文件(.tic), 约束文件(.tid) 和数据文件(.trd)
bool FileManager::create_table(const std::string& dbName,
    const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& fieldsWithType,
    const std::map<std::string, int>& constraints) {
    // 构建元数据和数据文件的完整路径
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";
    std::string tableMetaPath = dbMetaPath + tableName + "/"; // 表的元数据存放在独立的子目录
    std::string tableDataPath = COMMON_ROOT + tableName + ".trd"; // 数据文件直接放在 COMMON_ROOT 下

    cout << "调试: 创建表路径信息 -" << endl;
    cout << "  元数据目录: " << tableMetaPath << endl;
    cout << "  数据文件:   " << tableDataPath << endl;

    // 1. 创建数据库和表的元数据目录
    if (!create_directory(tableMetaPath)) { // create_directory 会创建所有父目录
        cerr << "错误: 创建表元数据目录失败: " << tableMetaPath << endl;
        return false;
    }
    // 确保通用数据目录也存在 (虽然 create_directory 应该会创建，但显式检查一次更保险)
    if (!create_directory(COMMON_ROOT)) {
        cerr << "错误: 创建通用数据目录失败: " << COMMON_ROOT << endl;
        return false;
    }

    cout << "目录结构创建成功。" << endl;

    // 2. 检查表是否已存在 (通过检查主要的元数据文件，例如 .tdf)
    std::string tdfFilePath = tableMetaPath + tableName + ".tdf";
    if (fs::exists(tdfFilePath)) {
        cerr << "错误: 表 '" << tableName << "' 在数据库 '" << dbName << "' 中已存在。" << endl;
        return false;
    }

    // 3. 创建并打开元数据文件和数据文件
    ofstream tdfFile(tdfFilePath);                     // 字段名文件
    ofstream ticFile(tableMetaPath + tableName + ".tic"); // 字段类型文件
    ofstream tidFile(tableMetaPath + tableName + ".tid"); // 约束文件
    ofstream trdFile(tableDataPath);                   // 数据文件 (初始为空)

    // 检查文件是否成功打开
    if (!tdfFile.is_open()) { cerr << "错误: 无法打开 .tdf 文件: " << tdfFilePath << endl; return false; }
    if (!ticFile.is_open()) { cerr << "错误: 无法打开 .tic 文件: " << tableMetaPath + tableName + ".tic" << endl; tdfFile.close(); return false; }
    if (!tidFile.is_open()) { cerr << "错误: 无法打开 .tid 文件: " << tableMetaPath + tableName + ".tid" << endl; tdfFile.close(); ticFile.close(); return false; }
    if (!trdFile.is_open()) { cerr << "错误: 无法打开 .trd 文件: " << tableDataPath << endl; tdfFile.close(); ticFile.close(); tidFile.close(); return false; }

    cout << "元数据和数据文件已打开。" << endl;

    // 4. 写入数据到元数据文件
    try {
        // 写入字段名到 .tdf (每行一个字段名)
        cout << "调试: 写入 .tdf 文件..." << endl;
        for (const auto& field : fieldsWithType) {
            cout << "  写入字段名: " << field.first << endl;
            tdfFile << field.first << '\n';
            if (tdfFile.fail()) throw runtime_error(".tdf 文件写入失败");
        }

        // 写入字段类型到 .tic (每行一个字段类型)
        cout << "调试: 写入 .tic 文件..." << endl;
        for (const auto& field : fieldsWithType) {
            // 规范化类型字符串，去除内部空格
            string fieldType = regex_replace(field.second, regex(R"(\s+)"), "");
            cout << "  写入字段类型: " << fieldType << endl;
            ticFile << fieldType << '\n';
            if (ticFile.fail()) throw runtime_error(".tic 文件写入失败");
        }

        // 写入约束到 .tid (例如 "primary_key 0")
        cout << "调试: 写入 .tid 文件..." << endl;
        for (const auto& [type, pos] : constraints) {
            cout << "  写入约束: " << type << " " << pos << endl;
            tidFile << type << " " << pos << '\n';
            if (tidFile.fail()) throw runtime_error(".tid 文件写入失败");
        }
    }
    catch (const exception& e) {
        cerr << "错误: 写入表元数据时发生异常: " << e.what() << endl;
        // 出现错误时，尝试关闭所有文件并清理已创建的文件 (复杂，这里只关闭)
        tdfFile.close(); ticFile.close(); tidFile.close(); trdFile.close();
        // 理想情况下，应该删除刚刚创建的文件，回滚操作
        // delete_table(dbName, tableName); // 简单回滚尝试，但可能不完全
        return false;
    }

    // 5. 显式关闭所有文件 (流析构时也会自动关闭，但显式关闭更清晰)
    tdfFile.close();
    ticFile.close();
    tidFile.close();
    trdFile.close();

    // 检查文件关闭后的状态 (可选)
    if (!tdfFile.good() || !ticFile.good() || !tidFile.good() || !trdFile.good()) {
        cerr << "警告: 部分元数据或数据文件写入后状态异常。" << endl;
        // 即使警告也可能算成功，取决于需求
    }


    cout << "表 '" << tableName << "' 创建成功。" << endl;
    return true;
}


// 删除指定数据库中的表结构及其数据
bool FileManager::delete_table(const std::string& dbName, const std::string& tableName) {
    std::string dbMetaPath = METADATA_ROOT + dbName + "/";
    std::string tableMetaPath = dbMetaPath + tableName + "/"; // 表的元数据目录
    std::string tableDataPath = COMMON_ROOT + tableName + ".trd"; // 数据文件路径

    cout << "调试: 准备删除表 '" << tableName << "'" << endl;
    cout << "  元数据目录: " << tableMetaPath << endl;
    cout << "  数据文件:   " << tableDataPath << endl;

    bool success = true; // 跟踪是否所有删除都成功 (或至少不因文件不存在而失败)

    // 1. 删除所有关联的元数据文件
    if (!delete_file(tableMetaPath + tableName + ".tdf")) {
        cerr << "警告: 删除 .tdf 文件失败或文件不存在。" << endl;
        // 根据需要决定这里是否返回 false 或只警告
        // success = false;
    }
    else { cout << "调试: .tdf 已删除。" << endl; }

    if (!delete_file(tableMetaPath + tableName + ".tic")) {
        cerr << "警告: 删除 .tic 文件失败或文件不存在。" << endl;
        // success = false;
    }
    else { cout << "调试: .tic 已删除。" << endl; }

    if (!delete_file(tableMetaPath + tableName + ".tid")) {
        cerr << "警告: 删除 .tid 文件失败或文件不存在。" << endl;
        // success = false;
    }
    else { cout << "调试: .tid 已删除。" << endl; }


    // 2. 删除数据文件
    if (!delete_file(tableDataPath)) {
        cerr << "警告: 删除 .trd 文件失败或文件不存在。" << endl;
        // success = false;
    }
    else { cout << "调试: .trd 已删除。" << endl; }


    // 3. 删除表的元数据目录 (如果存在且为空)
    error_code ec_rmdir;
    fs::remove(tableMetaPath, ec_rmdir); // 使用 C++17 filesystem 删除目录
    if (ec_rmdir) {
        // 如果目录不存在或者不为空，这里会报错。只在目录不存在或为空且删除失败时视为警告。
        if (ec_rmdir != errc::no_such_file_or_directory && ec_rmdir != errc::directory_not_empty) {
            cerr << "警告: 删除表元数据目录失败: " << ec_rmdir.message() << endl;
            // success = false;
        }
        else {
            cout << "调试: 表元数据目录已删除或不存在/不为空。" << endl;
        }
    }
    else {
        cout << "调试: 表元数据目录已删除。" << endl;
    }


    // 4. TODO: 更新数据库级别的元数据 (例如表列表)，如果你的系统有这种全局管理

    if (success) {
        cout << "表 '" << tableName << "' 已成功删除。" << endl;
    }
    else {
        cerr << "错误: 删除表 '" << tableName << "' 时发生部分错误。" << endl;
    }
    return success; // 如果任何一个文件删除失败 (且不是因为不存在)，可能返回 false
}


// 创建文件夹，如果父目录不存在则一并创建 (使用 Windows API 实现)
bool FileManager::create_directory(const string& path) {
#ifdef _WIN32
    char normalizedPath[MAX_PATH];
    // 规范化路径，处理相对路径、斜杠等
    if (!PathCanonicalizeA(normalizedPath, path.c_str())) {
        cerr << "错误: 规范化目录路径失败: " << path << endl;
        // 无法规范化路径，视为失败
        // throw runtime_error("Invalid directory path: " + path); // 可以抛出异常
        return false;
    }

    string pathStr = normalizedPath;
    // 确保路径以斜杠或反斜杠结尾，便于处理子路径
    if (pathStr.back() != '\\' && pathStr.back() != '/') {
        pathStr += '\\';
    }

    size_t startPos = 0;
    // 找到第一个路径分隔符 (跳过可能的盘符，如 C:\)
    size_t driveColon = pathStr.find(':');
    if (driveColon != string::npos && driveColon + 1 < pathStr.length() && (pathStr[driveColon + 1] == '\\' || pathStr[driveColon + 1] == '/')) {
        startPos = driveColon + 2; // 从盘符后的路径开始
    }
    else if (pathStr.rfind("\\\\", 0) == 0 || pathStr.rfind("//", 0) == 0) { // 处理 UNC 路径 \\server\share
        // 查找第三个斜杠作为起点
        size_t firstSlash = pathStr.find_first_of("\\/", 2);
        if (firstSlash != string::npos) {
            size_t secondSlash = pathStr.find_first_of("\\/", firstSlash + 1);
            if (secondSlash != string::npos) {
                startPos = secondSlash + 1;
            }
        }
        if (startPos == 0) startPos = pathStr.find_first_not_of("\\/", 0); // 如果是 \\share 之类的简单 UNC
        if (startPos == string::npos) startPos = 0; // 如果只有 \\ 或 //
    }
    else {
        startPos = pathStr.find_first_not_of("\\/", 0); // 非 UNC，非盘符开头
        if (startPos == string::npos) startPos = 0;
    }


    while (startPos < pathStr.length()) {
        // 查找下一个路径分隔符
        size_t endPos = pathStr.find_first_of("\\/", startPos);
        if (endPos == string::npos) {
            endPos = pathStr.length(); // 最后一个组件
        }

        string subPath = pathStr.substr(0, endPos); // 当前要创建的子路径

        // 跳过根目录或盘符本身 (如 "C:" 或 "/")
        if (subPath.empty() || subPath == "\\" || subPath == "/" || (subPath.length() == 2 && subPath[1] == ':')) {
            startPos = endPos + 1;
            continue;
        }
        // 跳过 UNC 路径的前两部分 (如 "\\server" 或 "//server")
        if (pathStr.rfind("\\\\", 0) == 0 || pathStr.rfind("//", 0) == 0) {
            size_t firstSlash = pathStr.find_first_of("\\/", 2);
            if (firstSlash != string::npos && endPos <= firstSlash) {
                startPos = endPos + 1; continue;
            }
            size_t secondSlash = pathStr.find_first_of("\\/", (firstSlash != string::npos ? firstSlash + 1 : 2));
            if (secondSlash != string::npos && endPos <= secondSlash) {
                startPos = endPos + 1; continue;
            }
        }


        // 尝试创建当前子目录
        if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
            DWORD error = GetLastError();
            // 如果目录已存在，这是正常情况，继续
            if (error != ERROR_ALREADY_EXISTS && error != ERROR_SUCCESS) {
                cerr << "错误: 创建目录失败: " << subPath << ", 错误码: " << error << endl;
                // throw runtime_error("Failed to create directory: " + subPath + ", error code: " + to_string(error)); // 可以抛出异常
                return false; // 创建失败
            }
        }
        else {
            // cout << "调试: 目录创建成功: " << subPath << endl; // 调试信息
        }

        startPos = endPos + 1; // 移动到下一个路径分隔符之后
    }
    return true; // 所有组件都已创建或已存在
#else
    // 非 Windows 环境，使用 C++17 filesystem
    error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        cerr << "错误: 创建目录失败 (std::filesystem): " << path << ", 错误: " << ec.message() << endl;
        return false;
    }
    return true;
#endif
}


// 删除文件 (使用 Windows API 实现，尝试解除只读属性)
bool FileManager::delete_file(const string& path) {
#ifdef _WIN32
    // 获取文件属性
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        // 如果文件不存在，则认为删除（即“不存在”）是成功的
        return (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND);
    }

    // 如果文件是只读属性，尝试解除
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY)) {
            cerr << "警告: 无法移除文件只读属性: " << path << ", 错误码: " << GetLastError() << endl;
            // 即使无法移除只读，仍尝试删除，可能成功
        }
    }

    // 执行删除操作
    BOOL success = DeleteFileA(path.c_str());
    if (!success) {
        DWORD error = GetLastError();
        // 如果删除失败，且不是因为文件不存在，则视为错误
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            cerr << "错误: 删除文件失败: " << path << ", 错误码: " << error << endl;
            return false; // 删除失败
        }
        // 如果文件在检查属性后消失了 (可能被其他进程删除)，也算成功
        return true;
    }

    return true; // 删除成功
#else
    // 非 Windows 环境，使用 C++17 filesystem
    error_code ec;
    // remove 函数会删除文件或空目录
    fs::remove(path, ec);
    if (ec) {
        // 如果文件不存在，则认为成功
        if (ec == errc::no_such_file_or_directory) return true;
        cerr << "错误: 删除文件失败 (std::filesystem): " << path << ", 错误: " << ec.message() << endl;
        return false;
    }
    return true; // 删除成功
#endif
}