#include "schema.hpp"
#include "jsonparser.hpp"
#include "list.hpp"
#include <sys/stat.h>
#include <cerrno>
#ifdef _WIN32
#include <direct.h>
#endif
#include <fstream>
#include <iostream>
#include <climits>

using namespace std;

Collection::Collection() : name(""){
    fields.setObject();
}

Collection::Collection(const string& n) : name(n){
    fields.setObject();
}

void Collection::setName(const string& n){
    name = n;
}

string Collection::getName() const {
    return name;
}

void Collection::addField(const string& fieldName, const string& typeName){
    JsonValue* field = new JsonValue(typeName);
    field->setKey(fieldName);
    fields.addChild(field);
}

const JsonValue* Collection::getField(int index) const{
    if (index < 0) {
        return nullptr;
    }

    const JsonValue* current = fields.getChild();

    while (current != nullptr && index > 0){
        current = current->getNext();
        index--;
    }

    return current;
}

int Collection::getFieldCount() const {
    int count = 0;

    const JsonValue* current = fields.getChild();

    while (current != nullptr) {
        count++;
        current = current->getNext();
    }

    return count;
}

string Collection::getFieldName(int index) const {
    const JsonValue* field = getField(index);

    if (field == nullptr) {
        return "";
    }

    return field->getKey();
}

string Collection::getFieldType(int index) const {
    const JsonValue* field = getField(index);

    if (field == nullptr) {
        return "";
    }

    if (field->getType() == JSON_OBJECT) {
        return "object";
    }

    return field->getValue();
}

bool Collection::setFields(const JsonValue* source){
    if (source == nullptr || source->getType() != JSON_OBJECT){
        return false;
    }

    return fields.copyFrom(source);
}

const JsonValue* Collection::getFields() const {
    return &fields;
}

Schema::Schema() : name(""), tuples_limit(0), collections(nullptr), collection_count(0) {}

Schema::~Schema() {
    clear();
}

void Schema::clear() {
    for (int i = 0; i < collection_count; i++){
        delete collections[i];
    }

    delete[] collections;

    collections = nullptr;
    collection_count = 0;
    tuples_limit = 0;
    name.clear();
}

Collection* Schema::getCollection(int index) const {
    if (index < 0 || index >= collection_count) {
        return nullptr;
    }

    return collections[index];
}

Collection* Schema::findCollection(const string& collName) const {
    for (int i = 0; i < collection_count; i++){
        if (collections[i]->getName() == collName){
            return collections[i];
        }
    }

    return nullptr;
}

static bool validateFields(const JsonValue* fields){
    if (fields == nullptr || fields->getType() != JSON_OBJECT){
        return false;
    }

    const JsonValue* current = fields->getChild();
    while (current != nullptr){
        if (current->getKey().empty()){
            return false;
        }

        if (current->getType() == JSON_OBJECT){
            if (!validateFields(current)){
                return false;
            }
        } else {
            if (current->getType() != JSON_STRING){
                return false;
            }

            string type = current->getValue();

            if (type != "int" && type != "str" && type != "time_stamp" && type != "array"){
                return false;
            }
        }

        const JsonValue* next = current->getNext();

        while (next != nullptr){
            if (next->getKey() == current->getKey()){
                return false;
            }
            next = next->getNext();
        }

        current = current->getNext();
    }

    return true;
}

bool Schema::loadFromFile(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()){
        cerr << "Cannot open: " << filename << endl;
        return false;
    }

    string json;
    string line;

    while (getline(file, line)){
        json += line + "\n";
    }

    if (file.bad()){
        return false;
    }

    file.close();

    JsonValue* root = JsonParser::parse(json);

    if (root == nullptr || root->getType() != JSON_OBJECT) {
        delete root;
        cerr << "Invalid schema JSON" << endl;
        return false;
    }

    const JsonValue* schemaName = root->find("name");
    const JsonValue* limit = root->find("tuples_limit");
    const JsonValue* structure = root->find("structure");

    if (schemaName == nullptr ||
        schemaName->getType() != JSON_STRING ||
        schemaName->getValue().empty() ||
        limit == nullptr ||
        limit->getType() != JSON_INT ||
        structure == nullptr ||
        structure->getType() != JSON_OBJECT){

        delete root;
        cerr << "Invalid schema structure" << endl;
        return false;
    }

    string limitText = limit->getValue();
    int parsedLimit = 0;

    for (char c : limitText){
        if (c < '0' || c > '9' || parsedLimit > (INT_MAX - (c - '0')) / 10){
            delete root;
            cerr << "Invalid tuples_limit" << endl;
            return false;
        }

        parsedLimit = parsedLimit * 10 + (c - '0');
    }

    if (parsedLimit <= 0 || !validateFields(structure)){
        delete root;
        cerr << "Invalid schema fields or limit" << endl;
        return false;
    }

    int count = 0;
    const JsonValue* current = structure->getChild();

    while (current != nullptr){
        if (current->getType() != JSON_OBJECT){
            delete root;
            cerr << "Invalid collection" << endl;
            return false;
        }

        count++;
        current = current->getNext();
    }

    clear();

    name = schemaName->getValue();
    tuples_limit = parsedLimit;

    collections = new Collection*[count];

    current = structure->getChild();

    while (current != nullptr) {
        Collection* coll = new Collection(current->getKey());
        if (!coll->setFields(current)) {
            delete coll;
            delete root;
            clear();
            return false;
        }

        collections[collection_count++] = coll;

        current = current->getNext();
    }

    delete root;

    return true;
}

static bool makeDirectory(const string& path) {
#ifdef _WIN32
    int result = _mkdir(path.c_str());
#else
    int result = mkdir(path.c_str(), 0777);
#endif

    if (result == 0) {
        return true;
    }

    if (errno != EEXIST) {
        return false;
    }

    struct stat info;

    return stat(path.c_str(), &info) == 0 &&
           (info.st_mode & S_IFMT) == S_IFDIR;
}

void Schema::createDirectories(){
    string mainDir = "data/" + name;

    if (!makeDirectory("data") || !makeDirectory(mainDir)){
        cerr << "Cannot create database directory" << endl;
        return;
    }

    for (int i = 0; i < collection_count; i++){
        string collDir = mainDir + "/" + collections[i]->getName();
        if (!makeDirectory(collDir)) {
            cerr << "Cannot create: " << collDir << endl;
            return;
        }

        string filePath = collDir + "/1.json";

        struct stat info;

        if (stat(filePath.c_str(), &info) == 0) {
            continue;
        }

        if (errno != ENOENT) {
            cerr << "Cannot access: " << filePath << endl;
            return;
        }

        ofstream out(filePath);

        if (!out.is_open()) {
            cerr << "Cannot create: " << filePath << endl;
            return;
        }

        out << "{}";
        out.close();

        if (out.fail()) {
            cerr << "Cannot write: " << filePath << endl;
            return;
        }
    }

    cout << "Database structure ready" << endl;
}