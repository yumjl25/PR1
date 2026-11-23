#pragma once
#include <string>
#include "schema.hpp"
#include "document.hpp"

enum KeySearchResult {
    KEY_NOT_FOUND,
    KEY_FOUND,
    KEY_ERROR
};

class Storage{
private:
    Schema* schema;

    std::string getCollectionPath(const std::string& collectionName) const;
    std::string getFilePath(const std::string& collectionName, int fileNumber) const;

    bool fileExists(const std::string& filename) const;
    bool validateDocument(const Collection* collection, const JsonValue* document) const;
    KeySearchResult keyExists(const std::string& collectionName, const std::string& key) const;
    bool writeFile(const std::string& filename, const std::string& content) const;
    bool matchesFilter(const JsonValue* document, const JsonValue* filter) const;
    int findInternal(const std::string& collectionName, const JsonValue* filter, int limit, const List* projection) const;
    bool compareValues(const JsonValue* first, const JsonValue* second, const std::string& op) const;
    bool matchesCondition(const JsonValue* field, const JsonValue* condition) const;
    bool matchesLogical(const JsonValue* document, const JsonValue* condition) const;
    void printDocument(const JsonValue* document, const List* projection) const;
    JsonValue* storageLoadFile(const std::string& filename) const;
    bool aplyUpdate(JsonValue* document, const JsonValue* update) const;
    bool aplySet(JsonValue* document, const JsonValue* fields) const;
    bool aplyInc(JsonValue* document, const JsonValue* fields) const;
    bool aplyPush(JsonValue* document, const JsonValue* fields) const;

public:
    explicit Storage(Schema* dbSchema);
    bool insertOne(const std::string& collectionName, const std::string& key, const JsonValue* document);
    int insertMany(const std::string& collectionName, const JsonValue* documents);
    int find(const std::string& collectionName, const JsonValue* filter, int limit = 0, const List* projection = nullptr) const;
    bool findOne(const std::string& collectionName, const JsonValue* filter) const;
    bool updateOne(const std::string& collectionName, const JsonValue* filter, const JsonValue* update);
};
