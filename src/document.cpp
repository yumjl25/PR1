#include "document.hpp"
using namespace std;

JsonValue::JsonValue() : type(JSON_NULL), value(""), child(nullptr), next(nullptr), key("") {}
JsonValue::JsonValue(int number) : JsonValue(){
    setInt(number);
}
JsonValue::JsonValue(const string& text) : JsonValue(){
    setString(text);
}

JsonValue::~JsonValue(){
    delete child;
    delete next;
}

JsonType JsonValue::getType() const {
    return type;
}

string JsonValue::getValue() const {
    return value;
}

string JsonValue::getKey() const {
    return key;
}

JsonValue* JsonValue::getChild() const {
    return child;
}

JsonValue* JsonValue::getNext() const{
    return next;
}

void JsonValue::setKey(const string& newKey){
    key = newKey;
}

void JsonValue::setInt(int number){
    if (child != nullptr){
        delete child;
        child = nullptr;
    }

    type = JSON_INT;
    value = to_string(number);
}

void JsonValue::setString(const string& text){
    if (child != nullptr){
        delete child;
        child = nullptr;
    }

    type = JSON_STRING;
    value = text;
}

void JsonValue::setObject(){
    if (child != nullptr){
        delete child;
        child = nullptr;
    }
    
    type = JSON_OBJECT;
    value = "";
}

void JsonValue::setArray(){
    if (child != nullptr){
        delete child;
        child = nullptr;
    }
    
    type = JSON_ARRAY;
    value = "";
}

void JsonValue::addChild(JsonValue* node){
    if (node == nullptr) return;

    if (type != JSON_OBJECT && type != JSON_ARRAY) return;

    if (child == nullptr){
        child = node;
        return;
    }

    JsonValue* cur = child;
    while(cur->next != nullptr){
        cur = cur->next;
    }
    cur->next = node;
}

JsonValue* JsonValue::find(const string& fieldName) const {
    if (type != JSON_OBJECT) return nullptr;

    JsonValue* cur = child;
    while(cur != nullptr){
        if (cur->key == fieldName){
            return cur;
        }
        cur = cur->next;
    }
    return nullptr;
}