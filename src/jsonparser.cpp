#include "jsonparser.hpp"
#include <cctype>
#include <climits>

using namespace std;

void JsonParser::skipWhitespace(const std::string& json, size_t& pos){
    while(pos < json.length() && isspace((unsigned char)json[pos])){
        pos++;
    }
}

string JsonParser::parseString(const std::string& json, size_t& pos){
    string result = "";
    if(pos >= json.length() || json[pos] != '"'){
        return "";
    }
    pos++;
    while (pos < json.length()){
        char c = json[pos++];
        if (c == '"'){
            return result;
        }
        if (c == '\\'){
            if (pos >= json.length()){
                return "";
            }
            char escaped = json[pos++];
            switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                default: return "";
            }
        }else{
            if ((unsigned char)c < 32) return "";
            result += c;
        }
    }
    return "";
}

JsonValue* JsonParser::parseObject(const std::string& json, size_t& pos){
    if (pos >= json.length() || json[pos] != '{') return nullptr;
    pos++;

    JsonValue* object = new JsonValue();
    object->setObject();

    skipWhitespace(json, pos);

    if (pos <= json.length() && json[pos] == '}'){
        pos++;
        return object;
    }

    while(pos < json.length()){
        skipWhitespace(json, pos);

        if (pos >= json.length() || json[pos] != '"'){
            delete object;
            return nullptr;
        }

        string key = parseString(json, pos);
        skipWhitespace(json, pos);

        if (pos >= json.length() || json[pos] != ':'){
            delete object;
            return nullptr;
        }

        pos++;
        skipWhitespace(json, pos);
        JsonValue* value = parseValue(json, pos);

        if (value == nullptr){
            delete object;
            return nullptr;
        }

        value->setKey(key);
        object->addChild(value);

        skipWhitespace(json, pos);

        if (pos < json.length() && json[pos] == '}'){
            pos++;
            return object;
        }

        if (pos >= json.length() || json[pos] != ','){
            delete object;
            return nullptr;
        }

        pos++;
    }

    delete object;
    return nullptr;
}

JsonValue* JsonParser::parseArray(const string& json, size_t& pos) {
    if (pos >= json.length() || json[pos] != '[') return nullptr;

    pos++;

    JsonValue* array = new JsonValue();
    array->setArray();

    skipWhitespace(json, pos);

    if (pos < json.length() && json[pos] == ']'){
        pos++;
        return array;
    }

    while (pos < json.length()){
        skipWhitespace(json, pos);

        JsonValue* value = parseValue(json, pos);

        if (value == nullptr){
            delete array;
            return nullptr;
        }

        array->addChild(value);

        skipWhitespace(json, pos);

        if (pos < json.length() && json[pos] == ']'){
            pos++;
            return array;
        }

        if (pos >= json.length() || json[pos] != ','){
            delete array;
            return nullptr;
        }

        pos++;
    }

    delete array;
    return nullptr;
}


JsonValue* JsonParser::parseValue(const string& json, size_t& pos) {
    skipWhitespace(json, pos);

    if (pos >= json.length()) return nullptr;

    if (json[pos] == '{') {
        return parseObject(json, pos);
    }

    if (json[pos] == '[') {
        return parseArray(json, pos);
    }

    if (json[pos] == '"') {
        string text = parseString(json, pos);
        return new JsonValue(text);
    }

    if (json.compare(pos, 4, "null") == 0) {
        pos += 4;
        return new JsonValue();
    }

    if (json[pos] == '-' || isdigit((unsigned char)json[pos])) {
        bool negative = false;

        if (json[pos] == '-') {
            negative = true;
            pos++;
        }

        if (pos >= json.length() ||
            !isdigit((unsigned char)json[pos])) {
            return nullptr;
        }

        long long number = 0;
        long long limit = negative
            ? -(long long)INT_MIN
            : INT_MAX;

        while (pos < json.length() &&
               isdigit((unsigned char)json[pos])) {
            int digit = json[pos] - '0';

            if (number > (limit - digit) / 10) {
                return nullptr;
            }

            number = number * 10 + digit;
            pos++;
        }

        if (negative) number = -number;

        return new JsonValue((int)number);
    }

    return nullptr;
}

JsonValue* JsonParser::parse(const string& json) {
    size_t pos = 0;

    JsonValue* root = parseValue(json, pos);

    if (root == nullptr) return nullptr;

    skipWhitespace(json, pos);

    if (pos != json.length()) {
        delete root;
        return nullptr;
    }

    return root;
}


string JsonParser::escapeString(const string& text) {
    string result = "";

    const char* hex = "0123456789abcdef";

    for (size_t i = 0; i < text.length(); i++) {
        unsigned char c = (unsigned char)text[i];

        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;

            default:
                if (c < 32) {
                    result += "\\u00";
                    result += hex[c >> 4];
                    result += hex[c & 15];
                } else {
                    result += (char)c;
                }
                break;
        }
    }

    return result;
}

string JsonParser::stringify(const JsonValue* value) {
    if (value == nullptr) return "null";

    switch (value->getType()) {
        case JSON_NULL:
            return "null";

        case JSON_INT:
            return value->getValue();

        case JSON_STRING:
            return "\"" + escapeString(value->getValue()) + "\"";

        case JSON_OBJECT: {
            string result = "{";
            JsonValue* cur = value->getChild();

            while (cur != nullptr) {
                if (cur != value->getChild()) result += ",";

                result += "\"" + escapeString(cur->getKey()) + "\":";
                result += stringify(cur);

                cur = cur->getNext();
            }

            result += "}";
            return result;
        }

        case JSON_ARRAY: {
            string result = "[";
            JsonValue* cur = value->getChild();

            while (cur != nullptr) {
                if (cur != value->getChild()) result += ",";

                result += stringify(cur);
                cur = cur->getNext();
            }

            result += "]";
            return result;
        }
    }

    return "null";
}


