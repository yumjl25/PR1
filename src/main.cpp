
#include "schema.hpp"
#include "storage.hpp"
#include "commandparser.hpp"
#include <iostream>

using namespace std;

int main() {
    cout << "DBMS Starting" << endl;

    Schema db_schema;

    if (!db_schema.loadFromFile("schema.json")) {
        cerr << "Cannot load schema" << endl;
        return 1;
    }

    db_schema.createDirectories();

    Storage storage(&db_schema);

    CommandParser parser(&storage);

    string command;

    cout << "\nDBMS started" << endl;
    cout << "Enter a command or 'exit' to quit." << endl;

    while (true) {

        cout << "db> ";

        if (!getline(cin, command)){
            break;
        }

        if (command == "exit"){
            break;
        }

        if (command.empty()){
            continue;
        }

        parser.execute(command);
    }

    cout << "DBMS stopped" << endl;

    return 0;
}
