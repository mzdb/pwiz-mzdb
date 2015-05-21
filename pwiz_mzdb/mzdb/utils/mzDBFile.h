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

/**
 * @brief The MzDBFile struct
 * Contains sqlite3 db and statement pointers plus some metadata about the conversion
 * Can also read user_params of the mzdb table when used in `reading mode`.
 * @see ParamContainer
 */
struct MzDBFile : public pwiz::msdata::ParamContainer {

    /// Dimensions of ``bounding box``
    /// `Width` corresponds to th rt dimension and `height` to the mz dimension.
    /// There are two kinds of bouning box, one for MS1 and one for MSn
    float bbHeight, bbHeightMsn, bbWidth, bbWidthMsn;

    /// Boolean set to true if the user wants to perform a no loss conversion
    /// i.e. m/z encoded 64 bits and intensity 64 bits
    bool noLoss;

    /// mzDB filename
    std::string name;

    /// Pointer to the sqlite DB
    struct sqlite3* db;

    /// Pointer to a sqlite statement
    struct sqlite3_stmt* stmt;

    /**
     * @brief MzDBFile
     * Short constructor
     *
     * @param name: string raw filename
     */
    MzDBFile(string& name);

    /**
     * @brief MzDBFile
     * Full constructor
     *
     * @param _name: string raw file name
     * @param _bbHeight: m/z dimension in Da for msLevel=1
     * @param _bbHeightMsn: m/z dimension in Da for msLevel > 1
     * @param _bbWidth: rt dimension in seconds for msLevel=1
     * @param _bbWidthMsn: rt dimension in seconds for msLevel > 1
     * @param noLoss: bool if true noLoss conversion, by default is set to false
     */
    MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn, float _bbWidth, float _bbWidthMsn, bool noLoss=false);

    /**
     * @brief MzDBFile destructor
     * Close properly sqlite db connection and statement
     */
    ~MzDBFile();

    /**
     * @brief open
     * Create a new database with the specified filename.
     *
     *
     * @param filename: string mzDB filename
     * @return int: sqlite code of opening/creating the DB
     */
    inline int open(std::string& filename) {
        return sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    }

    /**
     * @brief isNoLoss
     * load user_params mzdb table to find the `noLoss user param`
     * @see ParamContainer
     * Useful when reading file
     *
     * @return bool true if the file is in noLoss mode else false
     */
    bool isNoLoss();

    /**
     * @brief getBBSize
     * load user_params of mzdb table to get the size of the bounding boxes
     *
     * @param msLevel: fetch bounding boxes sizes for the specified msLevel
     * @return pair<double, double>: m/z dimension, rt dimension
     */
    pair<double, double> getBBSize(int msLevel);

};

} //end namespace
#endif // MZDBFILE_H
