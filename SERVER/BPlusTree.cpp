#include"Indexes.h"
#include "SQLInterface.h"
#include <iomanip>
#include <unordered_set>
#include <chrono>

// ? 关键：静态成员变量必须在类外定义！
std::unordered_map<std::string, std::shared_ptr<BPlusTree>> IndexManager::indexCache;
std::mutex IndexManager::cacheMutex;

// --- 辅助函数实现 ---
std::vector<std::string> SplitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}


bool isIndexExist(const std::string& table, const std::string& column, const std::string& indexName) {
    std::ifstream metaFile("DATA/INDEX/index_meta.csv");
    std::string line;
    while (std::getline(metaFile, line)) {
        std::istringstream iss(line);
        std::string idxName, tblName, colName;
        if (std::getline(iss, idxName, ',') && std::getline(iss, tblName, ',') && std::getline(iss, colName, ',')) {
            if (tblName == table && colName == column && idxName == indexName)
                return true;
        }
    }
    return false;
}

std::string findIndexPath(const std::string& table, const std::string& column) {
    std::ifstream metaFile("DATA/INDEX/index_meta.csv");
    std::string line;
    while (std::getline(metaFile, line)) {
        std::istringstream iss(line);
        std::string idxName, tblName, colName;
        if (std::getline(iss, idxName, ',') && std::getline(iss, tblName, ',') && std::getline(iss, colName, ',')) {
            if (tblName == table && colName == column)
                return "DATA/INDEX/" + idxName + ".bpt";
        }
    }
    return "";
}

std::string getColumnType(const std::string& table, const std::string& column) {
    std::ifstream metaFile("DATA/INDEX/index_meta.csv");
    std::string line;
    while (std::getline(metaFile, line)) {
        std::istringstream iss(line);
        std::string idxName, tblName, colName, colType;
        if (std::getline(iss, idxName, ',') && std::getline(iss, tblName, ',') &&
            std::getline(iss, colName, ',') && std::getline(iss, colType)) {
            if (tblName == table && colName == column)
                return colType;
        }
    }

    // 如果没找到返回空字符串
    return "";
}

IndexKey createIndexKey(const std::string& table, const std::string& column, const std::string& input) {
    std::string columnType = getColumnType(table, column);
    IndexKey key;
    if (isIntegerType(columnType)) {
        try {
            key.key_str = std::to_string(std::stoll(input));
        }
        catch (...) {
            key.key_str = input;
        }
    }
    else {
        key.key_str = input;
    }
    return key;
}

std::vector<long> rangeQuery(
    const std::string& table,
    const std::string& column,
    const IndexKey& start,
    const IndexKey& end,
    const std::string& columnType) {
    // 获取索引路径
    std::string indexPath = findIndexPath(table, column);
    if (indexPath.empty()) {
        std::cerr << "No index defined for " << table << "." << column << std::endl;
        return {};
    }

    // 获取或反序列化索引
    auto tree = IndexManager::getOrDeserializeIndex(indexPath);
    if (!tree) {
        std::cerr << "Failed to load index: " << indexPath << std::endl;
        return {};
    }

    std::vector<long> positions = tree->rangeSearch(start, end, columnType);
    return positions;
}

std::string findIndexName(const std::string& table, const std::string& column) {
    std::ifstream metaFile("DATA/INDEX/index_meta.csv");
    std::string line;
    while (std::getline(metaFile, line)) {
        std::istringstream iss(line);
        std::string idxName, tblName, colName;
        if (std::getline(iss, idxName, ',') && std::getline(iss, tblName, ',') && std::getline(iss, colName, ',')) {
            if (tblName == table && colName == column)
                return idxName;
        }
    }
    return "";
}

// --- 功能函数实现 ---

/*

创建索引

*/
void create_index(const std::string& table_name, const std::string& column_name, const std::string& index_name) {
    // 检查索引是否已存在（文件或元数据）
    std::string indexPath = "DATA/INDEX/" + index_name + ".bpt";
    if (IndexManager::isIndexExist(indexPath)) {  // 使用 IndexManager 的文件检查
        std::cerr << "Error: Index already exists\n";
        return;
    }

    // 构建索引（返回 unique_ptr）
    TableProcessor processor(table_name, column_name);
    std::unique_ptr<BPlusTree> uniqueTree = processor.buildIndex();
    std::string column_type = processor.getColumnType();

    // 序列化到文件（使用原始指针）
    if (!uniqueTree->serializeToFile(indexPath)) {
        std::cerr << "Failed to serialize index\n";
        return;
    }

    // 转换 unique_ptr → shared_ptr 并加入缓存
    std::shared_ptr<BPlusTree> sharedTree(std::move(uniqueTree));
    IndexManager::addToCache(indexPath, sharedTree);

    // 写入元数据
    std::ofstream metaFile("DATA/INDEX/index_meta.csv", std::ios::app);
    metaFile << index_name << "," << table_name << "," << column_name << "," << column_type << "\n";
    metaFile.close();

    std::cout << "Index created and cached successfully.\n";
}
/*

索引查询

*/
std::vector <long> rangeQueryAuto(
    const std::string& table,
    const std::string& column,
    const std::string& startInput,
    const std::string& endInput) {
    IndexKey start = createIndexKey(table, column, startInput);
    IndexKey end = createIndexKey(table, column, endInput);
    std::string columnType = getColumnType(table, column);
    return rangeQuery(table, column, start, end, columnType);
}
/*

删除索引

*/
void drop_index(const std::string& index_name) {
    // 1. 从元数据文件中查找并删除记录
    std::ifstream metaIn("DATA/INDEX/index_meta.csv");
    std::vector<std::string> metaLines;
    std::string line;
    bool found = false;

    // 读取所有行，跳过要删除的索引
    while (std::getline(metaIn, line)) {
        std::istringstream iss(line);
        std::string currentIdxName;
        if (std::getline(iss, currentIdxName, ',')) {
            if (currentIdxName == index_name) {
                found = true;
                continue; // 跳过这一行
            }
        }
        metaLines.push_back(line);
    }
    metaIn.close();

    if (!found) {
        std::cerr << "Error: Index '" << index_name << "' not found\n";
        return;
    }

    // 2. 删除索引数据文件
    std::string indexPath = "DATA/INDEX/" + index_name + ".bpt";
    if (std::remove(indexPath.c_str()) != 0) {
        std::cerr << "Warning: Failed to delete index file '" << indexPath
            << "', it may not exist or is locked\n";
    }

    // 3. 从缓存中移除索引
    IndexManager::removeFromCache(indexPath);

    // 4. 重写元数据文件
    std::ofstream metaOut("DATA/INDEX/index_meta.csv", std::ios::trunc);
    for (const auto& l : metaLines) {
        metaOut << l << "\n";
    }
    metaOut.close();

    std::cout << "Index '" << index_name << "' dropped successfully\n";
}

/*

更新索引

*/
void update_index(const std::string& table_name, const std::string& column_name) {
    // 第一步：查找对应表名和列名的索引名
    std::string index_name = findIndexName(table_name, column_name);
    if (index_name.empty()) {
        std::cerr << "Error: No index found for " << table_name << "." << column_name << std::endl;
        return;
    }

    // 第二步：删除现有的索引
    drop_index(index_name);

    // 第三步：检查索引是否成功删除
    std::string indexPath = "DATA/INDEX/" + index_name + ".bpt";
    if (IndexManager::isIndexExist(indexPath)) {
        std::cerr << "Error: Failed to delete index " << index_name << std::endl;
        return;
    }

    // 第四步：重建索引
    create_index(table_name, column_name, index_name);

    // 第五步：检查索引是否成功重建
    if (!IndexManager::isIndexExist(indexPath)) {
        std::cerr << "Error: Failed to rebuild index " << index_name << std::endl;
    }
    else {
        std::cout << "Index " << index_name << " updated successfully." << std::endl;
    }
}
/*

合并查询结果

*/

std::vector<long> Merge_index(const std::vector<long>& result1, const std::vector<long>& result2)
{
    std::unordered_set<long> set2(result2.begin(), result2.end());
    std::vector<long> finalResults;
    for (long value : result1) {
        if (set2.find(value) != set2.end()) {
            finalResults.push_back(value);
        }
    }
    return finalResults;
}

/*

行偏移量转化为字符串

*/

std::vector<std::string> LongtoString(const std::vector<long>& position, const std::string& table_name) {
    std::string filepath = "DATA/COMMONDATA/" + table_name + ".trd";
    std::ifstream dataFile(filepath);
    std::vector<std::string> records;

    for (long pos : position) {
        dataFile.seekg(pos);
        std::string record;
        if (std::getline(dataFile, record))
            records.push_back(record);
    }

    return records;
}

SelectResult LongToStruct(const std::vector<long>& position, const std::string& table_name,int index) {
    std::string filepath = "DATA/COMMONDATA/" + table_name + ".trd";
    std::ifstream dataFile(filepath);
    SelectResult results;
    results.header.push_back(table_name);
    for (long pos : position) {
        dataFile.seekg(pos);
        std::string record;
        if (std::getline(dataFile, record))
            results.data[index].push_back(record);
    }
    return results;
}

/*

不含索引的查询

*/

std::vector<long> TraverseQuery(
    const std::string& table,
    const std::string& column,
    const std::string& startInput,
    const std::string& endInput) {
    std::vector<long> results;

    std::string columnsFilePath = "DATA/METADATA/DBATTER/" + table + "/" + table + ".tdf";
    std::string typesFilePath = "DATA/METADATA/DBATTER/" + table + "/" + table + ".tic";
    std::string dataFilePath = "DATA/COMMONDATA/" + table + ".trd";

    std::ifstream file(columnsFilePath);
    if (!file.is_open()) {
        std::cout << "Failed to open: " << columnsFilePath << std::endl;
        return {};
    }
    std::string name;
    int targetIndex = 0;
    while (std::getline(file, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name == column) {
            break;
        }
        ++targetIndex;
    }
    file.close();
    std::string targetColumnType;
    std::ifstream columnFile(typesFilePath);
    std::string type;
    int index = 0;
    while (std::getline(columnFile, type)) {
        if (!type.empty() && type.back() == '\r') type.pop_back();
        if (index == targetIndex) {
            targetColumnType = type;
            break;
        }
        ++index;
    }
    columnFile.close();
    std::ifstream dataFile(dataFilePath);
    if (!dataFile.is_open()) {
        std::cout << "Failed to open: " << dataFilePath << std::endl;
        return {};
    }
    long long recordPosition = 0;
    std::string line;
    if (isStringType(targetColumnType)) {
        while (true) {
            recordPosition = dataFile.tellg();

            if (!std::getline(dataFile, line)) break;


            // 忽略空行
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
                continue;
            std::vector<std::string> values = SplitString(line, ',');
            if (values[targetIndex] >= startInput && values[targetIndex] <= endInput) results.push_back(recordPosition);
        }
        dataFile.close();
        return results;
    }
    else {
        while (true) {
            recordPosition = dataFile.tellg();

            if (!std::getline(dataFile, line)) break;


            // 忽略空行
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
                continue;
            std::vector<std::string> values = SplitString(line, ',');
            int num = std::stoi(values[index]);
            if (num >= std::stoi(startInput) && num <= std::stoi(endInput)) results.push_back(recordPosition);
        }
        dataFile.close();
        return results;
    }
}
/*
int main() {
    //auto result3 = rangeQueryAuto("students", "sid", "99", "105");
    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = rangeQueryAuto("students", "sid", "99", "105");
    auto end1 = std::chrono::high_resolution_clock::now();
    long long time1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();

    auto start2 = std::chrono::high_resolution_clock::now();
    auto result2 = TraverseQuery("students", "sid", "99", "105");
    auto end2 = std::chrono::high_resolution_clock::now();
    long long time2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();

    std::cout << "方法1执行时间: " << time1 << " 纳秒" << std::endl;
    std::cout << "方法2执行时间: " << time2 << " 纳秒" << std::endl;
    std::cout << "性能差异: " << ((time1 < time2) ? "方法1更快" : "方法2更快") << std::endl;

    
    auto result1 = rangeQueryAuto("students", "sid", "99", "105");
    auto result2 = rangeQueryAuto("students", "age", "*", "20");
    auto results = Merge_index(result1, result2);
    std::cout << "Query result size: " << result1.size() << std::endl;
    std::cout << "Query result size: " << results.size() << std::endl;
    auto finalresult = LongtoString(results, "students");
    // 打印查询结果
    if (finalresult.empty()) {
        std::cout << "│ 没有找到匹配的记录"
            << std::setw(33) << " │\n";
    }
    else {
        for (size_t i = 0; i < finalresult.size(); ++i) {
            std::cout << "│ " << std::setw(2)
                << std::setw(45) << std::left
                << finalresult[i].substr(0, 45) << " │\n";
        }
    }

}
*/