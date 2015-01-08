#ifndef MZBBINSERTER_H
#define MZBBINSERTER_H

#include "../lib/sqlite3/include/sqlite3.h"

#include "bb_computer.hpp"
#include "mzDBFile.h"

namespace mzdb {

class mzBBInserter {

private:

    MzDBFile& m_mzdbFile;
    int m_bbCount, m_lastMinRunSliceIdx, m_lastMaxRunSliceIdx, m_runSliceCount;
    map<int, int> m_runSliceIndexByMsLevel;

    /// Insert data into sqlite file
    template<class h_mz_t, class h_int_t,
                 class l_mz_t, class l_int_t>
    void _insertData(int msLevel,
                            double& bbheight,
                            map<int, int>& runSlices,
                            map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >& highResPeaks,
                            map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >& lowResPeaks,
                            map<int, DataMode>& dataModes,
                            double parentMinMz=0,
                            double parentMaxMz = 0) {

        //useful typedef
        typedef std::shared_ptr<Centroid<h_mz_t, h_int_t> > HighResCentroidSPtr;
        typedef std::shared_ptr<Centroid<l_mz_t, l_int_t> > LowResCentroidSPtr;
        typedef unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > mzBoundingBoxUPtr;

        vector<mzBoundingBoxUPtr> bbs;

        map<int, unique_ptr<map<int, vector<HighResCentroidSPtr> > > > r1;
        map<int, unique_ptr<map<int, vector<LowResCentroidSPtr> > > > r2;

        BBComputer::computeBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(bbheight, highResPeaks, lowResPeaks, bbs, r1, r2);


        //special cases leading to failure IMPORTANT SEGFAULT
        bool emptyMsLevel = false;
        if (bbs.empty()) {
            //Too much log in ABI sciex file
            //LOG(INFO) << "Found a bounding box empty";
            emptyMsLevel = true;
            auto bb = mzBoundingBoxUPtr(
                        new mzBoundingBox<h_mz_t, h_int_t,l_mz_t, l_int_t>(
                            unique_ptr<map<int, vector<HighResCentroidSPtr> > >(new map<int, vector<HighResCentroidSPtr> >),
                            unique_ptr<map<int, vector<LowResCentroidSPtr> > >(new map<int, vector<LowResCentroidSPtr> >)));

            bb->update(highResPeaks, lowResPeaks);
            bbs.push_back(std::move(bb));
        }


        int refScanIdx, refLastScanId;
        if ( ! highResPeaks.empty() && ! lowResPeaks.empty()) {
            refScanIdx = min(highResPeaks.begin()->first, lowResPeaks.begin()->first);
            refLastScanId = max(highResPeaks.rbegin()->first, lowResPeaks.rbegin()->first);
        } else if ( ! highResPeaks.empty() && lowResPeaks.empty()) {
            refScanIdx = highResPeaks.begin()->first;
            refLastScanId = highResPeaks.rbegin()->first;
        } else{
            refScanIdx = lowResPeaks.begin()->first;
            refLastScanId = lowResPeaks.rbegin()->first;
        }

        //bbs are sorted by mz and so on by runSliceIdx
        auto firstRunSliceIdx = bbs.front()->runSliceIdx();
        auto lastRunSliceIdx = bbs.back()->runSliceIdx();

        if (m_lastMinRunSliceIdx && firstRunSliceIdx < m_lastMinRunSliceIdx) {
            printf("The first spectrum does not contain a low mass contained in the first runSlice.\n");
            printf("This is a bug and it will be fixed in a next release of raw2mzdb.\n");
            exit(0);
        }

        const double invBBheight = 1.0 / bbheight;

        if (m_lastMaxRunSliceIdx && m_lastMaxRunSliceIdx < firstRunSliceIdx) {
            const char* sql = "INSERT INTO run_slice VALUES (?, ?, ?, ?, ?, ?, ?);";
            sqlite3_prepare_v2(m_mzdbFile.db, sql, -1, &(m_mzdbFile.stmt), 0);
            for (int i = m_lastMaxRunSliceIdx + 1; i <= firstRunSliceIdx; ++i) {
                if (runSlices.find(i) == runSlices.end()) {//not found
                    runSlices[i] = m_runSliceCount;
                    sqlite3_bind_int(m_mzdbFile.stmt, 1, m_runSliceCount);
                    sqlite3_bind_int(m_mzdbFile.stmt, 2, msLevel);
                    sqlite3_bind_int(m_mzdbFile.stmt, 3, m_runSliceCount);
                    double moz = i * invBBheight;
                    sqlite3_bind_double(m_mzdbFile.stmt, 4, moz);
                    sqlite3_bind_double(m_mzdbFile.stmt, 5, moz + invBBheight);
                    sqlite3_bind_null(m_mzdbFile.stmt, 6);
                    sqlite3_bind_int(m_mzdbFile.stmt, 7, 1);
                    sqlite3_step(m_mzdbFile.stmt);
                    sqlite3_reset(m_mzdbFile.stmt);
                    m_runSliceCount++;
                }
            }
            sqlite3_finalize(m_mzdbFile.stmt);
            m_mzdbFile.stmt = 0; //optional
        }

        const char* sql = "INSERT INTO run_slice VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(m_mzdbFile.db, sql, -1, &(m_mzdbFile.stmt), 0);

        for (int i = firstRunSliceIdx; i <= lastRunSliceIdx; ++i) {
            if (runSlices.find(i) == runSlices.end()) {
                //not found
                runSlices[i] = m_runSliceCount;
                sqlite3_bind_int(m_mzdbFile.stmt, 1, m_runSliceCount);
                sqlite3_bind_int(m_mzdbFile.stmt, 2, msLevel);
                sqlite3_bind_int(m_mzdbFile.stmt, 3, m_runSliceCount);
                double moz = i * invBBheight;
                sqlite3_bind_double(m_mzdbFile.stmt, 4, moz);
                sqlite3_bind_double(m_mzdbFile.stmt, 5, moz + invBBheight);
                sqlite3_bind_null(m_mzdbFile.stmt, 6);
                sqlite3_bind_int(m_mzdbFile.stmt, 7, 1);
                sqlite3_step(m_mzdbFile.stmt);
                sqlite3_reset(m_mzdbFile.stmt);
                m_runSliceCount++;
            }
        }
        sqlite3_finalize(m_mzdbFile.stmt);
        m_mzdbFile.stmt = 0;

        // bounding boxes
        const char* sql_2 = "INSERT INTO bounding_box VALUES ( ?, ?, ?, ?, ?);";
        const char* sql_3 = "INSERT INTO bounding_box_rtree VALUES (?, ?, ?, ?, ?);";
        const char* sql_4 = "INSERT INTO bounding_box_msn_rtree VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);";

        //create a new statement in order to iterate over bounding boxes only once
        sqlite3_stmt* stmt = 0;
        sqlite3_stmt* stmtHigherLevel = 0;

        sqlite3_prepare_v2(m_mzdbFile.db, sql_2, -1, &(m_mzdbFile.stmt), 0);
        sqlite3_prepare_v2(m_mzdbFile.db, sql_3, -1, &stmt, 0);
        sqlite3_prepare_v2(m_mzdbFile.db, sql_4, -1, &stmtHigherLevel, 0);

        for (auto it = bbs.begin(); it != bbs.end(); ++it) {
            mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>& bb = **it;
            vector<byte> data;// output;
            bb.asByteArray(data, dataModes);

            sqlite3_bind_int(m_mzdbFile.stmt, 1, m_bbCount);
            sqlite3_bind_blob(m_mzdbFile.stmt, 2, &data[0], data.size(), SQLITE_STATIC);
            sqlite3_bind_int(m_mzdbFile.stmt, 3, runSlices[bb.runSliceIdx()]);
            sqlite3_bind_int(m_mzdbFile.stmt, 4, refScanIdx);
            sqlite3_bind_int(m_mzdbFile.stmt, 5, refLastScanId);
            sqlite3_step(m_mzdbFile.stmt);
            sqlite3_reset(m_mzdbFile.stmt);

            if (msLevel == 1) {
                sqlite3_bind_int(stmt, 1, m_bbCount);
                sqlite3_bind_double(stmt, 2, bb.mzmin());
                sqlite3_bind_double(stmt, 3, bb.mzmax());
                sqlite3_bind_double(stmt, 4, bb.rtmin());
                sqlite3_bind_double(stmt, 5, bb.rtmax());
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            } else {
                sqlite3_bind_int(stmtHigherLevel, 1, m_bbCount);
                sqlite3_bind_int(stmtHigherLevel, 2, msLevel);
                sqlite3_bind_int(stmtHigherLevel, 3, msLevel);
                sqlite3_bind_int(stmtHigherLevel, 4, parentMinMz); // parent_min_mz
                sqlite3_bind_int(stmtHigherLevel, 5, parentMaxMz); // parent_max_mz
                sqlite3_bind_double(stmtHigherLevel, 6, bb.mzmin());
                sqlite3_bind_double(stmtHigherLevel, 7, bb.mzmax());
                sqlite3_bind_double(stmtHigherLevel, 8, bb.rtmin());
                sqlite3_bind_double(stmtHigherLevel, 9, bb.rtmax());
                sqlite3_step(stmtHigherLevel);
                sqlite3_reset(stmtHigherLevel);
            }
            ++m_bbCount;

        }
        sqlite3_finalize(m_mzdbFile.stmt);
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmtHigherLevel);
        m_mzdbFile.stmt = stmt = stmtHigherLevel = 0;
    }


public:
    inline mzBBInserter(MzDBFile& mzdbFile):
        m_mzdbFile(mzdbFile), m_bbCount(1), m_lastMinRunSliceIdx(0), m_lastMaxRunSliceIdx(0), m_runSliceCount(1){}


    template<class h_mz_t, class h_int_t,
                 class l_mz_t, class l_int_t>
    void buildAndInsertData(int msLevel,
                                    double& bbheight,
                                    vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > >& highResBuffer,
                                    vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > >& lowResBuffer,
                                    map<int, int>& runSlices,
                                    double parentMinMz=0,
                                    double parentMaxMz=0) {

        map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >p1;
        map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >p2;
        map<int, DataMode> dataModes;
        BBComputer::buildCentroidsByScanID<h_mz_t, h_int_t>(p1, highResBuffer, dataModes);
        BBComputer::buildCentroidsByScanID<l_mz_t, l_int_t>(p2, lowResBuffer, dataModes);
        this->_insertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel, bbheight, runSlices, p1, p2, dataModes, parentMinMz, parentMaxMz);
    }
};


__END_MZDB_NM //end namespace

#endif // MZBBINSERTER_H
