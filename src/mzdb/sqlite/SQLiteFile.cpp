#include "SQLiteFile.h"

namespace sqlite {

    using namespace std;

    //using boost format
    string SQLiteFile::Pragma::asSql(bool withValue)  {
        string s = "PRAGMA ";// + name +"=";
        //if (withValue) return s += value + ";";
        //else r
        return s;
    }

    //init static sql request
    const char* SQLiteFile::paramsSql = "select param_tree from mzdb";

    //type name value unit
    const SQLiteFile::Pragma SQLiteFile::availablePragmas[6]={
        {SQLiteFile::Pragma::SYNCHRONOUS, "synchronous", "", SQLiteFile::Pragma::STRING},
        {SQLiteFile::Pragma::JOURNAL_MODE, "journal_mode", "", SQLiteFile::Pragma::STRING},
        {SQLiteFile::Pragma::TEMP_STORE, "temp_store", "", SQLiteFile::Pragma::NUMERIC},
        {SQLiteFile::Pragma::PAGE_SIZE, "page_size", "", SQLiteFile::Pragma::NUMERIC},
        {SQLiteFile::Pragma::DEFAULT_CACHE_SIZE, "default_cache_size", "", SQLiteFile::Pragma::NUMERIC},
        {SQLiteFile::Pragma::CACHE_SIZE, "cache_size", "", SQLiteFile::Pragma::NUMERIC}
    };

    SQLiteFile::SQLiteFile(string& _name): name(_name), db(0), stmt(0) {}

    SQLiteFile::~SQLiteFile() {
        if (stmt) {
            sqlite3_finalize(stmt);
            stmt =0;
        }
        if (db)
            sqlite3_close(db);
    }

    int SQLiteFile::open() {
        //string mzdb = ".mzDB";
        //string f_ = name + mzdb;
        return sqlite3_open_v2(name.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    }

    void SQLiteFile::close() {
        sqlite3_finalize(stmt);
        stmt =0;
        sqlite3_close(db);
    }

    void SQLiteFile::cleanStatement() {
        sqlite3_finalize(stmt);
        stmt = 0;
    }

    int SQLiteFile::prepareStatement(const string& sql) {
        return sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    }

    int SQLiteFile::prepareStatement(const char* sql) {
        return sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    }



    /** try to set a pragma, return sqlite error code if it fails*/
    int SQLiteFile::setPragma(const SQLiteFile::Pragma& pragma, string& value) {
        string str = "";//pragma.asSql() + value +";";
        auto val = sqlite3_exec(db, str.c_str(), 0, 0, 0);
        if ( val == SQLITE_OK) {
            pragmas[pragma.type] = value;
            stmt = 0;
        }
        return val;
    }

    /** try to set a pragma, return sqlite error code if it fails*/
    int SQLiteFile::setPragma(Pragma::PragmaType& type, std::string& s) {
        Pragma p;
        for (size_t i =0; i < 6; ++i) {
            if (type == (SQLiteFile::availablePragmas[i]).type) {
                p = SQLiteFile::availablePragmas[i];
                break;
            }
        }
        string str = "";//p.asSql() + s +";";
        int val = sqlite3_exec(db, str.c_str(), 0, 0, 0);
        if (val == SQLITE_OK) {
            pragmas[type] = s;
            stmt = 0;
        }
        return val;
    }

    /** create a table and return it*/
    SQLiteTable* SQLiteFile::createTable(const string& name) {
        auto* t = new SQLiteTable(name);
        tables.push_back(t);
        return t;
    }

    /** return a table by its name or throw an exception*/
    SQLiteTable* SQLiteFile::getTable(const string& name) {
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            if (strcmp(name.c_str(),(*it)->name.c_str()) == 0)
                return (*it);
        }
        throw exception("table does not exit\n"); // , name.c_str());
    }
}
