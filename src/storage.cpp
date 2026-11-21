#include "storage.hpp"
#include "jsonparser.hpp"

#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

Storage::Storage(Schema* dbSchema) : schema(dbSchema) {}

string Storage::getCollectionPath(const string& collectionName) const {
    return "data/" + schema->getName() + "/" + collectionName;
}

string Storage::getFilePath(const string& collectionName, int fileNumber) const {
    return getCollectionPath(collectionName) + "/" + to_string(fileNumber) + ".json";
}

bool Storage::fileExists(const string& filename) const {
    ifstream file(filename);
    return file.is_open();
}

bool Storage::validateDocument(const Collection* collection, const JsonValue* document) const {
    if (collection == nullptr || document == nullptr) return false;
    if (document->getType() != JSON_OBJECT) return false;

    for (int i = 0; i < collection->getFieldCount(); i++){
        string fieldName = collection->getFieldName(i);
        string fieldType = collection->getFieldType(i);

        JsonValue* field = document->find(fieldName);

        if (field == nullptr){
            cerr << "Missing field: " << fieldName << endl;
            return false;
        }

        if (fieldType == "int"){
            if (field->getType() != JSON_INT){
                cerr << "Invalid type for field: " << fieldName << endl;
                return false;
            }
        }
        else if(fieldType == "str" || fieldType == "time_stamp"){
            if (field->getType() != JSON_STRING){
                cerr << "Invalid type for field: " << fieldName << endl;
                return false;
            }
        }
        else{
            cerr << "Unsupported field type: " << fieldType << endl;
            return false;
        }
    }
    return true;
}


KeySearchResult Storage::keyExists(const string& collectionName, const string& key) const {
    int fileNumber = 1;

    while (true){
        string filename = getFilePath(collectionName, fileNumber);

        if (!fileExists(filename)){
            break;
        }

        ifstream file(filename);

        if (!file.is_open()){
            cerr << "Cannot open file: " << filename << endl;
            return KEY_ERROR;
        }

        string content = "";
        string line;

        while (getline(file, line)){
            content += line;
            content += '\n';
        }

        if (file.bad()) {
            cerr << "Error reading file: " << filename << endl;
            return KEY_ERROR;
        }

        file.close();

        if (content.empty()){
            fileNumber++;
            continue;
        }

        JsonValue* root = JsonParser::parse(content);

        if (root == nullptr || root->getType() != JSON_OBJECT){
            delete root;
            cerr << "Invalid storage file: " << filename << endl;
            return KEY_ERROR;
        }

        bool found = root->find(key) != nullptr;

        delete root;

        if (found) {
            return KEY_FOUND;
        }

        fileNumber++;
    }

    return KEY_NOT_FOUND;
}


bool Storage::writeFile(const string& filename, const string& content) const {
    string tempFilename = filename + ".tmp";

    ofstream out(tempFilename, ios::binary | ios::trunc);

    if (!out.is_open()){
        cerr << "Cannot create temporary file: "
             << tempFilename << endl;
        return false;
    }

    out << content;

    out.flush();

    bool success = out.good();

    out.close();

    if (!success || out.fail()){
        cerr << "Error writing temporary file!" << endl;
        remove(tempFilename.c_str());
        return false;
    }

    if (rename(tempFilename.c_str(), filename.c_str()) != 0){
        cerr << "Cannot replace file: " << filename << endl;
        remove(tempFilename.c_str());
        return false;
    }

    return true;
}


bool Storage::insertOne(const std::string& collectionName, const std::string& key, const JsonValue* document){
    if (schema == nullptr || key.empty()) return false;

    Collection* collection = schema->findCollection(collectionName);
    if (collection == nullptr){
        cerr << "Collection not found" << endl;
        return false;
    }

    if (!validateDocument(collection, document)){
        cerr << "Document validation failed" << endl;
        return false;
    }

    if (schema->getTuplesLimit() <= 0){
        cerr << "Invalid tuples limit" << endl;
        return false;
    }


    KeySearchResult searchResult = keyExists(collectionName, key);

    if (searchResult == KEY_FOUND){
        cerr << "Key already exists: " << key << endl;
        return false;
    }

    if (searchResult == KEY_ERROR){
        cerr << "Cannot insert document: storage error" << endl;
        return false;
    }

    int fileNumber = 1;
    while(true){
        string filename = getFilePath(collectionName, fileNumber);
        ifstream file(filename);

        string content = "";
        string line;

        if (file.is_open()) {
            while (getline(file, line)) {
                content += line;
            }
            file.close();
        }

        JsonValue* root = nullptr;
        if (content.empty()){
            root = new JsonValue();
            root->setObject();
        }else{
            root = JsonParser::parse(content);
        }

        if (root == nullptr || root->getType() != JSON_OBJECT){
            delete root;
            cerr << "Invalid storage file: " << filename << endl;
            return false;
        }

        int count = 0;
        JsonValue* cur = root->getChild();
        while(cur != nullptr){
            count++;
            cur = cur->getNext();
        }

        if (count >= schema->getTuplesLimit()){
            delete root;
            fileNumber++;
            continue;
        }

        string documentJson = JsonParser::stringify(document);
        JsonValue* copy =  JsonParser::parse(documentJson);

        if (copy == nullptr){
            delete root;
            return false;
        }

        copy->setKey(key);
        root->addChild(copy);

        ofstream out(filename);

        
        string resultJson = JsonParser::stringify(root);

        bool success = writeFile(filename, resultJson);
        delete root;
        if (!success) {
            cerr << "Error saving document!" << endl;
            return false;
        }
        cout << "Document inserted successfully!" << endl;
        return true;

    }
    return true;
}