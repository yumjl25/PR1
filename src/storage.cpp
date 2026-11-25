#include "storage.hpp"
#include "jsonparser.hpp"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <climits>
#include <cstdint>


using namespace std;


static bool parseInt32(const std::string& text, int32_t& result) {
    if (text.empty()) return false;

    int i = 0;
    bool negative = false;

    if (text[0] == '-') {
        negative = true;
        i = 1;
    }

    if (i == static_cast<int>(text.length())) {
        return false;
    }

    int32_t number = 0;

    for (; i < static_cast<int>(text.length()); i++) {
        char digit = text[i];

        if (digit < '0' || digit > '9') {
            return false;
        }

        int32_t d = digit - '0';

        if (negative) {
            if (number < (INT32_MIN + d) / 10) {
                return false;
            }

            number = number * 10 - d;
        } else {
            if (number > (INT32_MAX - d) / 10) {
                return false;
            }

            number = number * 10 + d;
        }
    }

    result = number;
    return true;
}

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

bool Storage::validateFields(const JsonValue* fields, const JsonValue* document) const {
    if (fields == nullptr || document == nullptr){
        return false;
    }

    if (fields->getType() != JSON_OBJECT || document->getType() != JSON_OBJECT){
        return false;
    }

    const JsonValue* current = fields->getChild();

    while (current != nullptr){
        string name = current->getKey();

        const JsonValue* field = document->find(name);

        if (field == nullptr){
            cerr << "Missing field: " << name << endl;
            return false;
        }

        if (current->getType() == JSON_OBJECT){
            if (!validateFields(current, field)){
                return false;
            }
        }
        else {
            string type = current->getValue();

            bool valid =(type == "int" && field->getType() == JSON_INT) || ((type == "str" || type == "time_stamp") && field->getType() == JSON_STRING) || (type == "array" && field->getType() == JSON_ARRAY);
            if (!valid){
                cerr << "Invalid type for field: " << name << endl;
                return false;
            }
        }

        current = current->getNext();
    }

    return true;
}

bool Storage::validateDocument(const Collection* collection, const JsonValue* document) const {
    if (collection == nullptr){
        return false;
    }

    return validateFields(collection->getFields(), document);
}


KeySearchResult Storage::keyExists(const string& collectionName, const string& key) const {
    int fileNumber = 1;

    while (true){
        string filename = getFilePath(collectionName, fileNumber);

        if (!fileExists(filename)){
            break;
        }

        JsonValue* root = storageLoadFile(filename);

        if (root == nullptr){
            return KEY_ERROR;
        }

        bool found = root->find(key) != nullptr;

        delete root;

        if (found){
            return KEY_FOUND;
        }

        fileNumber++;
    }

    return KEY_NOT_FOUND;
}

string Storage::generateKey(const string& collectionName) const {
    int number = 1;

    while (true){
        string key = "doc_" + to_string(number);

        KeySearchResult result = keyExists(collectionName, key);

        if (result == KEY_NOT_FOUND){
            return key;
        }

        if (result == KEY_ERROR || number == INT_MAX){
            return "";
        }

        number++;
    }
}

bool Storage::writeFile(const string& filename, const string& content) const {
    string tempFilename = filename + ".tmp";

    ofstream out(tempFilename, ios::binary | ios::trunc);

    if (!out.is_open()){
        cerr << "Cannot create temporary file: " << tempFilename << endl;
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

        JsonValue* root = nullptr;

        if (fileExists(filename)){
            root = storageLoadFile(filename);

            if (root == nullptr){
                return false;
            }
        }
        else{
            root = new JsonValue();
            root->setObject();
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

        JsonValue* copy = new JsonValue();

        if (!copy->copyFrom(document)){
            delete copy;
            delete root;
            return false;
        }

        copy->setKey(key);
        root->addChild(copy);
        
        string resultJson = JsonParser::stringify(root);

        bool success = writeFile(filename, resultJson);
        delete root;
        if (!success) {
            cerr << "Error saving document!" << endl;
            return false;
        }
        return true;

    }
    return true;
}

bool Storage::insertOne(const string& collectionName, const JsonValue* document){
    if (schema == nullptr || document == nullptr) return false;

    if (schema->findCollection(collectionName) == nullptr){
        cerr << "Collection not found" << endl;
        return false;
    }

    string key = generateKey(collectionName);

    if (key.empty()){
        cerr << "Cannot generate primary key" << endl;
        return false;
    }

    return insertOne(collectionName, key, document);
}

int Storage::insertMany(const string& collectionName, const JsonValue* documents){
    if (documents == nullptr || schema == nullptr){
        return -1;
    }

    if (schema->findCollection(collectionName) == nullptr){
        cerr << "Collection not found" << endl;
        return -1;
    }

    if (documents->getType() != JSON_ARRAY && documents->getType() != JSON_OBJECT){
        cerr << "Invalid documents type" << endl;
        return -1;
    }

    int insertedCount = 0;
    const JsonValue* current = documents->getChild();

    while (current != nullptr){
        bool success = false;

        if (documents->getType() == JSON_ARRAY){
            success = insertOne(collectionName, current);
        }
        else{
            success = insertOne(collectionName, current->getKey(), current);
        }

        if (success){
            insertedCount++;
        }

        current = current->getNext();
    }

    return insertedCount;
}


bool Storage::matchesFilter(const JsonValue* document, const JsonValue* filter) const {
    if (document == nullptr || filter == nullptr){
        return false;
    }

    if (document->getType() != JSON_OBJECT || filter->getType() != JSON_OBJECT){
        return false;
    }

    const JsonValue* condition = filter->getChild();

    while (condition != nullptr){
        string key = condition->getKey();

        if (key == "$and" || key == "$or" || key == "$not"){
            if (!matchesLogical(document, condition)) {
                return false;
            }
        }
        else {
            if (!key.empty() && key[0] == '$') {
                cerr << "Unknown filter operator: " << key << endl;
                return false;
            }

            JsonValue primaryKey;
            const JsonValue* field = nullptr;

            if (key == "_id"){
                primaryKey.setString(document->getKey());
                field = &primaryKey;
            }
            else {
                field = document->find(key);
            }

            if (field == nullptr){
                return false;
            }

            if (!matchesCondition(field, condition)){
                return false;
            }
        }
        condition = condition->getNext();
    }
    return true;
}

int Storage::findInternal(const string& collectionName, const JsonValue* filter, int limit, const List* projection) const {
    if (schema == nullptr) return -1;

    if (schema->findCollection(collectionName) == nullptr){
        cerr << "Collection not found: " << collectionName << endl;
        return -1;
    }

    if (filter == nullptr || filter->getType() != JSON_OBJECT){
        cerr << "Invalid filter" << endl;
        return -1;
    }

    int foundCount = 0;
    int fileNumber = 1;

    while (true) {
        string filename = getFilePath(collectionName, fileNumber);

        if (!fileExists(filename)){
            break;
        }

        JsonValue* root = storageLoadFile(filename);
        if (root == nullptr){
            return -1;
        }

        const JsonValue* current = root->getChild();
        while(current != nullptr){
            if (matchesFilter(current, filter)){
                cout << current->getKey() << ":";
                printDocument(current, projection);
                cout << endl;
                foundCount++;
                if (limit > 0 && foundCount >= limit) {
                    delete root;
                    return foundCount;
                }
            }
            current = current->getNext();
        }
        delete root;
        fileNumber++;
    }
    return foundCount;
}


int Storage::find(const string& collectionName, const JsonValue* filter, int limit, const List* projection, const string& sortField, bool ascending) const {
    if (limit < 0){
        cerr << "Invalid limit" << endl;
        return -1;
    }

    if(sortField.empty()){
        return findInternal(collectionName, filter, limit, projection);
    }

    return findSorted(collectionName, filter, limit, projection, sortField, ascending);
}

bool Storage::findOne(const string& collectionName, const JsonValue* filter) const {
    return findInternal(collectionName, filter, 1, nullptr) == 1;
}


bool Storage::compareValues(const JsonValue* first, const JsonValue* second, const string& op) const {
    if (first == nullptr || second == nullptr){
        return false;
    }

    if (first->getType() != second->getType()){
        return false;
    }

    JsonType type = first->getType();

    if (type == JSON_INT) {
        int32_t a;
        int32_t b;

        if (!parseInt32(first->getValue(), a) || !parseInt32(second->getValue(), b)) {
            return false;
        }

        if (op == "$eq") return a == b;
        if (op == "$ne") return a != b;
        if (op == "$gt") return a > b;
        if (op == "$lt") return a < b;
        if (op == "$gte") return a >= b;
        if (op == "$lte") return a <= b;
    }
    else if (type == JSON_STRING) {
        string a = first->getValue();
        string b = second->getValue();

        if (op == "$eq") return a == b;
        if (op == "$ne") return a != b;
        if (op == "$gt") return a > b;
        if (op == "$lt") return a < b;
        if (op == "$gte") return a >= b;
        if (op == "$lte") return a <= b;
    }
    else if (type == JSON_NULL) {
        if (op == "$eq") return true;
        if (op == "$ne") return false;
    }
    else if (type == JSON_OBJECT || type == JSON_ARRAY) {
        if (op == "$eq" || op == "$ne") {
            bool equal = JsonParser::stringify(first) == JsonParser::stringify(second);
            return op == "$eq" ? equal : !equal;
        }
    }

    return false;
}


bool Storage::matchesCondition(const JsonValue* field, const JsonValue* condition) const {
    if (field == nullptr || condition == nullptr){
        return false;
    }

    if (condition->getType() != JSON_OBJECT){
        return compareValues(field, condition, "$eq");
    }

    const JsonValue* opNode = condition->getChild();

    if (opNode == nullptr){
        return compareValues(field, condition, "$eq");
    }

    if (opNode->getKey().empty() || opNode->getKey()[0] != '$'){
        return compareValues(field, condition, "$eq");
    }

    while (opNode != nullptr){
        string op = opNode->getKey();

        if (op == "$in") {
            if (opNode->getType() != JSON_ARRAY){
                return false;
            }

            bool found = false;
            const JsonValue* item = opNode->getChild();

            while (item != nullptr) {
                if (compareValues(field, item, "$eq")){
                    found = true;
                    break;
                }

                item = item->getNext();
            }

            if (!found) return false;
        }

        else if (op == "$not"){
            if (opNode->getType() != JSON_OBJECT || opNode->getChild() == nullptr) {
                return false;
            }
            if (matchesCondition(field, opNode)) {
                return false;
            }
        }

        else if (op == "$eq" || op == "$ne" || op == "$gt" || op == "$lt" || op == "$gte" || op == "$lte"){

            if (!compareValues(field, opNode, op)){
                return false;
            }
        }
        else{
            return false;
        }

        opNode = opNode->getNext();
    }

    return true;
}

bool Storage::matchesLogical(const JsonValue* document, const JsonValue* condition) const {
    if (document == nullptr || condition == nullptr) return false;
    string op = condition->getKey();
    if (op == "$and"){
        if (condition->getType() != JSON_ARRAY) return false;
        const JsonValue* current = condition->getChild();
        if (current == nullptr) return false;

        while (current != nullptr){
            if (!matchesFilter(document, current)){
                return false;
            }
            current = current->getNext();
        }
        return true;
    }

    if (op == "$or"){
        if (condition->getType() != JSON_ARRAY) return false;
        const JsonValue* current = condition->getChild();
        if (current == nullptr) return false;

        while (current != nullptr){
            if (matchesFilter(document, current)){
                return true;
            }
            current = current->getNext();
        }
        return false;
    }

    if (op == "$not") {
        if (condition->getType() != JSON_OBJECT) {
            return false;
        }

        string json = JsonParser::stringify(condition);
        JsonValue* filter = JsonParser::parse(json);

        if (filter == nullptr) {
            return false;
        }

        bool result = !matchesFilter(document, filter);

        delete filter;
        return result;
    }
    cerr << "Unknown logical operator: " << op << endl;
    return false;
}

void Storage::printDocument(const JsonValue* document, const List* projection) const {
    if (document == nullptr) return;
    if (projection == nullptr){
        cout << JsonParser::stringify(document);
        return;
    }
    cout << "{";
    bool first = true;

    for (int i = 0; i < projection->getSize(); i++){
        string fieldName;
        if (!projection->getAt(i, fieldName)){
            continue;
        }
        
        const JsonValue* field = document->find(fieldName);
        if (field == nullptr) continue;

        if(!first) cout << ",";
        JsonValue* keyValue = new JsonValue();
        keyValue->setString(fieldName);

        cout << JsonParser::stringify(keyValue);
        cout << ":";
        cout << JsonParser::stringify(field);

        delete keyValue;
        first = false;
    }
    cout << "}";
}

JsonValue* Storage::storageLoadFile(const string& filename) const {
    ifstream file(filename);

    if (!file.is_open()){
        cerr << "Cannot open file: " << filename << endl;
        return nullptr;
    }

    string content;
    string line;
    
    while(getline(file, line)){
        content += line;
        content += '\n';
    }

    if (file.bad()){
        cerr << "Error reading file: " << filename << endl;
        return nullptr;
    }

    if (content.empty()){
        cerr << "Empty storage file: " << filename << endl;
        return nullptr;
    }

    JsonValue* root = JsonParser::parse(content);

    if (root == nullptr){
        cerr << "Cannot parse storage file: " << filename;
        return nullptr;
    }

    if (root->getType() != JSON_OBJECT){
        cerr << "Invalid storage format: " << filename << endl;
        delete root;
        return nullptr;
    }
    return root;
}


bool Storage::aplyUpdate(JsonValue* document, const JsonValue* update) const {
    if (document == nullptr || update == nullptr) return false;

    if (document->getType() != JSON_OBJECT || update->getType() != JSON_OBJECT || update->getChild() == nullptr){
        cerr << "Invalid update arguments" << endl;
        return false;
    }

    const JsonValue* current = update->getChild();

    while (current != nullptr){
        string op = current->getKey();

        if (op != "$set" && op != "$inc" && op != "$push"){
            cerr << "Unknown update operator: " << op << endl;
            return false;
        }

        if (current->getType() != JSON_OBJECT){
            cerr << "Invalid operator arguments: " << op << endl;
            return false;
        }

        const JsonValue* item = current->getChild();

        while (item != nullptr){
            JsonValue* field = document->find(item->getKey());

            if (field == nullptr){
                cerr << "Field not found: " << item->getKey() << endl;
                return false;
            }

            if (op == "$set"){
                if (field->getType() != item->getType() || !field->copyFrom(item)){
                    cerr << "Invalid $set value" << endl;
                    return false;
                }
            }
            else if (op == "$inc"){
                int32_t a, b;

                if (field->getType() != JSON_INT || item->getType() != JSON_INT || !parseInt32(field->getValue(), a) || !parseInt32(item->getValue(), b)){
                    cerr << "Invalid $inc value" << endl;
                    return false;
                }

                if ((b > 0 && a > INT32_MAX - b) || (b < 0 && a < INT32_MIN - b)){
                    cerr << "Integer overflow" << endl;
                    return false;
                }

                field->setInt(a + b);
            }
            else if (op == "$push"){
                if (field->getType() != JSON_ARRAY){
                    cerr << "$push requires an array" << endl;
                    return false;
                }

                JsonValue* element = new JsonValue();

                if (!element->copyFrom(item)){
                    delete element;
                    return false;
                }

                element->setKey("");
                field->addChild(element);
            }

            item = item->getNext();
        }

        current = current->getNext();
    }

    return true;
}


bool Storage::updateOne(const string& collectionName, const JsonValue* filter, const JsonValue* update){
    if (update == nullptr) return false;
    return modifyInternal(collectionName, filter, update, false) == 1;
}

int Storage::updateMany(const string& collectionName, const JsonValue* filter, const JsonValue* update){
    if (update == nullptr) return -1;
    return modifyInternal(collectionName, filter, update, true);
}


int Storage::modifyInternal(const string& collectionName, const JsonValue* filter, const JsonValue* update, bool multi){
    if (schema == nullptr) return -1;

    Collection* collection = schema->findCollection(collectionName);

    if (collection == nullptr){
        cerr << "Collection not found: " << collectionName << endl;
        return -1;
    }

    if (filter == nullptr || filter->getType() != JSON_OBJECT || (update != nullptr && update->getType() != JSON_OBJECT)){
        cerr << "Invalid modification arguments" << endl;
        return -1;
    }

    int count = 0;
    int fileNumber = 1;

    while (true){
        string filename = getFilePath(collectionName, fileNumber);

        if (!fileExists(filename)) break;

        JsonValue* root = storageLoadFile(filename);

        if (root == nullptr) return -1;

        bool fileChanged = false;
        JsonValue* current = root->getChild();

        while (current != nullptr){
            JsonValue* next = current->getNext();

            if (matchesFilter(current, filter)){
                bool success;

                if (update == nullptr){
                    success = root->removeChild(current);
                }
                else{
                    success = aplyUpdate(current, update) && validateDocument(collection, current);
                }

                if (!success){
                    cerr << "Document modification failed" << endl;
                    delete root;
                    return -1;
                }

                fileChanged = true;
                count++;

                if (!multi) break;
            }

            current = next;
        }

        if (fileChanged && !writeFile(filename, JsonParser::stringify(root))){
            delete root;
            return -1;
        }

        delete root;

        if (!multi && count > 0) break;

        fileNumber++;
    }

    return count;
}


bool Storage::deleteOne(const string& collectionName, const JsonValue* filter){
    return modifyInternal(collectionName, filter, nullptr, false) == 1;
}

int Storage::deleteMany(const string& collectionName, const JsonValue* filter){
    return modifyInternal(collectionName, filter, nullptr, true);
}

int Storage::compareDocument(const JsonValue* first, const JsonValue* second, const string& sortField) const {
    const JsonValue* firstField = first->find(sortField);
    const JsonValue* secondField = second->find(sortField);

    if (firstField == nullptr && secondField == nullptr){
        return 0;
    }
    if (firstField == nullptr) {return -1;}
    if (secondField == nullptr) {return -1;}

    if (firstField->getType() != secondField->getType()){
        if (firstField->getType() < secondField->getType()){
            return -1;
        }
        return 1;
    }

    if (firstField->getType() == JSON_INT){
        int32_t firstValue;
        int32_t secondValue;

        if (parseInt32(firstField->getValue(), firstValue) && parseInt32(secondField->getValue(), secondValue)){
            if (firstValue < secondValue) return -1;
            if (firstValue > secondValue) return 1;
            
            return 0;
        }
    }

    string firstValue = firstField->getValue();
    string secondValue = secondField->getValue();
    if (firstValue < secondValue) return -1;
    if (firstValue > secondValue) return 1;

    return 0;
}

int Storage::findSorted(const string& collectionName, const JsonValue* filter, int limit, const List* projection, const string& sortField, bool ascending) const {
    if (schema == nullptr) return -1;
    if (schema->findCollection(collectionName) == nullptr){
        cerr << "Collection not found: " << collectionName << endl;
        return -1;
    }

    if (filter == nullptr || filter->getType() != JSON_OBJECT){
        cerr << "Invalid filter" << endl;
        return -1;
    }

    if (limit < 0){
        cerr << "Invalid limit" << endl;
        return -1;
    }

    int fileNumber = 1;
    int runCount = 0;

    while(true){
        string filename = getFilePath(collectionName, fileNumber);

        if (!fileExists(filename)){
            break;
        }

        string tempFilename = getSortFilePath(collectionName, 0, runCount + 1);

        int count = createSortedRun(filename, tempFilename, filter, sortField, ascending);

        if (count < 0){
            cleanupSortFiles(collectionName, runCount + 1);
            return -1;
        }

        if (count == 0){
            std::remove(tempFilename.c_str());
        } else {
            runCount++;
        }

        fileNumber++;
    }
    if (runCount == 0) {
        return 0;
    }

    int initialRunCount = runCount;
    int pass = 0;

    while (runCount > 1){

        int newRunCount = 0;

        for (int i = 1; i <= runCount; i += 2){

            string firstFilename = getSortFilePath(collectionName, pass, i);

            string outputFilename = getSortFilePath(collectionName, pass + 1, newRunCount + 1);

            if (i == runCount) {

                if (std::rename(firstFilename.c_str(), outputFilename.c_str()) != 0){
                    cerr << "Cannot move sorted file" << endl;

                    cleanupSortFiles(collectionName, initialRunCount);
                    return -1;
                }

            } else {
                string secondFilename = getSortFilePath(collectionName, pass, i + 1);

                if (!mergeSortedRuns(firstFilename, secondFilename, outputFilename, sortField, ascending)){
                    cleanupSortFiles(collectionName, initialRunCount);
                    return -1;
                }

                std::remove(firstFilename.c_str());
                std::remove(secondFilename.c_str());
            }
            newRunCount++;
        }

        runCount = newRunCount;
        pass++;
    }

    string finalFilename = getSortFilePath(collectionName, pass, 1);

    ifstream file(finalFilename);

    if (!file.is_open()) {
        cleanupSortFiles(collectionName, initialRunCount);
        return -1;
    }

    int foundCount = 0;
    bool success = true;

    while (limit == 0 || foundCount < limit){

        JsonValue* document = nullptr;

        int status = readSortedDocument(file, document);

        if (status == 0){
            break;
        }

        if (status == -1){
            delete document;
            success = false;
            break;
        }

        cout << document->getKey() << ":";

        printDocument(document, projection);

        cout << endl;

        foundCount++;

        delete document;
    }

    file.close();

    cleanupSortFiles(collectionName, initialRunCount);

    if (!success){
        return -1;
    }

    return foundCount;
}



int Storage::createSortedRun(const string& filename, const string& tempFilename, const JsonValue* filter, const string& sortField, bool ascending) const {
    JsonValue* root = storageLoadFile(filename);

    if (root == nullptr) return -1;

    int capacity = 0;
    const JsonValue* current = root->getChild();

    while (current != nullptr){
        capacity++;
        current = current->getNext();
    }

    const JsonValue** documents = new const JsonValue*[capacity];

    int count = 0;
    current = root->getChild();

    while (current != nullptr){
        if (matchesFilter(current, filter)){
            documents[count++] = current;
        }

        current = current->getNext();
    }

    for (int i = 1; i < count; i++){
        const JsonValue* key = documents[i];
        int j = i - 1;

        while (j >= 0){
            int cmp = compareDocument(documents[j], key, sortField);

            if ((ascending && cmp <= 0) || (!ascending && cmp >= 0)){
                break;
            }

            documents[j + 1] = documents[j];
            j--;
        }

        documents[j + 1] = key;
    }

    ofstream out(tempFilename);
    bool success = out.is_open();

    for (int i = 0; i < count && success; i++){
        success = writeSortedDocument(out, documents[i]);
    }

    out.close();
    success = success && !out.fail();

    delete[] documents;
    delete root;

    if (!success){
        cerr << "Cannot write temporary file" << endl;
        return -1;
    }

    return count;
}



int Storage::readSortedDocument(ifstream& file, JsonValue*& document) const {
    document = nullptr;
    string keyText;
    string documentText;

    if (!getline(file, keyText)){
        if (file.eof() && !file.bad()){
            return 0;
        }
        return -1;
    }

    if (!getline(file, documentText)){
        return -1;
    }

    JsonValue* key = JsonParser::parse(keyText);

    if (key == nullptr || key->getType() != JSON_STRING){
        delete key;
        return -1;
    }

    document = JsonParser::parse(documentText);

    if (document == nullptr || document->getType() != JSON_OBJECT){
        delete key;
        delete document;

        document = nullptr;
        return -1;
    }

    document->setKey(key->getValue());

    delete key;

    return 1;
}


bool Storage::writeSortedDocument(ofstream& file, const JsonValue* document) const {
    if (document == nullptr) return false;

    JsonValue key(document->getKey());

    file << JsonParser::stringify(&key) << '\n';
    file << JsonParser::stringify(document) << '\n';

    return file.good();
}


bool Storage::mergeSortedRuns(const string& firstFilename, const string& secondFilename, const string& outputFilename, const string& sortField, bool ascending) const {
    ifstream firstFile(firstFilename);
    ifstream secondFile(secondFilename);
    ofstream outputFile(outputFilename);

    if (!firstFile.is_open() || !secondFile.is_open() || !outputFile.is_open()){
        cerr << "Cannot open sorted files" << endl;
        return false;
    }

    JsonValue* first = nullptr;
    JsonValue* second = nullptr;

    int firstStatus = readSortedDocument(firstFile, first);
    int secondStatus = readSortedDocument(secondFile, second);

    bool success = true;

    while (firstStatus >= 0 && secondStatus >= 0 && (firstStatus == 1 || secondStatus == 1)){
        bool takeFirst;

        if (firstStatus == 0){
            takeFirst = false;
        }
        else if (secondStatus == 0){
            takeFirst = true;
        }
        else{
            int cmp = compareDocument(first, second, sortField);
            takeFirst = ascending ? cmp <= 0 : cmp >= 0;
        }

        JsonValue*& document = takeFirst ? first : second;
        ifstream& file = takeFirst ? firstFile : secondFile;
        int& status = takeFirst ? firstStatus : secondStatus;

        if (!writeSortedDocument(outputFile, document)){
            success = false;
            break;
        }

        delete document;
        document = nullptr;

        status = readSortedDocument(file, document);
    }

    delete first;
    delete second;

    outputFile.close();

    if (firstStatus < 0 || secondStatus < 0 || outputFile.fail()){
        success = false;
    }

    if (!success){
        cerr << "Error merging sorted files" << endl;
    }

    return success;
}

string Storage::getSortFilePath(const string& collectionName, int pass, int number) const {
    return getCollectionPath(collectionName) + "/sort_" + to_string(pass) + "_" + to_string(number) + ".tmp";
}

void Storage::cleanupSortFiles(const string& collectionName, int runCount) const {
    int pass = 0;
    while (runCount > 0){
        for (int i = 1; i <= runCount; i++){
            string filename = getSortFilePath(collectionName, pass, i);
            std::remove(filename.c_str());
        }
        if (runCount == 1) break;
        runCount = runCount / 2 + runCount % 2;
        pass++;
    }
}


