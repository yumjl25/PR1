#include "commandparser.hpp"
#include "jsonparser.hpp"
#include <iostream>
#include <climits>

using namespace std;

CommandParser::CommandParser(Storage* storage){
    this->storage = storage;
}

string CommandParser::trim(const string& text) const {
    size_t start = 0;
    size_t end = text.size();

    while (start < end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\n' || text[start] == '\r')){
        start++;
    }

    while(end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' || text[end - 1] == '\r')){
        end--;
    }
    return text.substr(start, end - start);
}

bool CommandParser::parseCommand(const string& command, string& collection, string& operation, string& arguments) const {
    string text = trim(command);

    if (!text.empty() && text.back() == ':'){
        text = trim(text.substr(0, text.size() - 1));
    }

    if (text.substr(0, 3) != "db."){
        return false;
    }
    size_t pos = 3;
    size_t start = pos;

    while(pos < text.size() && text[pos] != '.'){
        pos++;
    }

    if (pos == text.size()){
        return false;
    }

    collection = trim(text.substr(start, pos - start));
    pos++;
    start = pos;

    while(pos < text.size() && text[pos] != '('){
        pos++;
    }

    if (pos == text.size()){
        return false;
    }

    operation = trim(text.substr(start, pos - start));

    if (collection.empty() || operation.empty()){
        return false;
    }

    if (text.back() != ')'){
        return false;
    }

    pos++;

    arguments = trim(text.substr(pos, text.size() - pos - 1));
    return true;
}


bool CommandParser::splitArguments(const string& text, List& arguments) const {
    if (trim(text).empty()) {
        return true;
    }

    int objectDepth = 0;
    int arrayDepth = 0;

    bool inString = false;
    bool escaped = false;

    size_t start = 0;

    for (size_t i = 0; i < text.size(); i++){
        char c = text[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            }
            else if (c == '\\') {
                escaped = true;
            }
            else if (c == '"') {
                inString = false;
            }

            continue;
        }

        if (c == '"') {
            inString = true;
        }
        else if (c == '{') {
            objectDepth++;
        }
        else if (c == '}') {
            objectDepth--;
        }
        else if (c == '[') {
            arrayDepth++;
        }
        else if (c == ']') {
            arrayDepth--;
        }
        else if (c == '(' || c == ')') {
            return false;
        }
        else if (c == ',' && objectDepth == 0 && arrayDepth == 0) {
            string argument = trim(text.substr(start, i - start));

            if (argument.empty()) {
                return false;
            }
            arguments.pushTail(argument);

            start = i + 1;
        }

        if (objectDepth < 0 || arrayDepth < 0){
            return false;
        }
    }

    if (inString || escaped || objectDepth != 0 || arrayDepth != 0){
        return false;
    }

    string argument = trim(text.substr(start));

    if (argument.empty()) {
        return false;
    }

    arguments.pushTail(argument);

    return true;
}


bool CommandParser::execute(const string& command){

    string collection;
    string operation;
    string argumentsText;

    if (!parseCommand(command, collection, operation, argumentsText)){
        cerr << "Invalid command syntax" << endl;
        return false;
    }

    List arguments;

    if (!splitArguments(argumentsText, arguments)){
        cerr << "Invalid arguments" << endl;
        return false;
    }

    if (operation == "find" || operation == "find_one"){
        return executeFind(collection, operation, arguments);
    }

    if (operation == "update_one" || operation == "update_many" || operation == "update"){
        return executeUpdate(collection, operation, arguments);
    }

    if (operation == "delete_one" || operation == "delete_many"){
        return executeDelete(collection, operation, arguments);
    }

    if (operation == "insert" || operation == "insert_one" || operation == "insert_many"){
        return executeInsert(collection, operation, arguments);
    }
    cerr << "Unknown operation: " << operation << endl;

    return false;
}

JsonValue* CommandParser::parseArgument(const List& arguments, int index) const {
    string out;
    if(!arguments.getAt(index, out)){
        cerr << "Argument not found" << endl;
        return nullptr;
    }

    JsonValue* value = JsonParser::parse(out);
    if (value == nullptr){
        cerr << "Invalid JSON argument" << endl;
        return nullptr;
    }
    return value;
}

bool CommandParser::executeFind(const string& collection, const string& operation, const List& arguments){
    int size = arguments.getSize();

    if (size > 4){
        cerr << "Invalid arguments count" << endl;
        return false;
    }

    JsonValue* filter = nullptr;

    if (size == 0){
        filter = JsonParser::parse("{}");
    } else {
        filter = parseArgument(arguments, 0);
    }

    if (filter == nullptr || filter->getType() != JSON_OBJECT) {
        delete filter;
        cerr << "Invalid filter" << endl;
        return false;
    }

    List projection;
    string sortField;

    bool ascending = true;
    bool valid = true;

    int limit = 0;

    if (size >= 2){
        string text;
        arguments.getAt(1, text);

        size_t pos = text.find('=');

        if (pos != string::npos && trim(text.substr(0, pos)) == "projection"){
            text = trim(text.substr(pos + 1));
        }

        JsonValue* fields = JsonParser::parse(text);

        if (fields == nullptr){
            valid = false;
        }
        else if (fields->getType() == JSON_ARRAY){
            const JsonValue* current = fields->getChild();

            while (current != nullptr){
                if (current->getType() != JSON_STRING || current->getValue().empty()){
                    valid = false;
                    break;
                }

                projection.pushTail(current->getValue());
                current = current->getNext();
            }
        }
        else if (fields->getType() == JSON_OBJECT){
            const JsonValue* current = fields->getChild();

            while (current != nullptr){
                if (current->getType() != JSON_INT || current->getValue() != "1"){
                    valid = false;
                    break;
                }

                projection.pushTail(current->getKey());
                current = current->getNext();
            }
        }
        else{
            valid = false;
        }

        delete fields;
    }

    if (size >= 3 && valid){
        JsonValue* sort = parseArgument(arguments, 2);
        if (sort == nullptr || sort->getType() != JSON_OBJECT) {
            valid = false;
        } else {
            const JsonValue* field = sort->getChild();
            if (field != nullptr){
                if (field->getNext() != nullptr || field->getType() != JSON_INT || (field->getValue() != "1" && field->getValue() != "-1")){
                    valid = false;
                } else {
                    sortField = field->getKey();
                    ascending = (field->getValue() == "1");
                }
            }
        }
        delete sort;
    }

    if (size >= 4 && valid){
        string text;
        if (!arguments.getAt(3, text) || text.empty()){
            valid = false;
        } else {
            for (size_t i = 0; i < text.size(); i++){
                char c = text[i];
                if (c < '0' || c > '9'){
                    valid = false;
                    break;
                }
                int digit = c - '0';
                if (limit > (INT_MAX - digit) / 10) {
                    valid = false;
                    break;
                }
                limit = limit * 10 + digit;
            }
        }
    }

    if (!valid){
        delete filter;
        cerr << "Invalid find arguments" << endl;
        return false;
    }

    if (operation == "find_one"){
        limit = 1;
    }

    const List* projectionPtr = nullptr;

    if (projection.getSize() > 0){
        projectionPtr = &projection;
    }

    int count = storage->find(collection, filter, limit, projectionPtr, sortField, ascending);

    delete filter;

    if (count < 0){
        return false;
    }

    cout << "Found: " << count << endl;

    return true;
}

bool CommandParser::executeUpdate(const string& collection, const string& operation, const List& arguments){
    int size = arguments.getSize();

    if (size < 2 || size > 3 || (size == 3 && operation != "update")){
        cerr << "Invalid arguments count" << endl;
        return false;
    }

    bool multi = (operation == "update_many");

    if (size == 3){
        string text;
        arguments.getAt(2, text);

        size_t pos = text.find('=');

        if (pos == string::npos || trim(text.substr(0, pos)) != "multi"){
            cerr << "Invalid multi argument" << endl;
            return false;
        }

        string value = trim(text.substr(pos + 1));

        if (value == "True" || value == "true"){
            multi = true;
        }
        else if (value == "False" || value == "false"){
            multi = false;
        }
        else{
            cerr << "Invalid multi value" << endl;
            return false;
        }
    }

    JsonValue* filter = parseArgument(arguments, 0);
    JsonValue* update = parseArgument(arguments, 1);

    if (filter == nullptr || update == nullptr || filter->getType() != JSON_OBJECT || update->getType() != JSON_OBJECT){
        delete filter;
        delete update;
        cerr << "Invalid update arguments" << endl;
        return false;
    }

    bool success;
    if (!multi){
        success = storage->updateOne(collection, filter, update);
        if (success){
        cout << "Updated: 1" << endl;
        }else{
            cout << "No update performed" << endl;
        }
    }else{
        int count = storage->updateMany(collection, filter, update);
        success = (count >= 0);

        if (success){
            cout << "Updated: " << count << endl;
        }
    }
    delete filter;
    delete update;
    return success;
}

bool CommandParser::executeDelete(const string& collection, const string& operation, const List& arguments){
    if (arguments.getSize() != 1){
        cerr << "Invalid arguments count" << endl;
        return false;
    }

    JsonValue* filter = parseArgument(arguments, 0);

    if (filter == nullptr || filter->getType() != JSON_OBJECT){

        delete filter;

        cerr << "Invalid filter" << endl;
        return false;
    }

    bool success;

    if (operation == "delete_one"){

        success = storage->deleteOne(
            collection, filter
        );

        if (success){
            cout << "Deleted: 1" << endl;
        } else {
            cout << "No deletion performed" << endl;
        }

    } else {
        int count = storage->deleteMany(
            collection, filter
        );
        success = (count >= 0);
        if (success) {
            cout << "Deleted: " << count << endl;
        }
    }

    delete filter;
    return success;
}

bool CommandParser::executeInsert(const string& collection, const string& operation, const List& arguments){
    if (operation == "insert_many") {
        if (arguments.getSize() != 1) {
            cerr << "Invalid arguments count" << endl;
            return false;
        }

        JsonValue* documents = parseArgument(arguments, 0);

        if (documents == nullptr || (documents->getType() != JSON_OBJECT && documents->getType() != JSON_ARRAY)){
            delete documents;
            cerr << "Invalid documents" << endl;
            return false;
        }

        int count = storage->insertMany(collection, documents);
        delete documents;

        if (count < 0) {
            return false;
        }

        cout << "Inserted: " << count << endl;

        return true;
    }

    if (arguments.getSize() == 1){
    JsonValue* document = parseArgument(arguments, 0);

    if (document == nullptr || document->getType() != JSON_OBJECT){
        delete document;
        cerr << "Invalid document" << endl;
        return false;
    }

    bool success = storage->insertOne(collection, document);

    delete document;

    if (success){
        cout << "Inserted: 1" << endl;
    }

    return success;
}

    if (arguments.getSize() != 2){
        cerr << "Invalid arguments count" << endl;
        return false;
    }

    JsonValue* key = parseArgument(arguments, 0);
    JsonValue* document = parseArgument(arguments, 1);

    if (key == nullptr || document == nullptr || key->getType() != JSON_STRING || document->getType() != JSON_OBJECT){
        delete key;
        delete document;
        cerr << "Invalid insert arguments" << endl;

        return false;
    }

    bool success = storage->insertOne(collection, key->getValue(), document);
    delete key;
    delete document;
    if (success){
        cout << "Inserted: 1" << endl;
    }
    return success;
}