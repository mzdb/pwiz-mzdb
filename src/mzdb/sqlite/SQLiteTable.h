#ifndef SQLTABLE_H
#define SQLTABLE_H

#include "../lib/sqlite3/include/sqlite3.h"
#include <map>

namespace sqlite {
    /** a sqlite table */
    struct SQLiteTable {

        struct Field{

            enum Type {
                SQL_INTEGER,
                SQL_FLOAT,
                SQL_TEXT,
                SQL_BLOB,
                SQL_NULL
            };

            std::string name;
            Type type;
            bool notNull;
            bool isPrimaryKey;
            bool isForeignKey;
        };

        std::map<size_t, SQLiteTable::Field> fields;
        const std::string name;

        std::string sql();

        SQLiteTable(const std::string& name): name(name) {}

        SQLiteTable* withFields(SQLiteTable::Field []);

        SQLiteTable::Field& getField(const std::string& name);

    };

}


#endif // SQLTABLE_H
