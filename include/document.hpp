#pragma once
#include <string>

enum JsonType{
    JSON_NULL,
    JSON_STRING,
    JSON_INT,
    JSON_OBJECT,
    JSON_ARRAY
};

class JsonValue{
private:
    JsonType type;
    std::string value;

    JsonValue* child;
    JsonValue* next;

    std::string key;
public:
    JsonValue();
    explicit JsonValue(int number);
    explicit JsonValue(const std::string& text);
    ~JsonValue();

    JsonValue(const JsonValue&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;

    JsonType getType() const;
    std::string getValue() const;
    std::string getKey() const;

    JsonValue* getChild() const;
    JsonValue* getNext() const;

    void setKey(const std::string& newKey);
    void setInt(int number);
    void setString(const std::string& text);
    void setObject();
    void setArray();

    void addChild(JsonValue* node);
    JsonValue* find(const std::string& fieldName) const;
    bool copyFrom(const JsonValue* other);
};