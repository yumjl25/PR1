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

bool JsonValue::copyFrom(const JsonValue* other){
    if (other == nullptr) return false;
    if (this == other) return true;

    JsonValue* newChild = nullptr;
    JsonValue* newTail = nullptr;

    const JsonValue* current = other->getChild();

    while (current != nullptr){
        JsonValue* node = new JsonValue();
        if (!node->copyFrom(current)){
            delete node;
            delete newChild;
            return false;
        }

        node->setKey(current->getKey());


        if (newChild == nullptr){
            newChild = node;
        }else{
            newTail->next = node;
        }

        newTail = node;
        current = current->next;
    }
    delete child;
    type = other->type;
    value = other->value;
    child = newChild;

    return true;
}

bool JsonValue::removeChild(JsonValue* node){
    if (node == nullptr) return false;

    if (type != JSON_OBJECT && type != JSON_ARRAY) return false;

    JsonValue* current = child;
    JsonValue* previous = nullptr;

    while(current != nullptr){
        if (node == current){
            if (previous == nullptr){
                child = current->next;
            }else{
                previous->next = current->next;
            }

            current->next = nullptr;

            delete current;
            return true;
        }

        previous = current;
        current = current->next;
    }
    return false;
}