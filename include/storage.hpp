#pragma once
#include <string>
#include <fstream>
#include <cstdio>
#include "schema.hpp"
#include "document.hpp"

enum KeySearchResult {
    KEY_NOT_FOUND,
    KEY_FOUND,
    KEY_ERROR
};

class Storage{
private:
    Schema* schema;

    std::string getCollectionPath(const std::string& collectionName) const;
    std::string getFilePath(const std::string& collectionName, int fileNumber) const;

    bool fileExists(const std::string& filename) const;
    bool validateDocument(const Collection* collection, const JsonValue* document) const;
    KeySearchResult keyExists(const std::string& collectionName, const std::string& key) const;
    bool writeFile(const std::string& filename, const std::string& content) const;
    bool matchesFilter(const JsonValue* document, const JsonValue* filter) const;
    int findInternal(const std::string& collectionName, const JsonValue* filter, int limit, const List* projection) const;
    bool compareValues(const JsonValue* first, const JsonValue* second, const std::string& op) const;
    bool matchesCondition(const JsonValue* field, const JsonValue* condition) const;
    bool matchesLogical(const JsonValue* document, const JsonValue* condition) const;
    void printDocument(const JsonValue* document, const List* projection) const;
    JsonValue* storageLoadFile(const std::string& filename) const;
    bool aplyUpdate(JsonValue* document, const JsonValue* update) const;
    bool aplySet(JsonValue* document, const JsonValue* fields) const;
    bool aplyInc(JsonValue* document, const JsonValue* fields) const;
    bool aplyPush(JsonValue* document, const JsonValue* fields) const;
    int updateInternal(const std::string& collectionName, const JsonValue* filter, const JsonValue* update, bool multi);
    int deleteInternal(const std::string& collectionName, const JsonValue* filter, bool multi);
    int compareDocument(const JsonValue* first, const JsonValue* second, const std::string& sortField) const;

    struct SortNode{
        const JsonValue* document;
        SortNode* next;
    };

    SortNode* splitSortList(SortNode* head) const;
    SortNode* mergeSortLists(SortNode* first, SortNode* second, const std::string& sortField, bool ascending) const;
    SortNode* mergeSort(SortNode* head, const std::string& sortField, bool ascending) const;
    void freeSortList(SortNode* head) const;
    int createSortedRun(const std::string& filename, const std::string& tempFilename, const JsonValue* filter, const std::string& sortField, bool ascending) const;
    int readSortedDocument(std::ifstream& file, JsonValue*& document) const;
    bool mergeSortedRuns(const std::string& firstFilename, const std::string& secondFilename, const std::string& outputFilename, const std::string& sortField, bool ascending) const;
    bool writeSortedDocument(std::ofstream& file, const JsonValue* document) const;
    std::string getSortFilePath(const std::string& collectionName, int pass, int number) const;
    void cleanupSortFiles(const std::string& collectionName, int runCount) const;
public:
    explicit Storage(Schema* dbSchema);
    bool insertOne(const std::string& collectionName, const std::string& key, const JsonValue* document);
    int insertMany(const std::string& collectionName, const JsonValue* documents);
    int find(const std::string& collectionName, const JsonValue* filter, int limit = 0, const List* projection = nullptr, const std::string& sortField = "", bool ascending = true) const;
    bool findOne(const std::string& collectionName, const JsonValue* filter) const;
    bool updateOne(const std::string& collectionName, const JsonValue* filter, const JsonValue* update);
    int updateMany(const std::string& collectionName, const JsonValue* filter, const JsonValue* update);
    bool deleteOne(const std::string& collectionName, const JsonValue* filter);
    int deleteMany(const std::string& collectionName, const JsonValue* filter);
    int findSorted(const std::string& collectionName, const JsonValue* filter, int limit, const List* projection, const std::string& sortField, bool acsending) const;
};
