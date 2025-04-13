#include "SQLInterface.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include <vector>
#include <sstream>
#include <regex>
#include "SQLInterface.h"  

// Ensure the constants are defined and accessible in this file  
const string METADATA_USER_ROOT = "DATA/METADATA/USERATTER/"; // 用户元数据路径  
const string METADATA_DB_ROOT = "DATA/METADATA/DBATTER/";     // 数据库元数据路径  
const string COMMONDATA_ROOT = "DATA/COMMONDATA/";  

// The rest of the code remains unchanged
using namespace std;

bool SQLInterface::create_user(const string& username, const string& password, const string& privilege) {
    // 确保用户目录存在
    if (!fileManager.create_directory(METADATA_USER_ROOT)) {
        return false;
    }

    const string headerPath = METADATA_USER_ROOT + "user_header.txt";
    const string dataPath = METADATA_USER_ROOT + "user_data.txt";

    // 创建/检查表头文件
    ifstream headerFile(headerPath);
    if (!headerFile.good()) {
        ofstream newHeader(headerPath);
        if (!newHeader) return false;
        newHeader << "用户名 密码 权限\n";
        if (!newHeader.good()) return false;
    }
    headerFile.close();

    // 检查用户是否存在
    ifstream dataFile(dataPath);
    string line;
    while (getline(dataFile, line)) {
        istringstream iss(line);
        string existingUser;
        if (iss >> existingUser && existingUser == username) {
            dataFile.close();
            return false; // 用户已存在
        }
    }
    dataFile.close();

    // 追加新用户信息
    ofstream outFile(dataPath, ios::app);
    if (!outFile) return false;
    outFile << username << " " << password << " " << privilege << "\n";
    return outFile.good();
}

bool SQLInterface::create_database(const string& username) {
    string dbMetaPath = METADATA_DB_ROOT + username + "/";
    string dbDataPath = COMMONDATA_ROOT + username + "/";
    return fileManager.create_directory(dbMetaPath) && fileManager.create_directory(dbDataPath);
}

bool SQLInterface::create_table(const std::string& dbName, const std::string& tableName,
    const vector<string>& fields, const map<string, int>& constraints) {
    return fileManager.create_table(dbName, tableName, fields, constraints);
}

bool SQLInterface::drop_table(const string& dbName, const string& tableName) {
    return fileManager.delete_table(dbName, tableName);
}

// 辅助函数：校验字段类型是否合法
bool validateFieldType(const string& typeStr) {
    static const regex typeRegex(R"((CHAR\(\d+\)|INT|DATE|BOOL))");
    return regex_match(typeStr, typeRegex);
}

// 辅助函数：解析字段定义
vector<pair<string, string>> parseFieldDefinitions(const string& defPath) {
    vector<pair<string, string>> fields;
    ifstream defFile(defPath);
    string line;

    while (getline(defFile, line)) {
        istringstream iss(line);
        string fieldName, typeAndRest;
        if (iss >> fieldName >> typeAndRest) {
            // 提取数据类型部分（可能包含CHAR(20)等）
            size_t paren = typeAndRest.find('(');
            string type = (paren != string::npos) ?
                typeAndRest.substr(0, paren + 1) :  // 保留CHAR( 用于正则校验
                typeAndRest;

            fields.emplace_back(fieldName, typeAndRest);
        }
    }
    return fields;
}

// 修改后的字段添加函数（新增类型校验）
bool SQLInterface::alter_table_add_field(const string& dbName,
    const string& tableName,
    const string& fieldDefinition) {
    // 解析字段定义
    istringstream iss(fieldDefinition);
    string fieldName, fieldType;
    if (!(iss >> fieldName >> fieldType)) return false;

    // 校验字段类型
    if (!validateFieldType(fieldType)) {
        cerr << "Invalid field type: " << fieldType << endl;
        return false;
    }

    string defPath = METADATA_DB_ROOT + dbName + "/" + tableName + ".tdf";
    ofstream defFile(defPath, ios::app);
    return defFile.is_open() && (defFile << fieldDefinition << "\n");
}

// 新增辅助函数：分割逗号分隔的数据
vector<string> split_values(const string& input) {
    vector<string> result;
    regex valueRegex(R"((?:'[^']*')|(?:[^,]+))");
    auto begin = sregex_iterator(input.begin(), input.end(), valueRegex);
    for (auto it = begin; it != sregex_iterator(); ++it) {
        string val = it->str();
        if (val.front() == '\'' && val.back() == '\'') {
            val = val.substr(1, val.size() - 2);
        }
        result.push_back(val);
    }
    return result;
}

// 新增辅助函数：解析字段类型
vector<string> parseFieldTypes(const string& dbName, const string& tableName) {
    vector<string> types;
    string ticPath = METADATA_DB_ROOT + dbName + "/" + tableName + ".tic";
    ifstream ticFile(ticPath);
    string line;

    if (getline(ticFile, line)) {
        istringstream iss(line);
        string type;
        while (iss >> type) {
            types.push_back(type);
        }
    }
    return types;
}

// 新增辅助函数：校验值类型
bool validateValueType(const string& type, const string& value) {
    // 处理CHAR类型
    if (type.find("CHAR") != string::npos) {
        size_t open = type.find('(');
        size_t close = type.find(')');
        if (open != string::npos && close != string::npos) {
            int len = stoi(type.substr(open + 1, close - open - 1));
            return value.length() <= len;
        }
        return true;
    }
    // 处理INT类型
    else if (type == "INT") {
        return regex_match(value, regex(R"(^\-?\d+$)"));
    }
    // 处理DATE类型
    else if (type == "DATE") {
        return regex_match(value, regex(R"(^\d{4}-\d{2}-\d{2}$)"));
    }
    // 处理BOOL类型
    else if (type == "BOOL") {
        return value == "true" || value == "false";
    }
    return false;
}




string SQLInterface::grant_privilege_sql(const string& username, const string& privilegeType) {
    return "GRANT " + privilegeType + " TO " + username + ";";
}

bool SQLInterface::insert_into_table(const string& dbName, const string& tableName, const string& rowData) {
    string dataPath = COMMONDATA_ROOT + dbName + "/" + tableName + ".trd";

    // 调试输出
    cout << "[DEBUG] 数据文件路径: " << dataPath << endl;

    // 检查元数据文件
    string ticPath = METADATA_DB_ROOT + dbName + "/" + tableName + ".tic";
    if (!filesystem::exists(ticPath)) {
        cerr << "错误：字段类型文件不存在 (" << ticPath << ")" << endl;
        return false;
    }

    // 获取字段类型
    vector<string> fieldTypes = parseFieldTypes(dbName, tableName);
    cout << "[DEBUG] 加载到 " << fieldTypes.size() << " 个字段类型" << endl;

    if (fieldTypes.empty()) {
        cerr << "错误：未解析到任何字段类型" << endl;
        return false;
    }

    // 分割输入数据
    vector<string> values = split_values(rowData);
    cout << "[DEBUG] 解析到 " << values.size() << " 个值" << endl;

    // 字段数量校验
    if (values.size() != fieldTypes.size()) {
        cerr << "错误：字段数量不匹配 (表定义 " << fieldTypes.size()
            << "，输入数据 " << values.size() << ")" << endl;
        return false;
    }

    // 类型校验
    for (size_t i = 0; i < values.size(); ++i) {
        cout << "[DEBUG] 校验字段 " << i + 1 << "/" << fieldTypes.size()
            << " 类型=" << fieldTypes[i] << " 值=" << values[i] << endl;

        if (!validateValueType(fieldTypes[i], values[i])) {
            cerr << "类型校验失败：字段" << i + 1
                << " 预期类型 " << fieldTypes[i]
                << " 实际值 '" << values[i] << "'" << endl;
                return false;
        }
    }

    // 写入数据
    ofstream dataFile(dataPath, ios::app);
    if (!dataFile) {
        cerr << "错误：无法打开数据文件" << endl;
        return false;
    }
    dataFile << rowData << "\n";

    cout << "[DEBUG] 数据写入成功" << endl;
    return true;
}

// 修改后的update_table_row实现
bool SQLInterface::update_table_row(const string& dbName, const string& tableName,
    int rowIndex, const string& newRow) {
    string dataPath = COMMONDATA_ROOT + dbName + "/" + tableName + ".trd";

    // 获取字段类型
    vector<string> fieldTypes = parseFieldTypes(dbName, tableName);
    if (fieldTypes.empty()) return false;

    // 分割输入数据
    vector<string> values = split_values(newRow);

    // 校验字段数量
    if (values.size() != fieldTypes.size()) {
        cerr << "Field count mismatch during update" << endl;
        return false;
    }

    // 类型校验
    for (size_t i = 0; i < values.size(); ++i) {
        if (!validateValueType(fieldTypes[i], values[i])) {
            cerr << "Invalid type during update: " << values[i]
                << " for type " << fieldTypes[i] << endl;
                return false;
        }
    }

    // 原有更新逻辑
    ifstream inFile(dataPath);
    vector<string> lines;
    string line;
    while (getline(inFile, line)) lines.push_back(line);
    inFile.close();

    if (rowIndex < 0 || rowIndex >= lines.size()) return false;
    lines[rowIndex] = newRow;

    ofstream outFile(dataPath);
    for (const auto& l : lines) outFile << l << "\n";
    return true;
}

bool SQLInterface::delete_table_row(const string& dbName, const string& tableName, int rowIndex) {
    string dataPath = COMMONDATA_ROOT + dbName + "/" + tableName + ".trd";
    ifstream inFile(dataPath);
    vector<string> lines;
    string line;
    while (getline(inFile, line)) lines.push_back(line);
    inFile.close();

    if (rowIndex < 0 || rowIndex >= lines.size()) return false;
    lines.erase(lines.begin() + rowIndex);

    ofstream outFile(dataPath);
    for (const auto& l : lines) outFile << l << "\n";
    return true;
}
