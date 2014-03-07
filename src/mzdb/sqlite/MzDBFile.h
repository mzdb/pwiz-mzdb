#ifndef MZDBFILE_H
#define MZDBFILE_H


/** pwiz includes */
#include "pwiz/data/common/ParamTypes.hpp"

/** mzdb includes */
#include "../lib/sqlite3/include/sqlite3.h"
#include "../utils/mzDeserializerInterface.h"

namespace mzdb {

struct MzDBFile : public pwiz::data::ParamContainer {
    float bbHeight, bbHeightMsn, bbWidth, bbWidthMsn;
    std::string name;
    struct sqlite3* db;
    struct sqlite3_stmt* stmt;

    /**constructors*/
    MzDBFile(std::string& _name);
    MzDBFile(std::string&, float, float, float, float);

    ~MzDBFile() {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    /**methods*/
    bool isNoLoss();
    std::pair<double, double> getBBSize(int msLevel);

    inline int open(std::string& filename) {
        return sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    }
};

} //end namespace


#endif // MZDBFILE_H
