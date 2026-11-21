#pragma once
#include <string>
#include "schema.hpp"
#include "document.hpp"

class Storage{
private:
    Schema* schema;

    std::string getCollectionPath(const std::string& collectionName) const;
    std::string getFilePath(const std::string& collectionName, int fileNumber) const;

    bool fileExists(const std::string& filename) const;
    bool validateDocument(const Collection* collection, const JsonValue* document) const;
    bool keyExists(const std::string& collectionName, const std::string& key) const;
public:
    explicit Storage(Schema* dbSchema);
    bool insertOne(const std::string& collectionName, const std::string& key, const JsonValue* document);
};
