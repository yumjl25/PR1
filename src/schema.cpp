#include "../include/schema.hpp"
#include <sys/stat.h>
#include <iostream>
#include <cctype>

using namespace std;


static void skip_ws(const string& s, size_t& pos) {
    while (pos < s.length() && isspace((unsigned char)s[pos])) pos++;
}

static string read_string(const string& s, size_t& pos) {
    skip_ws(s, pos);
    if (pos >= s.length() || s[pos] != '"') return ""; 
    pos++; 
    size_t start = pos;
    while (pos < s.length() && s[pos] != '"') pos++; 
    string result = s.substr(start, pos - start);
    if (pos < s.length()) pos++; 
    return result;
}

static int read_int(const string& s, size_t& pos) {
    skip_ws(s, pos);
    int val = 0;
    while (pos < s.length() && isdigit((unsigned char)s[pos])) {
        val = val * 10 + (s[pos] - '0');
        pos++;
    }
    return val;
}


Schema::Schema() : name(""), tuples_limit(0), collection_count(0) {
    for (int i = 0; i < MAX_COLLECTIONS; i++) {
        collections[i] = nullptr;
    }
}

Schema::~Schema() {
    for (int i = 0; i < collection_count; i++) {
        if (collections[i] != nullptr) {
            delete collections[i];
        }
    }
}

Collection* Schema::getCollection(int index) const {
    if (index >= 0 && index < collection_count) {
        return collections[index];
    }
    return nullptr;
}

Collection* Schema::findCollection(const string& collName) const {
    for (int i = 0; i < collection_count; i++) {
        if (collections[i]->getName() == collName) {
            return collections[i];
        }
    }
    return nullptr;
}

bool Schema::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: cannot open " << filename << endl;
        return false;
    }

    string json = "";
    string line;
    while (getline(file, line)) {
        json += line + "\n";
    }
    file.close();

    if (json.empty()) {
        cerr << "Error: file is empty!" << endl;
        return false;
    }

    size_t found_limit = json.find("\"tuples_limit\"");
    if (found_limit != string::npos) {
        size_t pos = found_limit + 14;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\n')) pos++;
        tuples_limit = read_int(json, pos);
    }

    size_t found_name = json.find("\"name\"");
    if (found_name != string::npos && found_name < found_limit) {
        size_t pos = found_name + 6;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\n')) pos++;
        name = read_string(json, pos);
    }

    size_t found_struct = json.find("\"structure\""); 
    if (found_struct != string::npos) {
        size_t pos = found_struct + 11; 
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\n')) pos++;
        
        if (pos < json.length() && json[pos] == '{') {
            pos++;
            
            while (pos < json.length()) {
                while (pos < json.length() && (json[pos] == ' ' || json[pos] == ',' || json[pos] == '\n')) pos++;
                if (pos >= json.length()) break;
                if (json[pos] == '}') { pos++; break; }

                string coll_name = read_string(json, pos);
                if (coll_name.empty()) { pos++; continue; } 

                while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\n')) pos++;

                if (pos < json.length() && json[pos] == '{') {
                    pos++; 
                    Collection* coll = new Collection(coll_name);

                    while (pos < json.length()) {
                        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ',' || json[pos] == '\n')) pos++;
                        if (pos >= json.length()) break;
                        if (json[pos] == '}') { pos++; break; }

                        string field_name = read_string(json, pos);
                        if (field_name.empty()) { pos++; continue; }

                        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\n')) pos++;
                        
                        string field_type = read_string(json, pos);
                        if (field_type.empty()) { pos++; continue; }

                        coll->addField(field_name, field_type);
                    }

                    if (collection_count < MAX_COLLECTIONS) {
                        collections[collection_count++] = coll;
                        collection_names.pushTail(coll_name);
                    }
                }
            }
        }
    }

    cout << "Debug: Parsed Name = '" << name << "', Tuples Limit = " << tuples_limit << endl;
    return true;
}

void Schema::createDirectories() {
    string main_dir = "data/" + name;

    #ifdef _WIN32
        mkdir("data");
        mkdir(main_dir.c_str());
    #else
        mkdir("data", 0777);
        mkdir(main_dir.c_str(), 0777);
    #endif

    for (int i = 0; i < collection_count; i++) {
        Collection* coll = collections[i];
        
        string coll_dir = main_dir + "/" + coll->getName();
        #ifdef _WIN32
            mkdir(coll_dir.c_str());
        #else
            mkdir(coll_dir.c_str(), 0777);
        #endif

        string file_path = coll_dir + "/1.json";
        ofstream out(file_path);
        if (out.is_open()) {
            out << "[]" << endl;
            out.close();
        }
    }
    cout << "Database structure created in 'data/" << name << "'" << endl;
}