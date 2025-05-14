#include "Indexes.h"
#include <unordered_set>

// 关键：静态成员变量必须在类外定义！
std::unordered_map<std::string, std::shared_ptr<BPlusTree>> IndexManager::indexCache;
std::mutex IndexManager::cacheMutex;

std::ostream& operator<<(std::ostream& os, const IndexKey& key) {
    os << key.key_str;
    return os;
}
// ================== 类型辅助函数 ==================
bool isIntegerType(const std::string& columnType) {
    std::string typeUpper = columnType;
    std::transform(typeUpper.begin(), typeUpper.end(), typeUpper.begin(), ::toupper);
    return (typeUpper.find("INT") != std::string::npos ||
        typeUpper == "NUMBER" ||
        typeUpper == "LONG");
}

bool isStringType(const std::string& columnType) {
    std::string typeUpper = columnType;
    std::transform(typeUpper.begin(), typeUpper.end(), typeUpper.begin(), ::toupper);
    return (typeUpper.find("CHAR") == 0 ||
        typeUpper.find("VARCHAR") == 0 ||
        typeUpper.find("DATE") == 0 ||
        typeUpper.find("STRING") == 0);
}

IndexType mapColumnType(const std::string& dbType) {
    if (isIntegerType(dbType)) return IndexType::INTEGER;
    else if (isStringType(dbType)) return IndexType::STRING;
    throw std::runtime_error("Unsupported column type: " + dbType);
}

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

/*

不含索引的查询

*/

std::vector<long>TraverseQuery(
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







// ================== 函数实现 ==================

// --- BPlusTree 实现 ---
BPlusTree::~BPlusTree() {
    auto destroy = [](BPlusNode* node) {
        std::function<void(BPlusNode*)> del = [&](BPlusNode* n) {
            if (!n) return;
            if (!n->isLeaf) {
                for (auto c : static_cast<BPlusInternalNode*>(n)->children)
                    del(c);
            }
            delete n;
            };
        del(node);
        };
    destroy(root);
}

BPlusLeafNode* BPlusTree::findLeaf(IndexKey key) const {
    BPlusNode* node = root;
    while (!node->isLeaf) {
        auto internal = static_cast<BPlusInternalNode*>(node);
        auto it = std::upper_bound(internal->keys.begin(), internal->keys.end(), key);
        size_t idx = it - internal->keys.begin();
        node = internal->children[idx];
    }
    return static_cast<BPlusLeafNode*>(node);
}

size_t BPlusTree::binarySearch(const std::vector<IndexKey>& arr, IndexKey key) const {
    return std::lower_bound(arr.begin(), arr.end(), key) - arr.begin();
}

void BPlusTree::splitLeaf(BPlusLeafNode* leaf) {
    auto newLeaf = new BPlusLeafNode();
    size_t splitPos = leaf->keys.size() / 2;
    newLeaf->keys.assign(leaf->keys.begin() + splitPos, leaf->keys.end());
    newLeaf->values.assign(leaf->values.begin() + splitPos, leaf->values.end());
    leaf->keys.resize(splitPos);
    leaf->values.resize(splitPos);
    newLeaf->next = leaf->next;
    leaf->next = newLeaf;

    if (!leaf->parent) {
        auto newRoot = new BPlusInternalNode();
        newRoot->keys.push_back(newLeaf->keys[0]);
        newRoot->children.push_back(leaf);
        newRoot->children.push_back(newLeaf);
        leaf->parent = newRoot;
        newLeaf->parent = newRoot;
        root = newRoot;
    }
    else {
        auto parent = static_cast<BPlusInternalNode*>(leaf->parent);
        IndexKey newKey = newLeaf->keys[0];
        auto pos = binarySearch(parent->keys, newKey);
        parent->keys.insert(parent->keys.begin() + pos, newKey);
        parent->children.insert(parent->children.begin() + pos + 1, newLeaf);
        newLeaf->parent = parent;

        if (parent->keys.size() > order)
            splitInternal(parent);
    }
}

void BPlusTree::splitInternal(BPlusInternalNode* node) {
    auto newNode = new BPlusInternalNode();
    size_t splitPos = node->keys.size() / 2;
    IndexKey promoteKey = node->keys[splitPos];
    newNode->keys.assign(node->keys.begin() + splitPos + 1, node->keys.end());
    newNode->children.assign(node->children.begin() + splitPos + 1, node->children.end());
    node->keys.resize(splitPos);
    node->children.resize(splitPos + 1);

    for (auto child : newNode->children)
        child->parent = newNode;

    if (!node->parent) {
        auto newRoot = new BPlusInternalNode();
        newRoot->keys.push_back(promoteKey);
        newRoot->children.push_back(node);
        newRoot->children.push_back(newNode);
        node->parent = newRoot;
        newNode->parent = newRoot;
        root = newRoot;
    }
    else {
        auto parent = static_cast<BPlusInternalNode*>(node->parent);
        auto pos = binarySearch(parent->keys, promoteKey);
        parent->keys.insert(parent->keys.begin() + pos, promoteKey);
        parent->children.insert(parent->children.begin() + pos + 1, newNode);
        newNode->parent = parent;

        if (parent->keys.size() > order)
            splitInternal(parent);
    }
}

void BPlusTree::insert(IndexKey key, long value) {
    BPlusLeafNode* leaf = findLeaf(key);
    size_t pos = binarySearch(leaf->keys, key);
    leaf->keys.insert(leaf->keys.begin() + pos, key);
    leaf->values.insert(leaf->values.begin() + pos, value);
    if (leaf->keys.size() > order)
        splitLeaf(leaf);
}

void BPlusTree::print() const {
    std::function<void(BPlusNode*, int)> printNode = [&](BPlusNode* node, int level) {
        std::cout << std::string(level * 2, ' ');
        if (node->isLeaf) {
            auto l = static_cast<BPlusLeafNode*>(node);
            std::cout << "Leaf [";
            for (size_t i = 0; i < l->keys.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << l->keys[i] << ":" << l->values[i];
            }
            std::cout << "]" << std::endl;
        }
        else {
            auto i = static_cast<BPlusInternalNode*>(node);
            std::cout << "Internal [";
            for (size_t j = 0; j < i->keys.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << i->keys[j];
            }
            std::cout << "]" << std::endl;
            for (auto child : i->children)
                printNode(child, level + 1);
        }
        };
    printNode(root, 0);
}

void BPlusTree::rebuildLeafLinkedList() {
    BPlusLeafNode* prev = nullptr;
    std::function<void(BPlusNode*)> link = [&](BPlusNode* node) {
        if (!node) return;
        if (node->isLeaf) {
            auto leaf = static_cast<BPlusLeafNode*>(node);
            if (prev) prev->next = leaf;
            prev = leaf;
        }
        else {
            for (auto child : static_cast<BPlusInternalNode*>(node)->children)
                link(child);
        }
        };
    link(root);
}

std::vector<long> BPlusTree::rangeSearch(const IndexKey& start, const IndexKey& end, const std::string& columnType) const {
    BPlusLeafNode* leaf = findLeaf(start);
    std::vector<long> results;
    size_t idx = binarySearch(leaf->keys, start);

    while (leaf) {
        for (; idx < leaf->keys.size(); ++idx) {
            const IndexKey& key = leaf->keys[idx];
            if (key > end) break;
            results.push_back(leaf->values[idx]);
        }
        leaf = leaf->next;
        idx = 0;
    }

    return results;
}

// --- 文件序列化与反序列化 ---
bool BPlusTree::serializeValue(std::ostream& out, long value) const {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    return out.good();
}

bool BPlusTree::deserializeValue(std::istream& in, long& value) const {
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return in.good();
}

bool BPlusTree::serializeIndexKey(std::ostream& out, const IndexKey& key) const {
    uint32_t len = key.key_str.size();
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(key.key_str.data(), len);
    return out.good();
}

bool BPlusTree::deserializeIndexKey(std::istream& in, IndexKey& key) const {
    uint32_t len;
    if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
    key.key_str.resize(len);
    in.read(&key.key_str[0], len);
    return in.good();
}

bool BPlusTree::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&order), sizeof(order));
    std::function<void(BPlusNode*)> writeNode = [&](BPlusNode* node) {
        bool isLeaf = node->isLeaf;
        out.write(reinterpret_cast<const char*>(&isLeaf), sizeof(isLeaf));
        uint32_t keyCount = node->keys.size();
        out.write(reinterpret_cast<const char*>(&keyCount), sizeof(keyCount));
        for (auto& k : node->keys)
            serializeIndexKey(out, k);
        if (isLeaf) {
            auto l = static_cast<BPlusLeafNode*>(node);
            for (auto v : l->values)
                serializeValue(out, v);
        }
        else {
            auto i = static_cast<BPlusInternalNode*>(node);
            for (auto c : i->children)
                writeNode(c);
        }
        };
    writeNode(root);
    return true;
}

bool BPlusTree::deserialize(std::istream& in) {
    if (!in.read(reinterpret_cast<char*>(&order), sizeof(order)))
        return false;

    std::function<BPlusNode* ()> readNode = [&]() -> BPlusNode* {
        bool isLeaf;
        if (!in.read(reinterpret_cast<char*>(&isLeaf), sizeof(isLeaf)))
            return nullptr;

        uint32_t keyCount;
        if (!in.read(reinterpret_cast<char*>(&keyCount), sizeof(keyCount)))
            return nullptr;

        BPlusNode* node = isLeaf
            ? static_cast<BPlusNode*>(new BPlusLeafNode())
            : static_cast<BPlusNode*>(new BPlusInternalNode());
        node->keys.resize(keyCount);

        for (auto& k : node->keys)
            if (!deserializeIndexKey(in, k))
                return nullptr;

        if (isLeaf) {
            auto l = static_cast<BPlusLeafNode*>(node);
            l->values.resize(keyCount);
            for (auto& v : l->values)
                if (!deserializeValue(in, v))
                    return nullptr;
        }
        else {
            auto i = static_cast<BPlusInternalNode*>(node);
            i->children.resize(keyCount + 1);
            for (int j = 0; j <= static_cast<int>(keyCount); ++j) {
                BPlusNode* child = readNode();
                if (!child) return nullptr;
                child->parent = i;
                i->children[j] = child;
            }
        }

        return node;
        };

    root = readNode();
    rebuildLeafLinkedList();
    return root != nullptr;
}

bool BPlusTree::serializeToFile(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    return out && serialize(out);
}

bool BPlusTree::deserializeFromFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    return in && deserialize(in);
}

// --- TableProcessor 实现 ---
TableProcessor::TableProcessor(const std::string& table, const std::string& column)
    : tableName(table), columnName(column) {
    columnsFilePath = "DATA/METADATA/DBATTER/" + tableName + "/" + tableName + ".tdf";
    typesFilePath = "DATA/METADATA/DBATTER/" + tableName + "/" + tableName + ".tic";
    dataFilePath = "DATA/COMMONDATA/" + tableName + ".trd";
}

bool TableProcessor::fileExists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

bool TableProcessor::loadColumnNames() {
    std::ifstream file(columnsFilePath);
    std::string name;
    int index = 0;
    while (std::getline(file, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name == columnName) {
            targetColumnIndex = index;
            return true;
        }
        ++index;
    }
    return false;
}

bool TableProcessor::loadDataTypes() {
    std::ifstream file(typesFilePath);
    std::string type;
    int index = 0;
    while (std::getline(file, type)) {
        if (!type.empty() && type.back() == '\r') type.pop_back();
        if (index == targetColumnIndex) {
            targetColumnType = type;
            return true;
        }
        ++index;
    }
    return false;
}

std::vector<std::string> TableProcessor::splitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

std::unique_ptr<BPlusTree> TableProcessor::buildIndex() {
    auto tree = std::make_unique<BPlusTree>();
    if (!loadColumnNames() || !loadDataTypes())
        return tree;

    std::ifstream dataFile(dataFilePath);
    if (!dataFile.is_open()) return tree;

    long recordPosition = 0;
    std::string line;
    int lineNumber = 0;

    while (true) {
        recordPosition = dataFile.tellg();

        if (!std::getline(dataFile, line)) break;

        ++lineNumber;

        // 忽略空行
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;
        std::vector<std::string> values = splitString(line, ',');
        IndexKey key;
        key.key_str = values[targetColumnIndex];
        tree->insert(key, recordPosition);
    }
    return tree;
}

std::string TableProcessor::getColumnType() const {
    return targetColumnType;
}
// --- IndexManager 实现 ---

 bool IndexManager::isIndexExist(const std::string& indexPath) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    // 1. 检查内存缓存
    if (indexCache.find(indexPath) != indexCache.end()) {
        return true;
    }

    // 2. 检查磁盘文件
    std::ifstream f(indexPath);
    return f.good(); // 使用之前定义的fileExists函数
}

 std::shared_ptr<BPlusTree> IndexManager::getOrDeserializeIndex(const std::string& indexPath) {
     std::lock_guard<std::mutex> lock(cacheMutex);

     // 1. 检查内存缓存
     auto it = indexCache.find(indexPath);
     if (it != indexCache.end()) {
         return it->second;
     }

     // 2. 检查文件是否存在
     std::ifstream f(indexPath);
     if (!f.good()) {
         std::cerr << "Index file not found: " << indexPath << std::endl;
         return nullptr;
     }

     // 3. 从文件反序列化重建B+树
     auto tree = std::make_shared<BPlusTree>();
     if (tree->deserializeFromFile(indexPath)) {
         indexCache[indexPath] = tree;
         std::cout << "Successfully deserialized index: " << indexPath << std::endl;
         return tree;
     }

     std::cerr << "Failed to deserialize index: " << indexPath << std::endl;
     return nullptr;
 }

 bool IndexManager::reloadIndex(const std::string& indexPath) {
     std::lock_guard<std::mutex> lock(cacheMutex);

     auto tree = std::make_shared<BPlusTree>();
     if (!tree->deserializeFromFile(indexPath)) {
         return false;
     }

     indexCache[indexPath] = tree;
     return true;
 }

void IndexManager::addToCache(const std::string& indexPath, std::shared_ptr<BPlusTree> tree) {
     std::lock_guard<std::mutex> lock(cacheMutex);
     indexCache[indexPath] = tree;
 }

void IndexManager::removeFromCache(const std::string& indexPath) {
     std::lock_guard<std::mutex> lock(cacheMutex);
     auto it = indexCache.find(indexPath);
     if (it != indexCache.end()) {
         indexCache.erase(it);
     }
 }

