
#pragma once
#include <string>
#include "document.hpp"

class Collection {
private:
    std::string name;
    JsonValue fields;

    const JsonValue* getField(int index) const;

public:
    Collection();
    Collection(const std::string& n);

    void setName(const std::string& n);
    std::string getName() const;

    void addField(const std::string& fieldName, const std::string& typeName);

    int getFieldCount() const;
    std::string getFieldName(int index) const;
    std::string getFieldType(int index) const;

    bool setFields(const JsonValue* source);
    const JsonValue* getFields() const;
};

class Schema {
private:
    std::string name;
    int tuples_limit;

    Collection** collections;
    int collection_count;

    void clear();

public:
    Schema();
    ~Schema();

    Schema(const Schema&) = delete;
    Schema& operator=(const Schema&) = delete;

    std::string getName() const {return name;}
    int getTuplesLimit() const {return tuples_limit;}
    int getCollectionCount() const {return collection_count;}

    Collection* getCollection(int index) const;
    Collection* findCollection(const std::string& collName) const;

    bool loadFromFile(const std::string& filename);
    void createDirectories();
};
