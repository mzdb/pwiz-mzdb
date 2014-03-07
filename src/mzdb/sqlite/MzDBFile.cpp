#include "MzDBFile.h"

namespace mzdb {
using namespace std;

MzDBFile::MzDBFile(string& _name) : name(_name), bbHeight(0), bbHeightMsn(0), bbWidth(0), bbWidthMsn(0), db(0), stmt(0){}

MzDBFile::MzDBFile(string& _name, float _bbHeight, float _bbHeightMsn,
                   float _bbWidth, float _bbWidthMsn) :
    name(_name),
    bbHeight(_bbHeight),
    bbHeightMsn(_bbHeightMsn),
    bbWidth(_bbWidth),
    bbWidthMsn(_bbWidthMsn)
{

}

bool MzDBFile::isNoLoss() {
    if (userParams.empty()) {
        sqlite3_prepare_v2(db, "select param_tree from mzdb", -1, &stmt, 0);
        sqlite3_step(stmt);
        const pugi::char_t* result = (const pugi::char_t *) sqlite3_column_text(stmt, 0);
        sqlite3_finalize(stmt);
        stmt = 0;
        pugi::xml_document doc;
        doc.load(result);
        Deserialization::setUserParams(*this, doc);
    }
    string isNoLossStr = userParam("is_no_loss").value;
    if (strcmp(isNoLossStr.c_str(), "false") == 0)
        return false;
    return true;
}



pair<double, double> MzDBFile::getBBSize(int msLevel) {
    if (userParams.empty()) {
        sqlite3_prepare_v2(db, "select param_tree from mzdb", -1, &stmt, 0);
        sqlite3_step(stmt);
        const pugi::char_t *result = (const pugi::char_t *) sqlite3_column_text(stmt, 0);
        sqlite3_finalize(stmt);
        stmt = 0;
        pugi::xml_document doc;
        doc.load(result);
        Deserialization::setUserParams(*this, doc);
    }

    string userParam_ = (msLevel == 1) ? "BB_height_ms1" : "BB_height_msn";
    string& bbHeight = userParam(userParam_).value;

    string userParam__ = (msLevel == 1) ? "BB_width_ms1" : "BB_width_msn";
    string& bbWidth = userParam(userParam__).value;

    return make_pair(boost::lexical_cast<double>(bbHeight),  boost::lexical_cast<double>(bbWidth));

}

}
