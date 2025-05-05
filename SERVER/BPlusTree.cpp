#include "Indexes.h"

// --- 辅助函数实现 ---
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
    throw std::runtime_error("Column not indexed");
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

std::vector<std::string> rangeQuery(
    const std::string& table,
    const std::string& column,
    const IndexKey& start,
    const IndexKey& end,
    const std::string& columnType) {
    std::string indexPath = findIndexPath(table, column);
    BPlusTree tree;
    if (!tree.deserializeFromFile(indexPath)) {
        std::cerr << "无法加载索引\n";
        return {};
    }

    std::vector<long> positions = tree.rangeSearch(start, end, columnType);
    std::ifstream dataFile("DATA/COMMONDATA/" + table + ".trd");
    std::vector<std::string> records;

    for (long pos : positions) {
        dataFile.seekg(pos);
        std::string record;
        if (std::getline(dataFile, record))
            records.push_back(record);
    }
    return records;
}

// --- 功能函数实现 ---

void create_index(const std::string& table_name, const std::string& column_name, const std::string& index_name) {
    if (isIndexExist(table_name, column_name, index_name)) {
        std::cerr << "Error: Index already exists\n";
        return;
    }

    TableProcessor processor(table_name, column_name);
    auto indexTree = processor.buildIndex();
    std::string column_type = processor.getColumnType();

    std::ofstream metaFile("DATA/INDEX/index_meta.csv", std::ios::app);
    if (!metaFile) {
        std::cerr << "Error: Cannot open metadata file\n";
        return;
    }
    metaFile << index_name << "," << table_name << "," << column_name << "," << column_type << "\n";
    metaFile.close();

    std::string filename = "DATA/INDEX/" + index_name + ".bpt";
    if (!indexTree->serializeToFile(filename)) {
        std::cerr << "Failed to serialize index\n";
        return;
    }
    std::cout << "Index created successfully. Type: " << column_type << "\n";
}

std::vector<std::string> rangeQueryAuto(
    const std::string& table,
    const std::string& column,
    const std::string& startInput,
    const std::string& endInput) {
    IndexKey start = createIndexKey(table, column, startInput);
    IndexKey end = createIndexKey(table, column, endInput);
    std::string columnType = getColumnType(table, column);
    return rangeQuery(table, column, start, end, columnType);
}
