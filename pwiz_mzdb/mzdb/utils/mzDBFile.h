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
 * Contains `sqlite3 db and statement` pointers plus some helping metadata about the current conversion.
 * Can also read user_params XML chunk of the `mzdb table` when used in `reading mode`.
 *
 * @see pwiz::msdata::ParamContainer
 */
struct MzDBFile : public pwiz::msdata::ParamContainer {

    /// ``Height`` (m/z dimension) for bounding box with a MS level = 1
    float bbHeight;

    /// ``Height`` (m/z dimension) for bounding box with a MS level > 1
    float bbHeightMsn;

    ///``Width`` (retention dimension) for bounding box with MS level = 1
    float bbWidth;

    ///``Width`` (retention dimension) for bounding box with MS level > 1
    float bbWidthMsn;

    /// last row ID of corresponding table in sqlite file.

    /// ID of last inserted `SourceFile` object in the `sqlitedb`
    /// Primary to avoid foreign keys constraint problems
    int sourceFileID;

    /// ID of last inserted `DataProcessing` object in the `sqlitedb`
    /// Primary to avoid foreign keys constraint problems
    int dataProcessingID;

    /// ID of last inserted `InstrumentConfiguration` object in the `sqlitedb`
    /// Primary to avoid foreign keys constraint problems
    int instrumentConfigurationID;

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
     * Short constructor
     *
     * @param name raw filename
     */
    MzDBFile(string& name);

    /**
     * Full constructor
     *
     * @param _name raw file name
     * @param _bbHeight m/z dimension in Da for msLevel=1
     * @param _bbHeightMsn m/z dimension in Da for msLevel > 1
     * @param _bbWidth retention time dimension in seconds for msLevel=1
     * @param _bbWidthMsn retention time dimension in seconds for msLevel > 1
     * @param noLoss if true noLoss conversion, by default is set to false
     */
    MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn, float _bbWidth, float _bbWidthMsn, bool noLoss=false);

    /**
     * Close properly sqlite db connection and statement
     */
    ~MzDBFile();

    /**
     * Create a new database with the specified filename.
     *
     * @param filename mzDB filename
     * @return ``sqlite code`` of opening/creating the sqlite DB
     */
    inline int open(std::string& filename) {
        return sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    }

    /**
     * load `user_params` mzdb table to find the `noLoss user param`
     * this method is useful if we are in ``reading mode``.
     *
     * @see ParamContainer
     * @return true if the file is in noLoss mode else false
     */
    bool isNoLoss();

    /**
     * load `user_params` of mzdb table to get the size of the bounding boxes for a specified MS level.
     *
     * @param msLevel: fetch bounding boxes sizes for the specified msLevel
     * @return pair<double, double> bounding box height (m/z dimension), bounding box width (retention time dimension)
     */
    pair<double, double> getBBSize(int msLevel);

};

} //end namespace
#endif // MZDBFILE_H
