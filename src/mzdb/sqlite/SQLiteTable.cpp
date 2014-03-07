#include "SQLiteTable.h"

namespace sqlite {
using namespace std;

string SQLiteTable::sql() {
    string s = "CREATE TABLE ";
    //s += (name + " (");
    for (auto it = fields.begin(); it != fields.end(); ++it) {

    }
    return s;
}

SQLiteTable* SQLiteTable::withFields(SQLiteTable::Field []) {
    return 0;

}

SQLiteTable::Field& SQLiteTable::getField(const std::string& name) {
    SQLiteTable::Field f;
    return f;
}

}
