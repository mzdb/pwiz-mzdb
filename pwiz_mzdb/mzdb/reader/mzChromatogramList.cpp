#include "mzChromatogramList.h"
#include "../../lib/sqlite3/include/sqlite3.h"
#include "mzIBlobReader.hpp"
#include "pwiz/data/msdata/References.hpp"

namespace  mzdb {
    using namespace std;

mzChromatogramList::mzChromatogramList(MzDBFile* f, pwiz::msdata::MSData* msdata): _mzdb(f), _msdata(msdata) {
    //fill ChromatogramIdentities vector
    sqlite3_prepare_v2(_mzdb->db, "SELECT chromatogram.* from chromatogram", -1, &(_mzdb->stmt), 0);
    while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
        pwiz::msdata::ChromatogramIdentity* ci = new pwiz::msdata::ChromatogramIdentity;
        int mzdbId = sqlite3_column_int(_mzdb->stmt, 0);
        ci->id = string((const char*)sqlite3_column_text(_mzdb->stmt, 1));
        ci->index = mzdbId;
        _chromatogramIdentities.push_back(ci);
    }

    initIteration();

}

mzChromatogramList::~mzChromatogramList() {
    for (size_t i =0; i < _chromatogramIdentities.size(); ++i) {
        delete _chromatogramIdentities[i];
    }
}

size_t mzChromatogramList::size() const {
    return _chromatogramIdentities.size();
}

const pwiz::msdata::ChromatogramIdentity& mzChromatogramList::chromatogramIdentity(size_t index) const {
    return *(const_cast<mzChromatogramList*>(this)->_chromatogramIdentities[index]);
}

pwiz::msdata::ChromatogramPtr mzChromatogramList::chromatogram(size_t index, bool getBinaryData) const {
    return (const_cast<mzChromatogramList*>(this))->nextChromatogram();
}

pwiz::msdata::ChromatogramPtr mzChromatogramList::nextChromatogram() {
    if (sqlite3_step(_mzdb->stmt) == SQLITE_OK) {

        int chromID = sqlite3_column_int(_mzdb->stmt, 0);

        byte* data = (byte*) sqlite3_column_blob(_mzdb->stmt, 1);
        size_t size = (size_t)sqlite3_column_bytes(_mzdb->stmt, 1);

        const pugi::char_t* s = (const pugi::char_t*) sqlite3_column_text(_mzdb->stmt, 2);

        pwiz::msdata::ChromatogramIdentity* si = _chromatogramIdentities[chromID - 1];
        int nbPoints = IBlobReader::get_2<int>(0, data, size);
        //encoded in pwiz as array of double
        vector<double> rt, inten;
        int step = 2 * sizeof(float);
        for (int i = sizeof(int);  i < step * nbPoints; i += step) {
            rt.push_back(static_cast<double>(IBlobReader::get_2<float>(i, data, size)));
            inten.push_back(static_cast<double>(IBlobReader::get_2<float>(i + sizeof(float), data, size)));
        }

        pwiz::msdata::ChromatogramPtr ptr(new pwiz::msdata::Chromatogram);
        ptr->dataProcessingPtr = dp_;
        ptr->defaultArrayLength = _chromatogramIdentities.size();
        ptr->setTimeIntensityArrays(rt, inten, pwiz::msdata::UO_second, pwiz::msdata::CVID_Unknown);
        ptr->id = si->id;
        ptr->index = si->index;
        IDeserializer::setParams(*ptr, s);
        pwiz::msdata::References::resolve(*ptr, *_msdata);
        return ptr;
    }
    throw exception("Unknown error in chromatogram iterator\n");

}

void mzChromatogramList::initIteration() {
    const char* sql = "SELECT data_points FROM chromatogram";
    sqlite3_prepare_v2(_mzdb->db, sql, -1, &(_mzdb->stmt), 0);
}




}//end namespace
