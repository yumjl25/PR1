#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "list.hpp"

struct Field{
    std::string name;
    std::string type;
    Field(const std::string& n, const std::string& t) : name(n), type(t) {}
};

class Collection{
private:
    std::string name;
    List fields;
public:
    Collection() : name("") {}
    Collection(const std::string& n) : name(n) {}
    
    void setName(const std::string& n) {name = n;}
    std::string getName() const {return name;}

    void addField(const std::string& fieldName, const std::string& typeName){
        fields.pushTail(fieldName + ":" + typeName);
    }

    int getFieldCount() const {return fields.getSize();}

    std::string getFieldName(int index) const {
        std::string out;
        if (fields.getAt(index, out)){
            size_t pos = out.find(':');
            if (pos != std::string::npos){
                return out.substr(0, pos);
            }
        }
        return "";
    }

    std::string getFieldType(int index) const {
        std::string out;
        if (fields.getAt(index, out)){
            size_t pos = out.find(':');
            if (pos != std::string::npos){
                return out.substr(pos + 1);
            }
        }
        return "";
    }
};

class Schema{
private:
    std::string name;
    int tuples_limit;

    List collection_names;

    static const int MAX_COLLECTIONS = 10;
    Collection* collections[MAX_COLLECTIONS];
    int collection_count;
public:
    Schema();
    ~Schema();

    std::string getName() const {return name;}
    int getTuplesLimit() const {return tuples_limit;}
    int getCollectionCount() const {return collection_count;}

    Collection* getCollection(int index) const;
    Collection* findCollection(const std::string& collName) const;
    
    bool loadFromFile(const std::string& filename);

    void createDirectories();
};