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
public:
    explicit Storage(Schema* dbSchema);
    bool insertOne(const std::string& collectionName, const std::string& key, const JsonValue* document);
};
