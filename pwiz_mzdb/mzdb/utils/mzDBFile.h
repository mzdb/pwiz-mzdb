#ifndef MZDBFILE_H
#define MZDBFILE_H

#include "iostream"

/** pwiz includes */
#include "pwiz/data/msdata/MSData.hpp"

/** mzdb includes */
#include "../lib/sqlite3/include/sqlite3.h"
#include "mzIDeserializer.h"
#include "glog/logging.h"

namespace mzdb {

struct MzDBFile : public pwiz::msdata::ParamContainer {
    float bbHeight, bbHeightMsn, bbWidth, bbWidthMsn;
    std::string name;
    struct sqlite3* db;
    struct sqlite3_stmt* stmt;

    /**constructors*/
    MzDBFile(string& _name);
    MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn, float _bbWidth, float _bbWidthMsn);

    ~MzDBFile();

    //----------------------------------------------------------------------------------------------
    //open a new sqlite file
    inline int open(std::string& filename) {
        return sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    }

    //check if isNoLoss encoding mode is enabled
    bool isNoLoss();

    pair<double, double> getBBSize(int msLevel);


};

} //end namespace
#endif // MZDBFILE_H
