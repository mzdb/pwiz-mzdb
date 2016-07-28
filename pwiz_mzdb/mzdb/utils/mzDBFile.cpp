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
 * @file mzDBFile.cpp
 * @brief Contains `sqlite3 db and statement` pointers plus some helping metadata about the current conversion.
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#include "mzDBFile.h"

using namespace std;

namespace mzdb {

MzDBFile::MzDBFile(string& _name) : bbHeight(0), bbHeightMsn(0), bbWidth(0),
    bbWidthMsn(0), name(_name), sourceFileID(0), sharedParamTreeID(0), instrumentConfigurationID(0),
    dataProcessingID(0), db(0), stmt(0) {

}

MzDBFile::MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn,
                   float _bbWidth, float _bbWidthMsn, bool _noLoss) :
    name(_name),
    bbHeight(_bbHeight),
    bbHeightMsn(_bbHeightMsn),
    bbWidth(_bbWidth),
    bbWidthMsn(_bbWidthMsn),
    sourceFileID(0),
    sharedParamTreeID(0),
    instrumentConfigurationID(0),
    dataProcessingID(0),
    noLoss(_noLoss) {

}

MzDBFile::MzDBFile(string& _name, string& _outputFilename, float _bbHeight, float _bbHeightMsn,
                   float _bbWidth, float _bbWidthMsn, bool _noLoss) :
    name(_name),
    outputFilename(_outputFilename),
    bbHeight(_bbHeight),
    bbHeightMsn(_bbHeightMsn),
    bbWidth(_bbWidth),
    bbWidthMsn(_bbWidthMsn),
    sourceFileID(0),
    sharedParamTreeID(0),
    instrumentConfigurationID(0),
    dataProcessingID(0),
    noLoss(_noLoss) {

}

MzDBFile::~MzDBFile() {
    //handles the closing of the sqlite file
    sqlite3_finalize(stmt);
    sqlite3_close_v2(db);
}

bool MzDBFile::isNoLoss() {

    if (this->userParams.empty()) {

        LOG(INFO) << "Loading mzdb param tree...";
        int sqlCode = sqlite3_prepare_v2(db, "SELECT param_tree FROM mzdb", -1, &stmt, 0);
        if (sqlCode != SQLITE_OK) {
            printf("[MZDBFILE: is no loss]: can not request param tree, this a fatal error... Exiting");
            exit(0);
        }

        sqlCode = sqlite3_step(stmt);

        string result = string((const char *)sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        stmt = 0;

        pugi::xml_document doc;
        pugi::xml_parse_result r = doc.load(result.c_str());
        if ( r.status )
            LOG(ERROR) << "[MZDBFILE: is no loss] There was an error ! pugi_xml exit code:%d\n", r;
        IDeserializer::setUserParams(*this, doc);

        LOG(INFO) << "mzDB userparams set !";
    }
    string isNoLossStr = this->userParam("is_lossless").value;
    if (strcmp(isNoLossStr.c_str(), "false") == 0)
        return false;
    else if ( strcmp(isNoLossStr.c_str(), "true") == 0 )
        return true;
    else {
        LOG(ERROR) <<("Buggy no loss detection ! Set default to false\n");
        return false;
    }
}

pair<double, double> MzDBFile::getBBSize(int msLevel) {
    if (userParams.empty()) {
        sqlite3_prepare_v2(db, "select param_tree from mzdb", -1, &stmt, 0);
        sqlite3_step(stmt);
        string result = string((const char*) sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        stmt = 0;
        pugi::xml_document doc;
        doc.load(result.c_str());
        IDeserializer::setUserParams(*this, doc);
    }

    string userParam_ = (msLevel == 1) ? "ms1_bb_mz_width" : "msn_bb_mz_width";
    const string& bbHeight = userParam(userParam_).value;

    string userParam__ = (msLevel == 1) ? "ms1_bb_time_width" : "msn_bb_time_width";
    const string& bbWidth = userParam(userParam__).value;

    return make_pair(boost::lexical_cast<double>(bbHeight),  boost::lexical_cast<double>(bbWidth));
}

} // end namespace
