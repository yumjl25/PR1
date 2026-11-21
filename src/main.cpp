#include "schema.hpp"
#include "jsonparser.hpp"
#include "storage.hpp"
#include <iostream>

using namespace std;

int main() {
    cout << "DBMS Starting" << endl;

    Schema db_schema;

    cout << "Loading schema from 'schema.json'..." << endl;
    if (!db_schema.loadFromFile("schema.json")) {
        cerr << "Failed to load schema. Exiting." << endl;
        return 1;
    }

    cout << "\n Schema Info " << endl;
    cout << "Name: " << db_schema.getName() << endl;
    cout << "Tuples Limit: " << db_schema.getTuplesLimit() << endl;
    cout << "Collections (" << db_schema.getCollectionCount() << "):" << endl;
    
    for (int i = 0; i < db_schema.getCollectionCount(); i++) {
        Collection* coll = db_schema.getCollection(i);
        cout << "  - " << coll->getName() << " (" << coll->getFieldCount() << " fields):" << endl;
        
        for (int j = 0; j < coll->getFieldCount(); j++) {
            cout << "      " << coll->getFieldName(j) << " : " << coll->getFieldType(j) << endl;
        }
    }

    cout << "\nCreating filesystem structure..." << endl;
    db_schema.createDirectories();


    cout << "\n Storage Test " << endl;

    Storage storage(&db_schema);

    cout << "\n Comparison Operators Test " << endl;

    string testFilters[] = {
        "{\"age\":{\"$gt\":21}}",
        "{\"age\":{\"$lte\":22}}",
        "{\"age\":{\"$ne\":21}}",
        "{\"age\":{\"$in\":[20,22,24]}}",
        "{\"age\":{\"$gte\":21,\"$lte\":23}}"
    };

    for (int i = 0; i < 5; i++) {
        JsonValue* filter = JsonParser::parse(testFilters[i]);

        if (filter != nullptr) {
            cout << "\nFilter: " << testFilters[i] << endl;

            int count = storage.find("users", filter);

            cout << "Found: " << count << endl;

            delete filter;
        }
    }

    cout << "\n Find Test " << endl;

    JsonValue* filter1 = JsonParser::parse("{}");

    if (filter1 != nullptr) {
        cout << "\nAll users:" << endl;
        int count = storage.find("users", filter1);
        cout << "Found: " << count << endl;

        delete filter1;
    }

    JsonValue* filter2 = JsonParser::parse("{\"name\":\"Alice\"}");

    if (filter2 != nullptr) {
        cout << "\nFind Alice:" << endl;
        int count = storage.find("users", filter2);
        cout << "Found: " << count << endl;

        delete filter2;
    }

    JsonValue* filter3 = JsonParser::parse("{\"age\":21}");

    if (filter3 != nullptr) {
        cout << "\nFind one user aged 21:" << endl;
        bool found = storage.findOne("users", filter3);
        cout << "Found one: " << (found ? "yes" : "no") << endl;

        delete filter3;
    }


    //string json = "{\"id\":1,\"name\":\"Alice\",\"age\":20}";
    string json = "{\"id\":2,\"name\":\"Bob\",\"age\":21}";

    JsonValue* document = JsonParser::parse(json);

    if (document != nullptr) {
        storage.insertOne("users", "user_2", document);
        delete document;
    } else {
        cerr << "Failed to parse document!" << endl;
    }


    cout << " DBMS Finished " << endl;


    /*cout << "\n Insert Many Test " << endl;

    string manyJson =
        "{"
        "\"user_3\":{\"id\":3,\"name\":\"Charlie\",\"age\":22},"
        "\"user_4\":{\"id\":4,\"name\":\"Diana\",\"age\":23},"
        "\"user_5\":{\"id\":5,\"name\":\"Eva\",\"age\":24}"
        "}";

    JsonValue* documents = JsonParser::parse(manyJson);

    if (documents != nullptr) {
        int inserted = storage.insertMany("users", documents);

        cout << "Successfully inserted: "
            << inserted << endl;

        delete documents;
    } else {
        cerr << "Failed to parse documents!" << endl;
    }*/

}

