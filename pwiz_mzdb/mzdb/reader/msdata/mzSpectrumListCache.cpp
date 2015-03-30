#include "mzSpectrumListCache.h"

namespace mzdb{

using namespace std;
using namespace pwiz::msdata;

mzSpectrumListCache::mzSpectrumListCache(MzDBFile* f, MSData* msdata, mzDataCache* cache) :
    _file(f), _msdata(msdata), _cacheDB(cache) {
    //printf("before populate\n");
    populateSpectrumIdentities();
    //printf("after populate\n");
}


SpectrumPtr mzSpectrumListCache::spectrum(size_t index, bool getBinaryData) const {
    string str = boost::lexical_cast<string> (index);
    SpectrumPtr ptr(new Spectrum);
    _cacheDB->getValue(str, ptr);
    mzSpectrumIdentity* si = (mzSpectrumIdentity*) _identities[index];

    ptr->id = si->id;
    ptr->sourceFilePtr = _msdata->run.defaultSourceFilePtr;

    //cvParams
    ptr->cvParams = si->cvParams;
    ptr->userParams = si->userParams;

    ptr->index = si->idMzDB - 1;
    ptr->defaultArrayLength = ptr->getMZArray()->data.size();
    Scan scan;
    scan.set(MS_retention_time, si->time, UO_second);
    scan.set(MS_scan_start_time, si->time, UO_second);
    ptr->scanList.scans.push_back(scan);
    References::resolve(*ptr, *_msdata);
    return ptr;
}


void mzSpectrumListCache::populateSpectrumIdentities() {
    const char* sql = "SELECT spectrum.*, data_encoding.mode FROM spectrum, data_encoding WHERE spectrum.data_encoding_id = data_encoding.id";
    //printf("before prepare\n");
    sqlite3_prepare_v2(_file->db, sql, -1, &(_file->stmt), 0);
    //printf("after prepare\n");
    while (sqlite3_step(_file->stmt) == SQLITE_ROW) {

        mzSpectrumIdentity *si = new mzSpectrumIdentity;
        int idMzDB = sqlite3_column_int(_file->stmt, 0);
        si->idMzDB = idMzDB;
        si->index = idMzDB - 1;
        si->id = string((const char*) sqlite3_column_text(_file->stmt, 1));
        si->spotID = "";
        si->title = string((const char*) sqlite3_column_text(_file->stmt, 2));
        si->cycle = sqlite3_column_int(_file->stmt, 3);
        si->time = static_cast<float> (sqlite3_column_double(_file->stmt, 4));
        si->msLevel = sqlite3_column_int(_file->stmt, 5);

        int type = sqlite3_column_type(_file->stmt, 6);
        switch (type) {
        case 3:
            si->activation = string((const char*) sqlite3_column_text(_file->stmt, 6)); //_stmt->GetColumnString(6);
            break;
        case 5:
            si->activation = "";
            break;
        default:
            printf("Error getting spectrumIdentity(activation)");
        }
        si->tic = sqlite3_column_double(_file->stmt, 7);
        int type_ = sqlite3_column_type(_file->stmt, 8);
        switch (type_) {
        case 2:
            si->basePeakMz = sqlite3_column_double(_file->stmt, 8);
            break;
        case 5:
            si->basePeakMz = -1;
            break;
        default:
            printf("Error getting spectrumIdentity(basePeakMz)\n");
        }
        if (type_ == 5)
            si->basePeakIntensity = -1;
        else {
            si->basePeakIntensity = sqlite3_column_double(_file->stmt, 9);
        }

        int type__ = sqlite3_column_type(_file->stmt, 10);
        //printf("type:%d\n", type__);
        switch (type__) {
        case 2:
            si->precursorMz = sqlite3_column_double(_file->stmt, 10); ;
            break;
        case 5:
            si->precursorMz = -1;
            break;
        default:
            printf("Error getting spectrumIdentity(precursorMz)");
        }
        if (type__ == 5)
            si->precursorCharge = -1;
        else
            si->precursorCharge = sqlite3_column_double(_file->stmt, 11);
        si->dataPointsCount = sqlite3_column_int(_file->stmt, 12);

        //read paramTree
        const pugi::char_t* paramTree = (const pugi::char_t*) sqlite3_column_text(_file->stmt, 13);
        IDeserializer::setParams(*si, paramTree);
        si->scansParamTree = (const char*)sqlite3_column_text(_file->stmt, 14);
        si->precursorsParamTree = (const char*)sqlite3_column_text(_file->stmt, 15);
        si->productsParamTree = (const char*)sqlite3_column_text(_file->stmt, 16);

        si->isInHighRes = (si->userParam("in_high_res").value == "true") ? true : false;
        PeakEncoding pe;
        //printf("hola\n");
        if (_file->isNoLoss())
            pe = NO_LOSS_PEAK;
        else if (si->isInHighRes)
            pe = HIGH_RES_PEAK;
        else {
            pe = LOW_RES_PEAK;
        }
        si->sourceFileID = sqlite3_column_int(_file->stmt, 19);
        si->dataProcessingID = sqlite3_column_int(_file->stmt, 21);
        si->dataEncodingID = sqlite3_column_int(_file->stmt, 22);
        si->bbFirstScanID = sqlite3_column_int(_file->stmt, 23);
        DataMode mode;
        string dataEncodingString = string((const char*) sqlite3_column_text(_file->stmt, 24));
        if (dataEncodingString == "centroided")
            mode = CENTROID;
        else if (dataEncodingString == "profile")
            mode = CENTROID; // treated a the same than centroid mode
        else if (dataEncodingString == "fitted")
            mode = FITTED;
        else
            printf("Unknown encoding mode: error in buildSpectrumIdentities\n");
        //add stuff
        si->encodingMode = DataEncoding(idMzDB, mode, pe);
        _identities.push_back(si);
    }
    sqlite3_finalize(_file->stmt);
    _file->stmt = 0;
    //_file->cleanStatement();
}

const SpectrumIdentity& mzSpectrumListCache::spectrumIdentity(size_t index) const {
    return *_identities[index];
}


}
