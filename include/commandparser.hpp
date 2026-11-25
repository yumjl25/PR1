#pragma once
#include "storage.hpp"
#include "list.hpp"
#include <string>

class CommandParser{
private:
    Storage* storage;
    std::string trim(const std::string& text) const;
    bool parseCommand(const std::string& command, std::string& collection, std::string& operation, std::string& arguments) const;
    bool splitArguments(const std::string& text, List& arguments) const;
    JsonValue* parseArgument(const List& arguments, int index) const;
    bool executeFind(const std::string& collection, const std::string& operation, const List& arguments);
    bool executeUpdate(const std::string& collection, const std::string& operation, const List& arguments);
    bool executeDelete(const std::string& collection, const std::string& operation, const List& arguments);
    bool executeInsert(const std::string& collection, const std::string& operation, const List& arguments);
public:
    explicit CommandParser(Storage* storage);
    bool execute(const std::string& command);
};