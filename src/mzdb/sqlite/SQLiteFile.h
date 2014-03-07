#ifndef SQLITEFILE_H
#define SQLITE_FILE_H

#include "SQLiteTable.h" //include sqlite, map
#include <vector>

namespace sqlite {

struct SQLiteFile {

    struct Pragma {

        enum PragmaType {
            SYNCHRONOUS,
            JOURNAL_MODE,
            TEMP_STORE,
            PAGE_SIZE,
            DEFAULT_CACHE_SIZE,
            CACHE_SIZE
        };

        enum Unit {
            NUMERIC,
            STRING
        };

        PragmaType type;
        std::string name;
        std::string value; //optional
        Unit unit; //optional

        //TODO test if numeric or string
        void setValue(const std::string& val) {value = val;}
        std::string asSql(bool withValue = false);
    };

    static const char* paramsSql;
    static const Pragma availablePragmas[6];

    struct sqlite3* db;
    struct sqlite3_stmt* stmt;
    std::string name;

    std::map<Pragma::PragmaType, std::string> pragmas;
    std::vector<SQLiteTable*> tables;

    SQLiteFile(std::string& _name);
    ~SQLiteFile();

    int open();
    void close();

    /**statement stuff*/
    void cleanStatement();
    int prepareStatement(const std::string& sql);
    int prepareStatement(const char* sql);


    /**metadata information*/
    int setPragma(const Pragma&, std::string&);
    int setPragma(Pragma::PragmaType&, std::string&);

    SQLiteTable* createTable(const std::string& name);
    SQLiteTable* getTable(const std::string& name);


};

}
#endif
