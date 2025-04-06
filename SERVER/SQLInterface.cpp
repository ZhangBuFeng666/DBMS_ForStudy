#include "SQLInterface.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include <vector>
#include <sstream>
#include <regex>

using namespace std;

bool SQLInterface::create_user(const string& username, const string& password, const string& privilege) {
    string userMetaPath = METADATA_USER_ROOT + username + "/";
    if (!fileManager.create_directory(userMetaPath)) return false;

    ofstream defFile(userMetaPath + "definition.txt");
    if (!defFile) return false;
    defFile << "Username: " << username << "\n";
    defFile << "Password: " << password << "\n";
    defFile << "Privilege: " << privilege << "\n";
    defFile.close();

    ofstream recordFile(userMetaPath + "record.txt");
    return recordFile.good();
}

bool SQLInterface::create_database(const string& username) {
    string dbMetaPath = METADATA_DB_ROOT + username + "/";
    string dbDataPath = COMMONDATA_ROOT + username + "/";
    return fileManager.create_directory(dbMetaPath) && fileManager.create_directory(dbDataPath);
}

bool SQLInterface::create_table(const string& dbName, const string& tableName) {
    return fileManager.create_table(dbName, tableName);
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

string SQLInterface::grant_privilege_sql(const string& username, const string& privilegeType) {
    return "GRANT " + privilegeType + " TO " + username + ";";
}

bool SQLInterface::insert_into_table(const string& dbName, const string& tableName, const string& rowData) {
    string dataPath = COMMONDATA_ROOT + dbName + "/" + tableName + ".trd";
    ofstream dataFile(dataPath, ios::app);
    if (!dataFile) return false;
    dataFile << rowData << "\n";
    return true;
}

bool SQLInterface::update_table_row(const string& dbName, const string& tableName, int rowIndex, const string& newRow) {
    string dataPath = COMMONDATA_ROOT + dbName + "/" + tableName + ".trd";
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
