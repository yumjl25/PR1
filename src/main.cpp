
#include "schema.hpp"
#include "storage.hpp"
#include "commandparser.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>

using namespace std;

bool lockDatabase(const string& path){
    int fd = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);

    if (fd == -1){
        if (errno == EEXIST){
            cerr << "Database is locked" << endl;
        }else{
            cerr << "Cannot create lock file" << endl;
        }
        return false;
    }

    close(fd);
    return true;
}

void unlockDatabase(const string& path){
    if (remove(path.c_str()) != 0){
        cerr << "Cannot remove lock file" << endl;
    }
}

int main() {
    cout << "DBMS Starting" << endl;

    Schema db_schema;

    if (!db_schema.loadFromFile("schema.json")) {
        cerr << "Cannot load schema" << endl;
        return 1;
    }

    string lockPath = "database.lock";

    if (!lockDatabase(lockPath)){
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

    unlockDatabase(lockPath);

    return 0;
}
