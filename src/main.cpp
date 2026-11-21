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

    string json = "{\"id\":1,\"name\":\"Alice\",\"age\":20}";

    JsonValue* document = JsonParser::parse(json);

    if (document != nullptr) {
        storage.insertOne("users", "user_1", document);
        delete document;
    } else {
        cerr << "Failed to parse document!" << endl;
    }


    cout << " DBMS Finished " << endl;
}