#ifndef MZDBFILE_H
#define MZDBFILE_H
/** pwiz includes */
#include "pwiz/data/msdata/MSData.hpp"
#include "iostream"

/** mzdb includes */
#include "../lib/sqlite3/include/sqlite3.h"
#include "mzIDeserializer.h"

namespace mzdb {

struct MzDBFile : public pwiz::msdata::ParamContainer {
    float bbHeight, bbHeightMsn, bbWidth, bbWidthMsn;
    std::string name;
    struct sqlite3* db;
    struct sqlite3_stmt* stmt;

    /**constructors*/
    MzDBFile(string& _name) : name(_name), bbHeight(0), bbHeightMsn(0), bbWidth(0), bbWidthMsn(0), db(0), stmt(0){}

    MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn,
                       float _bbWidth, float _bbWidthMsn) :
        name(_name),
        bbHeight(_bbHeight),
        bbHeightMsn(_bbHeightMsn),
        bbWidth(_bbWidth),
        bbWidthMsn(_bbWidthMsn)
    {

    }

    inline bool isNoLoss() {
        if (this->userParams.empty()) {

            printf("loads mzdb param tree...");
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
            printf("parse status:%s\n", r.description());
            if ( r.status )
                printf("[MZDBFILE: is no loss] There was an error ! pugi_xml exit code:%d\n", r);


            IDeserializer::setUserParams(*this, doc);
            /*pugi::xml_node& node = doc.child("params").child("userParams");
            for (pugi::xml_node_iterator it = node.children().begin(); it != node.children().end(); ++it ) {

                this->userParams.push_back(pwiz::msdata::UserParam(it->attribute("name").value(),
                                                                   it->attribute("value").value(),
                                                                   it->attribute("type").value() ));
            }*/
            printf("mzdb userparams set !\n");
        }
        string isNoLossStr = this->userParam("is_lossless").value;
        if (strcmp(isNoLossStr.c_str(), "false") == 0)
            return false;
        else if ( strcmp(isNoLossStr.c_str(), "true") == 0 )
            return true;
        else {
            printf("Buggy no loss detection ! Set default to false\n");
            return false;
        }
    }



    inline pair<double, double> getBBSize(int msLevel) {
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

    inline ~MzDBFile() {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }

    inline int open(std::string& filename) {
        return sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    }
};

} //end namespace
#endif // MZDBFILE_H
