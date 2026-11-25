#pragma once
#include "document.hpp"
#include <string>

class JsonParser{
private:
    static void skipWhitespace(const std::string& json, size_t& pos);
    static bool parseString(const std::string& json, size_t& pos, std::string& result);
    static JsonValue* parseValue(const std::string& json, size_t& pos);
    static JsonValue* parseObject(const std::string& json, size_t& pos);
    static JsonValue* parseArray(const std::string& json, size_t& pos);

    static std::string escapeString(const std::string& text);
public:
    static JsonValue* parse(const std::string& json);
    static std::string stringify(const JsonValue* value);
};