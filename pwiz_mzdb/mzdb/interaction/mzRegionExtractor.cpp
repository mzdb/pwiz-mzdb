#include <boost/algorithm/string.hpp>

#include "mzRegionExtractor.h"
#include "../reader/mzBlobReaderInterface.hpp"
#include "../lib/sqlite3/include/sqlite3.h"
#include "../utils/MzDBFile.h"

namespace mzdb {
using namespace std;

const char *mzRegionExtractor::sqlRTree = "SELECT bounding_box.* FROM bounding_box WHERE bounding_box.id IN (SELECT id FROM bounding_box_rtree WHERE min_mz >= ? AND max_mz <= ? AND min_time >= ? AND max_time <= ?);";
const char *mzRegionExtractor::sqlRunSlice = "SELECT bounding_box.* FROM bounding_box, run_slice WHERE bounding_box.run_slice_id = run_slice.id AND run_slice.begin_mz >= ? AND run_slice.end_mz <= ?";


PWIZ_API_DECL mzRegionExtractor::mzRegionExtractor(MzDBFile* mzdb) : _mzdb(mzdb){
    /** SQLITE does not support by default URI filenames */
    /*vector<string> r;
            boost::split(r, m->run.defaultSourceFilePtr->location, boost::is_any_of("///"));
            r.erase(r.begin());
            r.push_back(m->run.defaultSourceFilePtr->name);
            string filename = boost::join(r, "/");*/
    sqlite3_open_v2(_mzdb->name.c_str(), &(_mzdb->db), SQLITE_OPEN_READONLY, 0);

    isNoLoss = _mzdb->isNoLoss();//mzDBReader::isNoLoss(_mzdb->db, _mzdb->stmt);

    fillInternalMaps(); //fill msLevelbyid and rtbyid
}



void mzRegionExtractor::fillInternalMaps() {
    const char* sql_ = "SELECT spectrum.id, time, ms_level, spectrum.param_tree, data_encoding.mode FROM spectrum, data_encoding WHERE spectrum.data_encoding_id = data_encoding.id";
    sqlite3_prepare_v2(_mzdb->db, sql_, -1, &(_mzdb->stmt), 0);

    while ( sqlite3_step(_mzdb->stmt) == SQLITE_ROW ) {
        int id = sqlite3_column_int(_mzdb->stmt, 0);
        float time = (float)sqlite3_column_double(_mzdb->stmt, 1);
        int msLevel = sqlite3_column_int(_mzdb->stmt, 2);
        msLevelByID[id] = msLevel;
        rtByID[id] = time;
        //determine the encoding
        bool isInHighRes = (string((const char*) sqlite3_column_text(_mzdb->stmt, 3)).find("true") != string::npos) ? true : false;
        PeakEncoding pe;

        if (isNoLoss) {
            pe = NO_LOSS_PEAK;
        } else if (isInHighRes) {
            pe = HIGH_RES_PEAK;
        } else {
            pe = LOW_RES_PEAK;
        }

        DataMode mode;
        string dataEncodingString = string((const char*) sqlite3_column_text(_mzdb->stmt, 4));
        if (dataEncodingString == "centroided")
            mode = CENTROID;
        else if (dataEncodingString == "profile")
            mode = CENTROID; // treated a the same than centroid mode
        else if (dataEncodingString == "fitted")
            mode = FITTED;
        else
            printf("Unknown encoding mode: error in buildSpectrumIdentities\n");
        //fill dataEncodings Map
        dataEncodings[id] = DataEncoding(id, mode, pe);
    }
    sqlite3_finalize(_mzdb->stmt);
    _mzdb->stmt = 0;
}

PWIZ_API_DECL void mzRegionExtractor::rTreeExtraction(double minmz, double maxmz, double minrt, double maxrt, int msLevel, vector<mzScan*>& results) {
    //determine offset
    pair<double, double> offsets = _mzdb->getBBSize(msLevel);

    double& heightOffset = offsets.first;
    double& widthOffset = offsets.second;

    double _minmz = minmz - heightOffset;
    double _minrt = minrt - widthOffset;
    double _maxmz = maxmz + heightOffset;
    double _maxrt = maxrt + widthOffset;
    map<int, vector<mzBB> > BBs;
    //cout << endl << _minmz << "," << _maxmz << ","<< _minrt << "," << _maxrt << endl;

    sqlite3_prepare_v2(_mzdb->db, mzRegionExtractor::sqlRTree, -1, &(_mzdb->stmt), 0);
    sqlite3_bind_double(_mzdb->stmt, 1, _minmz);
    sqlite3_bind_double(_mzdb->stmt, 2, _maxmz);
    sqlite3_bind_double(_mzdb->stmt, 3, _minrt);
    sqlite3_bind_double(_mzdb->stmt, 4, _maxrt);


    while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
        int firstScanId = sqlite3_column_int(_mzdb->stmt, 3);
        if (msLevelByID[firstScanId] != msLevel) {
            continue;
        }
        int id = sqlite3_column_int(_mzdb->stmt, 0);
        int runSliceId = sqlite3_column_int(_mzdb->stmt, 2);
        byte* dataptr = (byte*) sqlite3_column_blob(_mzdb->stmt, 1);
        size_t sizeptr = sqlite3_column_bytes(_mzdb->stmt, 1);
        vector<byte> data(dataptr, dataptr + sizeptr / sizeof (byte));
        mzBB bb(id, data, sizeptr);
        bb.firstScanId = firstScanId;
        bb.runSliceId = runSliceId;
        BBs[firstScanId].push_back(bb);
    }
    sqlite3_finalize(_mzdb->stmt);
    _mzdb->stmt = 0;

#ifdef DEBUG
    printf("bbs size:%d\n", BBs.size());
#endif

    for (auto it = BBs.begin(); it != BBs.end(); ++it) {

        vector<mzBB>& bbs = it->second;

        if ( bbs.empty() )
            continue;

        mzBB& b = bbs[0];

        map<int, vector<int> > scansInfos;
        int scanCount = BlobReaderInterface::buildMapPositions(&(b.data[0]), b.size, scansInfos, dataEncodings);
        //vector<mzScan*> scans;

        for (int i = 1; i <= scanCount; ++i) {
            int& id = scansInfos[i][0];
            float& rt = rtByID[id];
            mzScan* s = new mzScan(id, msLevel, rt);
            BlobReaderInterface::readData(&(b.data[0]), b.size, i, s, scansInfos);
            results.push_back(s);
        }

        for (size_t j = 1; j < bbs.size(); ++j) {
            map<int, vector<int> > scansInfos_;
            mzBB& bb = bbs[j];

            int scanCount_ = BlobReaderInterface::buildMapPositions(&(bb.data[0]), bb.size, scansInfos_, dataEncodings);

            for (int i = 1; i <= scanCount_; ++i) {
                BlobReaderInterface::readData(&(bb.data[0]), bb.size, i, results[i-1], scansInfos_);
            }
        }
    }

    //rt filter
    results.erase(remove_if(results.begin(), results.end(), [&minrt, &maxrt](mzScan* s) -> bool {
        bool b = (s->rt < minrt || s->rt > maxrt);
        if (b)
           delete s;
        return b;
    }), results.end());

    //mz filter
    for_each(results.begin(), results.end(), [&minmz, &maxmz](mzScan* s) {
        s->reduceTo(minmz, maxmz);
    });
}



PWIZ_API_DECL void mzRegionExtractor::runSliceExtraction(double minmz, double maxmz, int msLevel, vector<mzScan*>& results) {
    pair<double, double> offsets = _mzdb->getBBSize(msLevel);

    double& heightOffset = offsets.first;

    double _minmz = minmz - heightOffset;
    double _maxmz = maxmz + heightOffset;

    map<int, vector<mzBB> > BBs;

    sqlite3_prepare_v2(_mzdb->db, mzRegionExtractor::sqlRunSlice, -1, &(_mzdb->stmt), 0);
    sqlite3_bind_double(_mzdb->stmt, 1, _minmz);
    sqlite3_bind_double(_mzdb->stmt, 2, _maxmz);

    while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
        int firstScanId = sqlite3_column_int(_mzdb->stmt, 3);
        if (msLevelByID[firstScanId] != msLevel) {
            continue;
        }
        int id = sqlite3_column_int(_mzdb->stmt, 0);
        int runSliceId = sqlite3_column_int(_mzdb->stmt, 2);
        byte* dataptr = (byte*) sqlite3_column_blob(_mzdb->stmt, 1);
        size_t sizeptr = sqlite3_column_bytes(_mzdb->stmt, 1);
        vector<byte> data(dataptr, dataptr + sizeptr / sizeof (byte));
        mzBB bb(id, data, sizeptr);
        bb.firstScanId = firstScanId;
        bb.runSliceId = runSliceId;
        BBs[firstScanId].push_back(bb);
    }
    sqlite3_finalize(_mzdb->stmt);
    _mzdb->stmt = 0;


    for (auto it = BBs.begin(); it != BBs.end(); ++it) {

        vector<mzBB>& bbs = it->second;

        if ( bbs.empty() )
            continue;

        mzBB& b = bbs[0];

        map<int, vector<int> > scansInfos;
        int scanCount = BlobReaderInterface::buildMapPositions(&b.data[0], b.size, scansInfos, dataEncodings);

        for (int i = 1; i <= scanCount; ++i) {
            int& id = scansInfos[i][0];
            float& rt = rtByID[id];
            mzScan* s = new mzScan(id, msLevel, rt);
            BlobReaderInterface::readData(&b.data[0], b.size, i, s, scansInfos);
            results.push_back(s);
        }

        for (size_t j = 1; j < bbs.size(); ++j) {
            map<int, vector<int> > scansInfos_;
            mzBB& bb = bbs[j];

            int scanCount_ = BlobReaderInterface::buildMapPositions(&bb.data[0], bb.size, scansInfos_, dataEncodings);

            for (int i = 1; i <= scanCount_; ++i) {
                BlobReaderInterface::readData(&bb.data[0], bb.size, i, results[i-1], scansInfos_);
            }
        }
    }


    //mz filter
    for_each(results.begin(), results.end(), [&minmz, &maxmz](mzScan* s) {
        s->reduceTo(minmz, maxmz);
    });
}
}
