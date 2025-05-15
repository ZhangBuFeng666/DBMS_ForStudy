#pragma once

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

// ================== 数据类型定义 ==================
enum class IndexType { INTEGER, STRING };

struct IndexKey {
    std::string key_str;

    bool operator<(const IndexKey& other) const {
        return key_str < other.key_str;
    }
    bool operator>(const IndexKey& other) const {
        return key_str > other.key_str;
    }
    bool operator==(const IndexKey& other) const {
        return key_str == other.key_str;
    }
};

std::ostream& operator<<(std::ostream& os, const IndexKey& key);

// ================== 类型辅助函数 ==================
bool isIntegerType(const std::string& columnType);

bool isStringType(const std::string& columnType);

IndexType mapColumnType(const std::string& dbType);
// ================== B+树节点定义 ==================
class BPlusNode {
friend class BPlusTree;
private:
    bool isLeaf;
    std::vector<IndexKey> keys;
    BPlusNode* parent;
public:
    
    BPlusNode(bool leaf = false) : isLeaf(leaf), parent(nullptr) {}
    virtual ~BPlusNode() = default;
};

class BPlusLeafNode : public BPlusNode {
friend class BPlusTree;
private:
    std::vector<long> values;
    BPlusLeafNode* next;
public:
    
    BPlusLeafNode() : BPlusNode(true), next(nullptr) {}
};

class BPlusInternalNode : public BPlusNode {
    friend class BPlusTree;
private:
    std::vector<BPlusNode*> children;

public:
    BPlusInternalNode() : BPlusNode(false) {}
    ~BPlusInternalNode() {
        for (auto child : children) delete child;
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

// --- 索引管理器实现 ---

class IndexManager {
private:
    static std::unordered_map<std::string, std::shared_ptr<BPlusTree>> indexCache;
    static std::mutex cacheMutex;

public:
    // 检查索引是否存在（内存或磁盘）
    static bool isIndexExist(const std::string& indexPath);
    // 核心方法：获取或反序列化索引
    static std::shared_ptr<BPlusTree> getOrDeserializeIndex(const std::string& indexPath);
    // 强制重新从文件加载
    static bool reloadIndex(const std::string& indexPath);
    //添加索引
    static void addToCache(const std::string& indexPath, std::shared_ptr<BPlusTree> tree);
    //删除索引
    static void removeFromCache(const std::string& indexPath);
    // 检查索引是否存在（内存或磁盘）
    // 核心方法：获取或反序列化索引
    // 强制重新从文件加载
};

// ================== 索引管理函数 ==================
bool isIndexExist(const std::string& table, const std::string& column, const std::string& indexName);
bool create_index(const std::string& table_name, const std::string& column_name, const std::string& index_name);
std::string findIndexPath(const std::string& table, const std::string& column);
std::string getColumnType(const std::string& table, const std::string& column);
IndexKey createIndexKey(const std::string& table, const std::string& column, const std::string& input);
std::string findIndexName(const std::string& table, const std::string& column);
bool drop_index(const std::string& index_name);
void update_index(const std::string& table_name, const std::string& column_name);
std::vector<long> Merge_index(const std::vector<long>& result1, const std::vector<long>& result2);
std::vector<std::string> LongtoString(const std::vector<long>& position, const std::string& table_name);
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

