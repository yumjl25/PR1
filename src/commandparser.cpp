#include "commandparser.hpp"
#include "jsonparser.hpp"
#include <iostream>

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


bool CommandParser::execute(const string& command) {

    string collection;
    string operation;
    string argumentsText;

    if (!parseCommand(command, collection, operation, argumentsText)) {
        cerr << "Invalid command syntax" << endl;
        return false;
    }

    List arguments;

    if (!splitArguments(argumentsText, arguments)) {
        cerr << "Invalid arguments" << endl;
        return false;
    }

    cout << "Collection: " << collection << endl;
    cout << "Operation: " << operation << endl;

    cout << "Arguments count: "
         << arguments.getSize() << endl;

    for (int i = 0; i < arguments.getSize(); i++){
        string out;
        if (!arguments.getAt(i, out)){
            cerr << "Cannot get argument" << endl;
            return false;
        }

        cout << "Argument " << i << ": " << out << endl;
    }

    return true;
}
