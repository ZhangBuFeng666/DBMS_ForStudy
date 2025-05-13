#pragma once

#include<Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <mutex>
#include <fileapi.h>
#include <queue>

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


// ================== 数据类型定义 ==================
enum class IndexType { INTEGER, STRING };

struct IndexKey {
    std::string key_str;
    std::string key_type;

    bool operator<(const IndexKey& other) const {
        if (isStringType(key_type)) {
            return key_str < other.key_str;
        }
        else {
            return std::stoi(key_str) < std::stoi(other.key_str);
        }
    }
    bool operator>(const IndexKey& other) const {
        if (isStringType(key_type)) {
            return key_str > other.key_str;
        }
        else {
            return std::stoi(key_str) > std::stoi(other.key_str);
        }
    }
    bool operator==(const IndexKey& other) const {
        return key_str == other.key_str;
    }
};

std::ostream& operator<<(std::ostream& os, const IndexKey& key) {
    os << key.key_str;
    return os;
}

// ================== 类型辅助函数 ==================

IndexType mapColumnType(const std::string& dbType) {
    if (isIntegerType(dbType)) return IndexType::INTEGER;
    else if (isStringType(dbType)) return IndexType::STRING;
    throw std::runtime_error("Unsupported column type: " + dbType);
}

// ================== B+树节点定义 ==================
class BPlusNode {
public:
    bool isLeaf;
    std::vector<IndexKey> keys;
    BPlusNode* parent;
    BPlusNode(bool leaf = false) : isLeaf(leaf), parent(nullptr) {}
    virtual ~BPlusNode() = default;
};

class BPlusLeafNode : public BPlusNode {
public:
    std::vector<long> values;
    BPlusLeafNode* next;
    BPlusLeafNode() : BPlusNode(true), next(nullptr) {}
};

class BPlusInternalNode : public BPlusNode {
public:
    std::vector<BPlusNode*> children;
    BPlusInternalNode() : BPlusNode(false) {}
    ~BPlusInternalNode() {
        for (auto child : children) {
            if (child != nullptr) {
                delete child;
            }
        }
    }
};

// ================== B+树类 ==================
class BPlusTree {
private:
    int order;
    BPlusNode* root;

    BPlusLeafNode* findLeaf(IndexKey key) const;
    size_t binarySearch(const std::vector<IndexKey>& arr, IndexKey key) const;
    void splitLeaf(BPlusLeafNode* leaf);
    void splitInternal(BPlusInternalNode* node);

    // 序列化/反序列化辅助函数
    bool serializeValue(std::ostream& out, long value) const;
    bool deserializeValue(std::istream& in, long& value) const;
    bool serializeIndexKey(std::ostream& out, const IndexKey& key) const;
    bool deserializeIndexKey(std::istream& in, IndexKey& key) const;

    void rebuildLeafLinkedList();

public:
    BPlusTree(int order = 500) : order(order), root(new BPlusLeafNode()) {}
    ~BPlusTree();

    BPlusLeafNode* getLeftmostLeaf() const {
        if (!root) return nullptr;
        BPlusNode* node = root;
        while (!node->isLeaf) {
            node = static_cast<BPlusInternalNode*>(node)->children[0];
        }
        return static_cast<BPlusLeafNode*>(node);
    }
    void insert(IndexKey key, long value);
    void print() const;
    std::vector<long> rangeSearch(const IndexKey& start, const IndexKey& end, const std::string& columnType) const;

    bool serialize(std::ostream& out) const;
    bool deserialize(std::istream& in);
    bool serializeToFile(const std::string& filename) const;
    bool deserializeFromFile(const std::string& filename);

};

// ================== 表处理器 ==================
class TableProcessor {
private:
    std::string tableName;
    std::string columnName;
    std::string columnsFilePath;
    std::string typesFilePath;
    std::string dataFilePath;
    int targetColumnIndex = -1;
    std::string targetColumnType;

    bool fileExists(const std::string& filename);
    bool loadColumnNames();
    bool loadDataTypes();
    std::vector<std::string> splitString(const std::string& s, char delimiter);

public:
    TableProcessor(const std::string& table, const std::string& column);
    std::unique_ptr<BPlusTree> buildIndex();
    std::string getColumnType() const;
};

// ================== 索引管理函数 ==================
bool isIndexExist(const std::string& table, const std::string& column, const std::string& indexName);
void create_index(const std::string& table_name, const std::string& column_name, const std::string& index_name);
std::string findIndexPath(const std::string& table, const std::string& column);
std::string getColumnType(const std::string& table, const std::string& column);
IndexKey createIndexKey(const std::string& table, const std::string& column, const std::string& input);
std::vector<long> rangeQuery(
    const std::string& table,
    const std::string& column,
    const IndexKey& start,
    const IndexKey& end,
    const std::string& columnType);
std::vector<long> rangeQueryAuto(
    const std::string& table,
    const std::string& column,
    const std::string& startInput,
    const std::string& endInput);

// ================== 函数实现 ==================

// --- BPlusTree 实现 ---
BPlusTree::~BPlusTree() {
    if (!root) return;
    std::queue<BPlusNode*> nodes;
    nodes.push(root);
    while (!nodes.empty()) {
        BPlusNode* current = nodes.front();
        nodes.pop();
        if (!current->isLeaf) {
            auto* internalNode = static_cast<BPlusInternalNode*>(current);
            for (BPlusNode* child : internalNode->children) {
                if (child) nodes.push(child); // 仅处理非空指针
            }
        }
        delete current;
    }
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
    std::vector<long> results;
    // 情况0：start和end均为"*"，返回整棵树的所有值
    if (start.key_str == "*" && end.key_str == "*") {
        BPlusLeafNode* leaf = getLeftmostLeaf();
        while (leaf) {
            results.insert(results.end(), leaf->values.begin(), leaf->values.end());
            leaf = leaf->next;
        }
        return results;
    }

    // 情况1：start为"*"（从头查到end）
    else if (start.key_str == "*") {
        BPlusLeafNode* leaf = getLeftmostLeaf();
        if (!leaf) return results;  // 空树

        size_t idx = 0;
        while (leaf) {
            for (; idx < leaf->keys.size(); ++idx) {
                if (leaf->keys[idx] > end) return results;  // 超过end则终止
                results.push_back(leaf->values[idx]);
            }
            leaf = leaf->next;
            idx = 0;
        }
        return results;
    }

    // 情况2：end为"*"（从start查到结尾）
    else if (end.key_str == "*") {
        BPlusLeafNode* leaf = findLeaf(start);
        if (!leaf) return results;  // start超出范围

        size_t idx = binarySearch(leaf->keys, start);
        while (leaf) {
            for (; idx < leaf->keys.size(); ++idx) {
                results.push_back(leaf->values[idx]);
            }
            leaf = leaf->next;
            idx = 0;
        }
        return results;
    }

    // 情况3：普通范围查询（无通配符）
    else {
        BPlusLeafNode* leaf = findLeaf(start);
        if (!leaf) return results;  // start超出范围

        size_t idx = binarySearch(leaf->keys, start);
        while (leaf) {
            for (; idx < leaf->keys.size(); ++idx) {
                if (leaf->keys[idx] > end) return results;  // 超过end则终止
                results.push_back(leaf->values[idx]);
            }
            leaf = leaf->next;
            idx = 0;
        }

        return results;
    }
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

    long long recordPosition = 0;
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

class IndexManager {
private:
    static std::unordered_map<std::string, std::shared_ptr<BPlusTree>> indexCache;
    static std::mutex cacheMutex;

public:

    // 检查索引是否存在（内存或磁盘）
    static bool isIndexExist(const std::string& indexPath) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        // 1. 检查内存缓存
        if (indexCache.find(indexPath) != indexCache.end()) {
            return true;
        }

        // 2. 检查磁盘文件
        std::ifstream f(indexPath);
        return f.good(); // 使用之前定义的fileExists函数
    }
    // 核心方法：获取或反序列化索引
    static std::shared_ptr<BPlusTree> getOrDeserializeIndex(const std::string& indexPath) {
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

    // 强制重新从文件加载
    static bool reloadIndex(const std::string& indexPath) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        auto tree = std::make_shared<BPlusTree>();
        if (!tree->deserializeFromFile(indexPath)) {
            return false;
        }

        indexCache[indexPath] = tree;
        return true;
    }

    static void addToCache(const std::string& indexPath, std::shared_ptr<BPlusTree> tree) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        indexCache[indexPath] = tree;
    }

    static void removeFromCache(const std::string& indexPath) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = indexCache.find(indexPath);
        if (it != indexCache.end()) {
            indexCache.erase(it);
        }
    }
};