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

    cout << "\n=== Combined Test ===" << endl;

    JsonValue* sortFilter = JsonParser::parse(
        "{\"age\":{\"$gt\":20}}"
    );

    List projection;
    projection.pushTail("name");
    projection.pushTail("age");

    storage.find(
        "users",
        sortFilter,
        2,
        &projection,
        "age",
        false
    );

    delete sortFilter;
    /*cout << "\n=== Sort Test ===" << endl;

    JsonValue* filter = JsonParser::parse("{}");

    if (filter != nullptr) {

        cout << "\nAscending:" << endl;

        storage.find(
            "users", filter, 0, nullptr, "age", true
        );

        cout << "\nDescending:" << endl;

        storage.find(
            "users", filter, 0, nullptr, "age", false
        );

        cout << "\nDescending with limit 1:" << endl;

        storage.find(
            "users", filter, 1, nullptr, "age", false
        );
    }

    delete filter;*/


    /*cout << "\n=== DeleteMany Test ===" << endl;

    JsonValue* filter = JsonParser::parse(
        "{\"age\":23}"
    );

    if (filter != nullptr) {
        int count = storage.deleteMany("users", filter);

        cout << "Deleted documents: " << count << endl;
    }

    delete filter;

    JsonValue* all = JsonParser::parse("{}");

    if (all != nullptr) {
        storage.find("users", all);
    }

    delete all;*/



    /*cout << "\n=== DeleteOne Test ===" << endl;

    JsonValue* filter = JsonParser::parse(
        "{\"name\":\"Robert\"}"
    );

    if (filter != nullptr) {
        bool deleted = storage.deleteOne("users", filter);

        cout << "Deleted: "
            << (deleted ? "yes" : "no") << endl;
    }

    delete filter;

    JsonValue* all = JsonParser::parse("{}");

    if (all != nullptr) {
        storage.find("users", all);
    }

    delete all;*/



    /*cout << "\n=== UpdateMany Test ===" << endl;

    JsonValue* filter = JsonParser::parse(
        "{\"age\":{\"$lt\":23}}"
    );

    JsonValue* update = JsonParser::parse(
        "{\"$inc\":{\"age\":1}}"
    );

    if (filter != nullptr && update != nullptr) {
        int count = storage.updateMany("users", filter, update);

        cout << "Updated documents: " << count << endl;
    }

    delete filter;
    delete update;

    JsonValue* all = JsonParser::parse("{}");

    if (all != nullptr) {
        storage.find("users", all);
    }

    delete all;*/



    /*cout << "\n=== UpdateOne Test ===" << endl;

    JsonValue* filter = JsonParser::parse(
        "{\"name\":\"Bob\"}"
    );

    JsonValue* update = JsonParser::parse(
        "{\"$set\":{\"name\":\"Robert\"},"
        "\"$inc\":{\"age\":2}}"
    );

    if (filter != nullptr && update != nullptr) {
        bool updated = storage.updateOne("users", filter, update);

        cout << "Updated: " << (updated ? "yes" : "no") << endl;
    }

    delete filter;
    delete update;

    JsonValue* all = JsonParser::parse("{}");

    if (all != nullptr) {
        storage.find("users", all);
    }

    delete all;*/


    /*cout << "\n CopyFrom Test " << endl;

    JsonValue* document = JsonParser::parse(
        "{\"id\":2,\"name\":\"Bob\",\"age\":21}"
    );

    JsonValue* update = JsonParser::parse(
        "{\"age\":25}"
    );

    if (document != nullptr && update != nullptr) {
        JsonValue* oldAge = document->find("age");
        JsonValue* newAge = update->find("age");

        if (oldAge != nullptr && newAge != nullptr) {
            oldAge->copyFrom(newAge);
        }

        cout << JsonParser::stringify(document) << endl;
    }

    delete document;
    delete update;



    cout << "\n Limit and Projection Test " << endl;

    JsonValue* filter = JsonParser::parse("{}");

    if (filter != nullptr) {

        cout << "\nFirst 2 users:" << endl;

        int count = storage.find("users", filter, 2);

        cout << "Found: " << count << endl;

        List projection;
        projection.pushTail("name");
        projection.pushTail("age");

        cout << "\nNames and ages:" << endl;

        count = storage.find("users", filter, 0, &projection);

        cout << "Found: " << count << endl;

        cout << "\nFirst 3 names:" << endl;

        List namesOnly;
        namesOnly.pushTail("name");

        count = storage.find("users", filter, 3, &namesOnly);

        cout << "Found: " << count << endl;

        delete filter;
    }
*/


    cout << "\n Logical Operators Test " << endl;

    string logicalFilters[] = {
        "{\"$and\":[{\"age\":{\"$gte\":21}},{\"age\":{\"$lte\":23}}]}",

        "{\"$or\":[{\"age\":20},{\"age\":24}]}",

        "{\"$not\":{\"age\":{\"$gt\":22}}}",

        "{\"$and\":[{\"age\":{\"$gte\":21}},"
        "{\"$or\":[{\"name\":\"Bob\"},{\"name\":\"Eva\"}]}]}"
    };

    for (int i = 0; i < 4; i++) {
        JsonValue* filter = JsonParser::parse(logicalFilters[i]);

        if (filter != nullptr) {
            cout << "\nFilter: " << logicalFilters[i] << endl;

            int count = storage.find("users", filter);

            cout << "Found: " << count << endl;

            delete filter;
        }
        else {
            cerr << "Failed to parse filter!" << endl;
        }
    }


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
    /*string json = "{\"id\":2,\"name\":\"Bob\",\"age\":21}";

    JsonValue* document = JsonParser::parse(json);

    if (document != nullptr) {
        storage.insertOne("users", "user_2", document);
        delete document;
    } else {
        cerr << "Failed to parse document!" << endl;
    }


    cout << " DBMS Finished " << endl;*/


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

