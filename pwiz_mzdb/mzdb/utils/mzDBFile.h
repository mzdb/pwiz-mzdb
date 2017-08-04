/*
 * Copyright 2014 CNRS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file mzDBFile.h
 * @brief Contains `sqlite3 db and statement` pointers plus some helping metadata about the current conversion.
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZDBFILE_H
#define MZDBFILE_H

#include "iostream"

/** pwiz includes */
#include "pwiz/data/msdata/MSData.hpp"

/** mzdb includes */
#include "../lib/sqlite3/sqlite3.h"
#include "mzIDeserializer.h"
#include "glog/logging.h"

namespace mzdb {

/**
 * MzDBFile structure
 * ==================
 *
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

    /// ID of last inserted `sharedParamTree` object in the `sqlitedb`
    /// Primary to avoid foreign keys constraint problems
    int sharedParamTreeID;

    /// Boolean set to true if the user wants to perform a no loss conversion
    /// i.e. m/z encoded 64 bits and intensity 64 bits
    bool noLoss;

    ///// mzDB filename
    //std::string name;
    /// input filename
    std::string name;
    
    /// output filename
    std::string outputFilename;

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
    //MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn, float _bbWidth, float _bbWidthMsn, bool noLoss=false);
    MzDBFile(string& _name, string& _outputFilename, float _bbHeight, float _bbHeightMsn, float _bbWidth, float _bbWidthMsn, bool noLoss=false);

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
    
    inline int close() {
        sqlite3_finalize(stmt);
        return sqlite3_close_v2(db);
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
