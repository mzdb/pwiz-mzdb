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

//author marc.dubois@ipbs.fr

#ifndef __MZDBWRITER__
#define __MZDBWRITER__

//--- Base import
#include <time.h>
#include <cmath>
#include <string>
#include <iostream>
#include <unordered_map>
#include <array>
#include <memory>
#include <functional>
//--- Pwiz import
#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz_tools/common/FullReaderList.hpp"
#include "pwiz/data/msdata/IO.hpp"
#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h"
#include "pwiz/analysis/spectrum_processing/SpectrumList_PrecursorRecalculator.hpp"
#include "pwiz/data/vendor_readers/Thermo/SpectrumList_Thermo.hpp"
#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"
#include "boost/thread/thread.hpp"
//--- SQLite import
#include "../lib/sqlite3/include/sqlite3.h"
#include "../lib/pugixml/include/pugixml.hpp"
//--- MzDB import
#include "include_msdata.h"
#include "../utils/mzDBFile.h"
#include "../utils/mzISerializer.h"
#include "msdata/mzCycleObject.hpp"
#include "../threading/Queue.hpp"
#include "../threading/SpScQueue.hpp"
#include "../msdata/mzUserText.h"
#include "mzAbstractMetadataExtractor.hpp"
//--- Compression import
#ifdef USE_COMPRESSION
#include "../utils/lz4.h"
#include "../utils/md5.hpp"
#endif

namespace mzdb {

using namespace std;

//versioning of the application: we distinguish schema database
//version against application version, both can have different version numbers
#define SCHEMA_VERSION_STR "0.6.0"
#define SCHEMA_VERSION_INT 0.6.0
#define SOFT_VERSION_STR "0.9.7"
#define SOFT_VERSION_INT 0.9.7

#define STEP_PERCENT 2
#define HUNDRED 100
#define MAX_MS 2

#define bb_ms1_mz_width_default 5
#define bb_ms1_rt_width_default 15
#define bb_msn_mz_width_default 10000
#define bb_msn_rt_width_default 0

#define IN_HIGH_RES_STR "in_high_res"


/**
 * Base class for writing a raw to mzDBFile
 */
class mzDBWriter: private boost::noncopyable {

private:

    MzDBFile& _mzdb;
    pwiz::msdata::MSDataPtr _msdata;
    pwiz::msdata::CVID _originFileFormat;
    std::unique_ptr<mzIMetadataExtractor> _metadataExtractor;

    map<int, DataMode> _msnMode;
    map<DataMode, int> _dataModePos;

    int _scanCount, _cycleCount, _bbCount, _lastMinRunSliceIdx,
    _lastMaxRunSliceIdx, _runSliceCount, _progressionCounter;

    ISerializer::xml_string_writer _serializer;

    unordered_map<string, pwiz::msdata::CVParam> _cvids; //assume they are unique
    unordered_map<string, pwiz::msdata::UserParam> _userParamsByName; // the same

    //---------------------------- SCANS INSERTIONS ----------------------------
    /**
     * Insert Scan into DB
     *
     * @param spectrum: current Spectrum
     * @param msLevel: msLevel
     * @param bbFirstScanId: idx of the first scan of the BB
     */
    template<typename U, typename V>
    void insertScan(std::shared_ptr<mzSpectrum<U, V> >& spectrum, int bbFirstScanId, pwiz::msdata::SpectrumPtr& parentSpectrum) {
        if (! spectrum) {
            LOG(ERROR) << "ERROR...spectrum pointer is null\n";
            return;
        }
        const pwiz::msdata::SpectrumPtr& spec = spectrum->spectrum;
        //update spectra
        updateCVMap(*spec);
        updateUserMap(*spec);
        //update for cvparams of scans
        updateCVMap(spec->scanList.scans[0]);
        updateUserMap(spec->scanList.scans[0]);

        const int& msLevel =spectrum->msLevel();
        const char* sql = "INSERT INTO spectrum  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(_mzdb.db, sql, -1, &(_mzdb.stmt), 0);

        sqlite3_bind_int(_mzdb.stmt, 1, spectrum->id);

        //initial_id
        sqlite3_bind_int(_mzdb.stmt, 2, spectrum->id);
        string& id = spec->id;
        //title
        sqlite3_bind_text(_mzdb.stmt, 3, id.c_str(), id.length(), SQLITE_TRANSIENT);
        //cycle
        sqlite3_bind_int(_mzdb.stmt, 4, spectrum->cycle);
        //time
        sqlite3_bind_double(_mzdb.stmt, 5, rtOf(spec));
        //msLevel
        sqlite3_bind_int(_mzdb.stmt, 6, msLevel);

        //activation_type
        if (msLevel > 1) {
            const string activationCode = mzdb::getActivationCode(spec->precursors.front().activation);
            sqlite3_bind_text(_mzdb.stmt, 7, activationCode.c_str(), activationCode.length(), SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_text(_mzdb.stmt, 7, "", 0, SQLITE_STATIC);
        }

        //tic
        sqlite3_bind_double(_mzdb.stmt, 8, spec->cvParam(MS_total_ion_current).valueAs<float>());
        //base peak mz
        sqlite3_bind_double(_mzdb.stmt, 9, spec->cvParam(MS_base_peak_m_z).valueAs<double>());
        //base peak intensity
        sqlite3_bind_double(_mzdb.stmt, 10, spec->cvParam(MS_base_peak_intensity).valueAs<float>());

        //precursor mz precursor charge
        if (msLevel > 1) {
            //spectrum->refinedPrecursorMz(parentSpectrum);// just add some metadata, PIP and nearest prec mz
            sqlite3_bind_double(_mzdb.stmt, 11, spectrum->precursorMz());
            sqlite3_bind_double(_mzdb.stmt, 12, precursorChargeOf(spec));
        } else {
            sqlite3_bind_null(_mzdb.stmt, 11);
            sqlite3_bind_null(_mzdb.stmt, 12);
        }

        //datapointscount
        sqlite3_bind_int(_mzdb.stmt, 13, spectrum->nbPeaks());
        //Idea put this into methods of spectrum ?
        //put a new user param to evaluate the resolution of a spectrum
        bool isInHighRes = this->_metadataExtractor->isInHighRes(spec);//(scanparam.find("FTMS") != string::npos) ? true : false;
        pwiz::msdata::UserParam p(IN_HIGH_RES_STR); p.type = "boolean";
        if (isInHighRes) {
            p.value = true_str;
        } else {
            p.value = false_str;
        }
        spec->userParams.push_back(p);
        string& r = ISerializer::serialize(*spec, _serializer);
        sqlite3_bind_text(_mzdb.stmt, 14, r.c_str(), r.length(), SQLITE_TRANSIENT);
        // ---Here we use directly the pwiz api for writing xml chunks
        //scan list
        ostringstream os_2;
        pwiz::minimxml::XMLWriter writer_2(os_2);
        pwiz::msdata::IO::write(writer_2, spec->scanList, *_msdata);
        string r_2 = os_2.str();
        sqlite3_bind_text(_mzdb.stmt, 15, r_2.c_str(), r_2.length(), SQLITE_TRANSIENT);

        //precursor list
        if (msLevel > 1) {
            ostringstream os_3;
            pwiz::minimxml::XMLWriter writer_3(os_3);
            for (auto p = spec->precursors.begin(); p != spec->precursors.end(); ++p) {
                pwiz::msdata::IO::write(writer_3, *p);
            }
            string r_3 =  os_3.str();
            sqlite3_bind_text(_mzdb.stmt, 16, r_3.c_str(), r_3.length(), SQLITE_TRANSIENT);

            //product list
            ostringstream os_4;
            pwiz::minimxml::XMLWriter writer_4(os_4);
            for (auto p = spec->products.begin(); p != spec->products.end(); ++p) {
                pwiz::msdata::IO::write(writer_4, *p);
            }
            string r_4 = os_4.str();
            sqlite3_bind_text(_mzdb.stmt, 17, r_4.c_str(), r_4.length(), SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(_mzdb.stmt, 16);
            sqlite3_bind_null(_mzdb.stmt, 17);
        }

        //shared param tree id
        sqlite3_bind_null(_mzdb.stmt, 18);
        //instrument config id
        sqlite3_bind_int(_mzdb.stmt, 19, instrumentConfigurationIndex(spec->scanList.scans[0].instrumentConfigurationPtr));
        //source file id
        sqlite3_bind_int(_mzdb.stmt, 20, sourceFileIndex(spec->sourceFilePtr));
        //run id
        sqlite3_bind_int(_mzdb.stmt, 21, 1); //run id default to 1
        //data proc id
        sqlite3_bind_int(_mzdb.stmt, 22, dataProcessingIndex(spec->dataProcessingPtr));
        //data enc id
        //TODO: fix this we like before
        sqlite3_bind_int(_mzdb.stmt, 23, _dataModePos[spectrum->getEffectiveMode(_msnMode[msLevel])]);
        //bb first spec id
        sqlite3_bind_int(_mzdb.stmt, 24, bbFirstScanId);
        sqlite3_step(_mzdb.stmt);
        sqlite3_finalize(_mzdb.stmt);
        _mzdb.stmt = 0;
    }

    /**
     * insert several scans
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    inline void insertScans(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject) {
        auto& spectra = cycleObject->spectra;
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {
            if (it->first != nullptr)
                this->insertScan<h_mz_t, h_int_t>(it->first, cycleObject->getBeginId(), cycleObject->parentSpectrum);
            else
                this->insertScan<l_mz_t, l_int_t>(it->second, cycleObject->getBeginId(), cycleObject->parentSpectrum);

        }
    }

    //---------------------------- BOUNDING BOX INSERTIONS ---------------------
    /**
     * build MapHolder elements
     * @param pt
     * @param vt
     * @param dataModes
     */
    template<typename U, typename V>
    inline static void buildCentroidsByScanID(map<int, vector<std::shared_ptr<Centroid<U, V> > > >& centroidsByScanId,
                                              const vector<std::shared_ptr<mzSpectrum<U, V> > >& spectra,
                                              map<int, DataMode>& dataModes) {

        for_each(spectra.begin(), spectra.end(), [&centroidsByScanId, &dataModes]( std::shared_ptr<mzSpectrum<U, V> > spectrum) {
            const int& id = spectrum->id;
            centroidsByScanId[id] = std::move(spectrum->peaks); //move semantics to avoid copy, leads to bug ?
            dataModes[id] = spectrum->effectiveMode;
        });
    }


    /**
     * Caculation of the runSlice index for each peak it belongs to
     * @param v
     * @param x
     * @param scanIdx
     * @param bbheight
     */
    template<class mz_t, class int_t>
    inline static void groupByMzIndex(vector<std::shared_ptr<Centroid<mz_t, int_t> > >& v, //correspond to the entire centroid of one spectrum,
                                      map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > > > >& x, //run Slice
                                      int scanIdx, double& bbheight) {

        typedef std::shared_ptr<Centroid<mz_t, int_t> > CentroidSPtr;

        for_each(v.begin(), v.end(), [&x, &scanIdx, &bbheight](CentroidSPtr& peak) {
            const int idx = peak->mz * bbheight;
            if (x.find(idx) == x.end()) {//not found
                auto m = unique_ptr<map<int, vector<CentroidSPtr> > >(new map<int, vector<CentroidSPtr> >);
                x[idx] = move(m);
            }
            (*(x[idx]))[scanIdx].push_back(peak);
        });

    }


    /**
     * compute BBs given a MapHolder
     * @param bbheight
     * @param bbs: bbs output
     * @param hrs: map<int, map<int, vector<mzPeak> > > runSliceidx / scanidx, peaks(hr) contained in this scan
     * @see MapHolder
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    static void computeBoundingBox(double& bbheight,
                                   map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >& highResPeaksByScanIDs,
                                   map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >& lowResPeaksByScanIDs,
                                   vector<unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& bbs, // output
                                   map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > > > >& hrs,
                                   map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > > > >& lrs ) {

        typedef std::shared_ptr<Centroid<h_mz_t, h_int_t> > HighResCentroidSPtr;
        typedef std::shared_ptr<Centroid<l_mz_t, l_int_t> > LowResCentroidSPtr;
        typedef unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > mzBoundingBoxUPtr;
        //group all peaks by their runSliceId

        vector<pair<int, int> > v;
        mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>::iterationOrder(highResPeaksByScanIDs, lowResPeaksByScanIDs, v);
        for (size_t i = 0; i < v.size(); ++i) {
            pair<int, int>& p = v[i];
            int scanIdx = p.second;
            if (p.first == 1) {
                mzDBWriter::groupByMzIndex<h_mz_t, h_int_t>( highResPeaksByScanIDs[scanIdx], hrs, scanIdx, bbheight );
            } else {
                mzDBWriter::groupByMzIndex<l_mz_t, l_int_t>( lowResPeaksByScanIDs[scanIdx], lrs, scanIdx, bbheight );
            }
        }

        vector<int> treatedRunSliceIdx;

        //retrieve all bounding box belonging to the same runSlice idx
        for (auto it = hrs.begin(); it != hrs.end(); ++it) {
            const int& runSliceIdx = it->first;
            auto lp = unique_ptr<map<int, vector<LowResCentroidSPtr> > >(new map<int, vector<LowResCentroidSPtr> >);
            if (lrs.find(runSliceIdx) != lrs.end()) {
                for (auto iit = lrs[runSliceIdx]->begin(); iit != lrs[runSliceIdx]->end(); ++iit) {
                    (*lp)[iit->first] = std::move(iit->second);
                }
                treatedRunSliceIdx.push_back(runSliceIdx);
            }
            auto bb = mzBoundingBoxUPtr(
                        new mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(runSliceIdx, 1.0/bbheight, it->second, lp));
            bb->update(highResPeaksByScanIDs, lowResPeaksByScanIDs);
            if (! bb->isEmpty()) {
                //bb->computeMzBounds();
                bbs.push_back(move(bb));
            }
        }

        for (auto it = lrs.begin(); it != lrs.end(); ++it) {
            const int& runSliceIdx = it->first;
            if (find(treatedRunSliceIdx.begin(), treatedRunSliceIdx.end(), runSliceIdx) == treatedRunSliceIdx.end()) {
                auto hp = unique_ptr<map<int, vector<HighResCentroidSPtr> > >(new map<int, vector<HighResCentroidSPtr> >);
                auto bb = mzBoundingBoxUPtr(
                            new mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(runSliceIdx, 1.0/bbheight, hp, it->second));
                bb->update(highResPeaksByScanIDs, lowResPeaksByScanIDs);
                if (! bb->isEmpty()) {
                    //bb->computeMzBounds();
                    bbs.push_back(move(bb));
                }
            }
        }
        //seems to be unuseful
        //sort selon mz begin implementation of operator <
        //sort(bbs.begin(), bbs.end(), mzBoundingBoxPtrComp<h_mz_t, h_int_t>());
    }


    /**
     * Compute BoundingBox then insert BBs found  and RunSlices in the database
     *
     * @param msLevel: current msLevel
     * @param bbheight: height of BBs, could be different among msLevel (mz: ms1 5da)
     * @param nbPeaks: count the number of peaks (just for debugging)
     * @param runSlices: idxRunSlices / RunSlice Count
     * @param dataModes: scanIdx / DataMode
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    void insertData(int msLevel,
                    double& bbheight,
                    map<int, int>& runSlices,
                    map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >& highResPeaks,
                    map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >& lowResPeaks,
                    map<int, DataMode>& dataModes) {

        typedef std::shared_ptr<Centroid<h_mz_t, h_int_t> > HighResCentroidSPtr;
        typedef std::shared_ptr<Centroid<l_mz_t, l_int_t> > LowResCentroidSPtr;
        typedef unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > mzBoundingBoxUPtr;

        vector<mzBoundingBoxUPtr> bbs;

        map<int, unique_ptr<map<int, vector<HighResCentroidSPtr> > > > r1;
        map<int, unique_ptr<map<int, vector<LowResCentroidSPtr> > > > r2;

        mzDBWriter::computeBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(bbheight, highResPeaks, lowResPeaks, bbs, r1, r2);

        //special cases leading to failure IMPORTANT SEGFAULT
        bool emptyMsLevel = false;
        if (bbs.empty()) {
            LOG(INFO) << "Found a bounding box empty";
            emptyMsLevel = true;
            auto bb = mzBoundingBoxUPtr(
                        new mzBoundingBox<h_mz_t, h_int_t,l_mz_t, l_int_t>(
                            unique_ptr<map<int, vector<HighResCentroidSPtr> > >(new map<int, vector<HighResCentroidSPtr> >),
                            unique_ptr<map<int, vector<LowResCentroidSPtr> > >(new map<int, vector<LowResCentroidSPtr> >)));

            bb->update(highResPeaks, lowResPeaks);
            bbs.push_back(move(bb));
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

        if (_lastMinRunSliceIdx && firstRunSliceIdx < _lastMinRunSliceIdx) {
            printf("The first spectrum does not contain a low mass contained in the first runSlice.\n");
            printf("This is a bug and it will be fixed in a next release of raw2mzdb.\n");
            exit(0);
        }
        if (_lastMaxRunSliceIdx && _lastMaxRunSliceIdx < firstRunSliceIdx) {
            const char* sql = "INSERT INTO run_slice VALUES (?, ?, ?, ?, ?, ?, ?);";
            sqlite3_prepare_v2(_mzdb.db, sql, -1, &(_mzdb.stmt), 0);
            for (int i = _lastMaxRunSliceIdx + 1; i <= firstRunSliceIdx; ++i) {
                if (runSlices.find(i) == runSlices.end()) {//not found
                    runSlices[i] = _runSliceCount;
                    sqlite3_bind_int(_mzdb.stmt, 1, _runSliceCount);
                    sqlite3_bind_int(_mzdb.stmt, 2, msLevel);
                    sqlite3_bind_int(_mzdb.stmt, 3, _runSliceCount);
                    double moz = i * (1.0/bbheight);
                    sqlite3_bind_double(_mzdb.stmt, 4, moz);
                    sqlite3_bind_double(_mzdb.stmt, 5, moz + (1.0/bbheight));
                    sqlite3_bind_null(_mzdb.stmt, 6);
                    sqlite3_bind_int(_mzdb.stmt, 7, 1);
                    sqlite3_step(_mzdb.stmt);
                    sqlite3_reset(_mzdb.stmt);
                    _runSliceCount++;
                }
            }
            sqlite3_finalize(_mzdb.stmt);
            _mzdb.stmt = 0;
        }

        const char* sql = "INSERT INTO run_slice VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(_mzdb.db, sql, -1, &(_mzdb.stmt), 0);
        for (int i = firstRunSliceIdx; i <= lastRunSliceIdx; ++i) {
            if (runSlices.find(i) == runSlices.end()) {//not found
                runSlices[i] = _runSliceCount;
                sqlite3_bind_int(_mzdb.stmt, 1, _runSliceCount);
                sqlite3_bind_int(_mzdb.stmt, 2, msLevel);
                sqlite3_bind_int(_mzdb.stmt, 3, _runSliceCount);
                double moz = i * (1.0/bbheight);
                sqlite3_bind_double(_mzdb.stmt, 4, moz);
                sqlite3_bind_double(_mzdb.stmt, 5, moz + (1.0/bbheight));
                sqlite3_bind_null(_mzdb.stmt, 6);
                sqlite3_bind_int(_mzdb.stmt, 7, 1);
                sqlite3_step(_mzdb.stmt);
                sqlite3_reset(_mzdb.stmt);
                _runSliceCount++;
            }
        }
        sqlite3_finalize(_mzdb.stmt);
        _mzdb.stmt = 0;

        const char* sql_2 = "INSERT INTO bounding_box VALUES ( ?, ?, ?, ?, ?);";
        const char* sql_3 = "INSERT INTO bounding_box_rtree VALUES (?, ?, ?, ?, ?);";
        const char* sql_4 = "INSERT INTO bounding_box_msn_rtree VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);";

        //create a new statement, we iterate over bounding box only once
        sqlite3_stmt* stmt = 0;
        sqlite3_stmt* stmtHigherLevel = 0;
        //prepare_v2 is still the fastest
        sqlite3_prepare_v2(_mzdb.db, sql_2, -1, &(_mzdb.stmt), 0);
        sqlite3_prepare_v2(_mzdb.db, sql_3, -1, &stmt, 0);
        sqlite3_prepare_v2(_mzdb.db, sql_4, -1, &stmtHigherLevel, 0);

        for (auto it = bbs.begin(); it != bbs.end(); ++it) {
            mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>& bb = **it;
            //int N = bb.getBytesVectorLength(dataModes);
            vector<byte> data;// output;
            //data.reserve(N);
            bb.asByteArray(data, dataModes);

            //compression
            /*int worstSize = LZ4_compressBound( data.size() );
            char* comp = (char *) malloc(worstSize);
            int r = LZ4_compress((const char*)&data[0], comp, data.size() );

            if (! r)
                printf("Compression failed !!!\n");

            //output.resize(r);

            //vector<byte> uncomp;
            char* uncomp = (char*) malloc(data.size());
            //uncomp.reserve( data.size() );
            int s = LZ4_decompress_fast( comp, uncomp, data.size() );
            if (s <= 0 ) {
                printf("Decompression failed :%d\n", s);
            }
            char* smd5 = md5( (unsigned char*)&data[0], data.size());
            char* rmd5 = md5( (unsigned char*)uncomp, data.size() );

            if ( strcmp(rmd5, smd5) == 0) {
               // printf("Good md5\n");
            } else {
                printf("bad md5\n");
            }*/


            sqlite3_bind_int(_mzdb.stmt, 1, _bbCount);
            sqlite3_bind_blob(_mzdb.stmt, 2, &data[0], data.size(), SQLITE_STATIC);
            sqlite3_bind_int(_mzdb.stmt, 3, runSlices[bb.runSliceIdx()]);
            sqlite3_bind_int(_mzdb.stmt, 4, refScanIdx);
            sqlite3_bind_int(_mzdb.stmt, 5, refLastScanId);
            sqlite3_step(_mzdb.stmt);
            sqlite3_reset(_mzdb.stmt);

            if (msLevel == 1) {
                sqlite3_bind_int(stmt, 1, _bbCount);
                sqlite3_bind_double(stmt, 2, bb.mzmin());
                sqlite3_bind_double(stmt, 3, bb.mzmax());
                sqlite3_bind_double(stmt, 4, bb.rtmin());
                sqlite3_bind_double(stmt, 5, bb.rtmax());
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            } else {
                sqlite3_bind_int(stmtHigherLevel, 1, _bbCount);
                sqlite3_bind_int(stmtHigherLevel, 2, msLevel);
                sqlite3_bind_int(stmtHigherLevel, 3, msLevel);
                sqlite3_bind_int(stmtHigherLevel, 4, 0); // parent_min_mz
                sqlite3_bind_int(stmtHigherLevel, 5, 0); // parent_max_mz
                sqlite3_bind_double(stmtHigherLevel, 6, bb.mzmin());
                sqlite3_bind_double(stmtHigherLevel, 7, bb.mzmax());
                sqlite3_bind_double(stmtHigherLevel, 8, bb.rtmin());
                sqlite3_bind_double(stmtHigherLevel, 9, bb.rtmax());
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
            ++_bbCount;
            //free(comp); free(uncomp);free(smd5); free(rmd5);

        }
        sqlite3_finalize(_mzdb.stmt);
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmtHigherLevel);
        _mzdb.stmt = 0;
        stmt = 0;
    }

    /** @brief mzDBWriter::buildAndInsertData */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    void buildAndInsertData(int msLevel,
                            double& bbheight,
                            vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > >& highResBuffer,
                            vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > >& lowResBuffer,
                            map<int, int>& runSlices) {

        map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >p1;
        map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >p2;
        map<int, DataMode> dataModes;
        this->buildCentroidsByScanID<h_mz_t, h_int_t>(p1, highResBuffer, dataModes);
        this->buildCentroidsByScanID<l_mz_t, l_int_t>(p2, lowResBuffer, dataModes);

        this->insertData<h_mz_t, h_int_t>(msLevel, bbheight, runSlices, p1, p2, dataModes);
    }


    //------------------ PEAK PICKING FUNCTIONS --------------------------------
    /**
     * launch peakPicking using a thread group
     * @brief lauchPeakPicking
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    inline void launchPeakPicking(vector<std::shared_ptr<mzSpectrum<h_mz_t,h_int_t> > >& highResBuffer,
                                  vector<std::shared_ptr<mzSpectrum<l_mz_t,l_int_t> > >& lowResBuffer,
                                  DataMode m,
                                  pwiz::msdata::CVID filetype,
                                  mzPeakFinderUtils::PeakPickerParams& params) {
        boost::thread_group g;
        for (size_t j = 0; j < highResBuffer.size(); ++j) {
            //FIXME: due to a bug in VC10 can move with unique_ptr since it requires to be non copyable
            //use std::shared_ptr instead which is less efficient
            g.create_thread(std::bind(&mzSpectrum<h_mz_t, h_int_t>::doPeakPicking, highResBuffer[j], m, filetype, params));
        }
        for (size_t j = 0; j < lowResBuffer.size(); ++j) {
            g.create_thread(std::bind(&mzSpectrum<l_mz_t, l_int_t>::doPeakPicking, lowResBuffer[j], m, filetype, params));
        }
        g.join_all();
    }


    /**
     * wrapper of the function above
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    inline void launchPeakPicking(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject,
                                  pwiz::msdata::CVID filetype,
                                  mzPeakFinderUtils::PeakPickerParams& params) {
        vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
        vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
        cycleObject->getSpectra(highResSpectra, lowResSpectra);
        this->launchPeakPicking<h_mz_t, h_int_t, l_mz_t, l_int_t>(highResSpectra, lowResSpectra, _msnMode[cycleObject->msLevel], filetype, params);
    }


    //------------------ UTILITY FUNCTIONS -------------------------------------

    /** Create sqlite tables */
    void createTables();

    /** create indexes on large tables */
    void setIndexes();

    /** build empty needed DataProessing... not const because we modify _msdata */
    void checkMetaData() throw();

    /** insert metadata into the database */
    void insertMetaData(bool noLoss);

    /** update the recensement of the cvterm used in the document */
    void updateCVMap(const pwiz::msdata::ParamContainer& p);

    /** */
    void updateUserMap(const pwiz::msdata::ParamContainer& p);


    /** get the good data extractor */
    inline std::unique_ptr<mzIMetadataExtractor> getMetadataExtractor() {
        switch ( (int) this->_originFileFormat ) {
        case (int) pwiz::msdata::MS_Thermo_RAW_format: {
            return  std::unique_ptr<mzIMetadataExtractor>(new mzThermoMetadataExtractor(this->_mzdb.name));
        }
        default: {
            LOG(WARNING) << "Metadata extraction is not supported for this file format:" << pwiz::msdata::cvTermInfo(this->_originFileFormat).name;
            return std::unique_ptr<mzIMetadataExtractor>(new mzEmptyMetadataExtractor(this->_mzdb.name));
        }
        }
    }

    //------------------ GET INDEX OF PWIZ OBJECTS------------------------------
    /** */
    int instrumentConfigurationIndex(const pwiz::msdata::InstrumentConfigurationPtr& ic) const;

    /** */
    int dataProcessingIndex(const pwiz::msdata::DataProcessingPtr& dp) const;

    /** */
    int sourceFileIndex(const pwiz::msdata::SourceFilePtr& sf) const;

    /** */
    int paramGroupIndex(const pwiz::msdata::ParamGroupPtr& pg) const;

    /** */
    void insertCollectedCVTerms();


    //------------------OTHER UTILITY FUNCTIONS --------------------------------

    /** return the dataMode from a spectra */
    static DataMode getDataMode(const pwiz::msdata::SpectrumPtr, DataMode);



    //---------------PRODUCER CONSUMER IMPLEMENTATION---------------------------
    /**
     *
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t, class SpectrumListType>
    inline void mzCycleCollectionProducer(pwiz::util::IntegerSet& levelsToCentroid,
                                          folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& cyclesCollectionQueue,
                                          SpectrumListType* spectrumList, int nscans,
                                          map<int, double>& bbWidthManager) { // only for test purpose


        unordered_map<int, std::unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > spectraByMsLevel;
        pwiz::msdata::SpectrumPtr currMs1;



        for (size_t i = 0; i < nscans; ++i) {

            const pwiz::msdata::SpectrumPtr spectrum = spectrumList->spectrum(i, true, levelsToCentroid);
            const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
            const float rt = rtOf(spectrum);

            bool isInHighRes = this->_metadataExtractor->isInHighRes(spectrum);

            if (msLevel == 1 ) {
                ++_cycleCount;
                currMs1 = spectrum;
            }

            //init
            if (spectraByMsLevel.find(msLevel) == spectraByMsLevel.end()) {
                unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >spectraCollection(
                            new mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel));
                spectraCollection->parentSpectrum = currMs1;
                spectraByMsLevel[msLevel] = std::move(spectraCollection);
            }

            //get a reference to the unique pointer corresponding to the current mslevel
            auto& container = spectraByMsLevel[msLevel];

            if ( (!container->empty()) && ( abs( rt - container->getBeginRt() ) > bbWidthManager[msLevel]) ) {
                while ( ! cyclesCollectionQueue.write(std::move(container)))
                  continue;
                //recreate a unique pointer
                unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > c(
                            new mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel));
                c->parentSpectrum = currMs1;
                spectraByMsLevel[msLevel] = std::move(c); // transfer ownership to the map
            }

           if (isInHighRes) {
                auto s = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(_scanCount, _cycleCount, spectrum);
                container->addHighResSpectrum(s);
           } else {
                auto s = std::make_shared<mzSpectrum<l_mz_t, l_int_t> >(_scanCount, _cycleCount, spectrum);
                container->addLowResSpectrum(s);
            }
            ++_scanCount;
        }

        //handles ending
        for(int i = 1; i <= spectraByMsLevel.size(); ++i) {
            auto& container = spectraByMsLevel[i];
            if (! container->empty()) {
                while (! cyclesCollectionQueue.write(std::move(container)))
                  continue;
            }
        }

        //signify that we finished producing
        LOG(INFO) << "Producer finished...\n";
    }

    /**
     *
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    inline void mzCycleCollectionConsumer(folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& cyclesCollectionQueue, //try to get cycleCollection
                                          pwiz::msdata::CVID filetype, mzPeakFinderUtils::PeakPickerParams& params, // peak picking parameters
                                          folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& peakPickedCyclesCollectionQueue,
                                          int nscans) {
        int progress = 0;
        while ( 1 ) {
            //get back the ownership
            unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > cycleCollection(nullptr);
            while (! cyclesCollectionQueue.read(cycleCollection))
              continue;
            if (cycleCollection == nullptr) {
                LOG(INFO) << "Peak picking consumer/producer finished due to null pointer" <<endl;
                break;
            }
            this->launchPeakPicking<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycleCollection, filetype, params);
            //after joining put the cycle collection in the peak picked collection queue
            progress += cycleCollection->size();

            while (! peakPickedCyclesCollectionQueue.write(std::move(cycleCollection)))
              continue;

            if (progress == nscans) {
              LOG(INFO) << "Peak picking consumer/producer finished: reaches final progression" <<endl;
              break;
            }
        }
    }

    /**
     *
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
    inline void mzCycleCollectionInserter(folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& peakPickedCyclesCollectionQueue,
                                          map<int, double>& bbHeightManager,
                                          map<int, map<int, int> >& runSlices,
                                          int& progressionCount, int nscans) {
        int lastPercent = 0;
        while ( 1 ) {
            unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > cycleCollection(nullptr);
            while (! peakPickedCyclesCollectionQueue.read(cycleCollection))
              continue;

            if ( cycleCollection == nullptr ) {
                LOG(INFO) << "Inserter consumer finished to null pointer\n";
                break;
            }
            this->insertScans(cycleCollection);
            vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
            vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
            cycleCollection->getSpectra(highResSpectra, lowResSpectra); //no const since will be deleted in buildAndInsertData
            progressionCount += cycleCollection->size();
            const int& msLevel = cycleCollection->msLevel;
            this->buildAndInsertData(msLevel, bbHeightManager[msLevel], highResSpectra, lowResSpectra, runSlices[msLevel]);

            int newPercent = (int) (((float) progressionCount / nscans * HUNDRED));
            if (newPercent == lastPercent + STEP_PERCENT) {
                printProgBar(newPercent);
                lastPercent = newPercent;
            }

            if (progressionCount == nscans ) {
                LOG(INFO) << "Inserter consumer finished: reaches final progression";
                break;
            }
        }
        printProgBar(HUNDRED);
    }

public:
    PWIZ_API_DECL mzDBWriter(MzDBFile& f, map<int, DataMode>& m, bool compress);
    //mzdbWriter is not copyable, not affectable, may be movable ?
    //PWIZ_API_DECL mzDBWriter(const mzDBWriter &o){}
    //PWIZ_API_DECL mzDBWriter& operator=(const mzDBWriter& o){}
    //PWIZ_API_DECL ~mzDBWriter();


    /**
     * @brief mzDBWriter::writeMzRTreeDB
     * @param noLoss
     * @param ppm
     * @param filename
     * @param nscans
     */
    template<typename h_mz_t, typename h_int_t, typename l_mz_t, typename l_int_t>
    void writeMzRTreeDB(bool noLoss, string& filename, int nscans, int nbCycles, mzPeakFinderUtils::PeakPickerParams& params) {

        clock_t beginTime = clock();
        //open database
        if (filename == "")
            filename = _mzdb.name + ".mzdb";

        if (! nscans)
            nscans = _msdata->run.spectrumListPtr->size();

        //---create peakpicker object
        pwiz::msdata::CVID& filetype = this->_originFileFormat;

        mzPeakFinderUtils::PeakPickerParams p;

        if (filetype == pwiz::msdata::MS_ABI_WIFF_format) {
            if ( ! params.minSNR ) {
                p.minSNR = 0.0;
            }
            p.baseline = params.baseline;
            p.noise = params.noise;

        } else if (filetype == pwiz::msdata::MS_Thermo_RAW_format) {
            if ( params.minSNR) {
                LOG(INFO) <<"Setting minSNR since it is a Thermo  raw data file";
                p.minSNR = 0;
            }
            p.baseline = params.baseline;
            p.noise = params.noise;
        } else {
            LOG(FATAL) << "raw data file not yet supported";
        }

        //create and open database
        if ( _mzdb.open(filename) != SQLITE_OK) {
            LOG(FATAL) << "Can not create database...Locked, space ? Exiting\n";
            exit(0);
        }

        //create tables within a transaction
        this->createTables();

        //insert metadata
        try {
            this->insertMetaData(noLoss);
        } catch (exception& e) {
            LOG(ERROR) << "Error occured during metadata insertion";
            LOG(ERROR) << "\t->" << e.what() << endl << endl;
        }

        pwiz::util::IntegerSet levelsToCentroid;
        for (auto it = this->_msnMode.begin(); it != this->_msnMode.end(); ++it) {
            if ( it->second == CENTROID)
                levelsToCentroid.insert(it->first);
        }

        //bool finishedProperly = true;
        pwiz::msdata::SpectrumListPtr spectrumList = _msdata->run.spectrumListPtr;

        int spectrumListSize = spectrumList->size();
        nscans = (nscans > spectrumListSize) ? spectrumListSize : nscans;
        if (! nscans) {
            LOG(INFO) << "Empty spectrum list...Exiting\n";
            return;
        }
        LOG(INFO) << "SpectrumList size: " << nscans;

        map<int, map<int, int> >runSlices;
        //settings currentRt
        map<int, double> currentRt, bbHeightManager, bbWidthManager;
        currentRt[1] = rtOf(spectrumList->spectrum(0, false));
        bbWidthManager[1] = _mzdb.bbWidth;
        //it is faster to do a multiplication than a division so calculate just once
        //the inverse
        double invms1 = 1.0 / _mzdb.bbHeight;
        double invmsn = 1.0 / _mzdb.bbHeightMsn;

        bbHeightManager[1] = invms1;
        for (int msnb = 2; msnb <= MAX_MS; ++msnb) {
            currentRt[msnb] = 0;
            bbHeightManager[msnb] = invmsn;
            bbWidthManager[msnb] = _mzdb.bbWidthMsn;
        }

        sqlite3_exec(_mzdb.db, "BEGIN;", 0, 0, 0);

// critical code, this code is twice faster than the other one but 1/60 crashes the machine
        //CycleCollectionQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > cyclesCollectionQueue(3);
        //CycleCollectionQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > peakPickedCyclesCollectionQueue(3);
        folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > cyclesCollectionQueue(nbCycles);
        folly::ProducerConsumerQueue<unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > peakPickedCyclesCollectionQueue(nbCycles);

        int progressCount = 0;

        size_t nbProc = boost::thread::hardware_concurrency();
        LOG(INFO) << "#detected core(s): " << nbProc;
        LOG(INFO) << "using #" << 1 << " core(s)";

        //iterate (get spectra for example) on it
        //if (nbProc > 2)
        //   printf("The raw2mzdb power has been unleashed...it could even break your computer!\n");
        boost::thread producer;
        if (this->_originFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
            producer = boost::thread(boost::bind(&mzDBWriter::mzCycleCollectionProducer<h_mz_t, h_int_t, l_mz_t, l_int_t, pwiz::msdata::detail::SpectrumList_Thermo>,
                                                 this,
                                                 std::ref(levelsToCentroid),
                                                 std::ref(cyclesCollectionQueue),
                                                 (pwiz::msdata::detail::SpectrumList_Thermo*)spectrumList.get(),
                                                 nscans,
                                                 std::ref(bbWidthManager)));
        } else if (this->_originFileFormat == pwiz::msdata::MS_ABI_WIFF_format){
            producer = boost::thread(boost::bind(&mzDBWriter::mzCycleCollectionProducer<h_mz_t, h_int_t, l_mz_t, l_int_t, pwiz::msdata::detail::SpectrumList_ABI>,
                                                 this,
                                                 std::ref(levelsToCentroid),
                                                 std::ref(cyclesCollectionQueue),
                                                 (pwiz::msdata::detail::SpectrumList_ABI*)spectrumList.get(),
                                                 nscans,
                                                 std::ref(bbWidthManager)));
        } else {
            LOG(FATAL) << "Constructor file not yet supported. Aborting...";
            exit(0);
        }

        boost::thread_group consumers;
        for (int i = 0; i < 1; ++i) {
            consumers.create_thread(boost::bind(&mzDBWriter::mzCycleCollectionConsumer<h_mz_t, h_int_t, l_mz_t, l_int_t>, this,
                                                std::ref(cyclesCollectionQueue), this->_originFileFormat,
                                                std::ref(p), std::ref(peakPickedCyclesCollectionQueue),
                                                nscans));
        }
        boost::thread inserter(boost::bind(&mzDBWriter::mzCycleCollectionInserter<h_mz_t, h_int_t, l_mz_t, l_int_t>, this,
                                           std::ref(peakPickedCyclesCollectionQueue), std::ref(bbHeightManager),
                                           std::ref(runSlices), std::ref(progressCount), nscans));
        producer.join();
        consumers.join_all();
        inserter.join();

//---end critical

//--------------------------FIX BUT 2x slower-----------------------------------------
//        int lastPercent = 0;
//        float progression = 0;
//        map<int, unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > > spectraByMsLevel;
//        pwiz::msdata::SpectrumPtr currMs1;

//        for (size_t i = 0; i < nscans; ++i) {

//            const pwiz::msdata::SpectrumPtr spectrum = (this->_originFileFormat == pwiz::msdata::MS_Thermo_RAW_format) ?
//                        ((pwiz::msdata::detail::SpectrumList_Thermo*)spectrumList.get())->spectrum(i, true, levelsToCentroid) :
//                         ((pwiz::msdata::detail::SpectrumList_ABI*)spectrumList.get())->spectrum(i, true, levelsToCentroid);

//            const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
//            const float rt = rtOf(spectrum);

//            bool isInHighRes = this->_metadataExtractor->isInHighRes(spectrum);

//            if (msLevel == 1 ) {
//                ++_cycleCount;
//                currMs1 = spectrum;
//            }

//            //init for specific msLevel
//            if (spectraByMsLevel.find(msLevel) == spectraByMsLevel.end()) {
//                unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > cont(
//                      new mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel));
//                cont->parentSpectrum = currMs1;
//                spectraByMsLevel[msLevel] = std::move(cont);
//            }

//            auto& container = spectraByMsLevel[msLevel];

//            if ( (!container->empty()) && ( abs( rt - container->getBeginRt() ) > bbWidthManager[msLevel]) ) {
//                //----Main Data treatment
//                this->launchPeakPicking(container, filetype, p);
//                progression += (float)container->size();
//                vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
//                vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
//                container->getSpectra(highResSpectra, lowResSpectra);
//                this->buildAndInsertData(msLevel, bbHeightManager[msLevel], highResSpectra, lowResSpectra, runSlices[msLevel]);
//                //-----------------------
//                unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> > c(
//                      new mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel));
//                c->parentSpectrum = currMs1;
//                spectraByMsLevel[msLevel] = std::move(c);
//            }


//            if (isInHighRes) {
//                auto s = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(_scanCount, _cycleCount, spectrum);
//                container->addHighResSpectrum(s);
//                this->insertScan<h_mz_t, h_int_t>(s, container->getBeginId(), currMs1);

//            } else {
//                auto s = std::make_shared<mzSpectrum<l_mz_t, l_int_t> >(_scanCount, _cycleCount, spectrum);
//                container->addLowResSpectrum(s);
//                this->insertScan<l_mz_t, l_int_t>(s, container->getBeginId(), currMs1);
//            }

//            ++_scanCount;

//            int newPercent = (int) (progression / nscans * HUNDRED);
//            if (newPercent != lastPercent) {
//                printProgBar(newPercent);
//                lastPercent = newPercent;
//            }
//        }

//        //finish
//        for(int i = 1; i <= spectraByMsLevel.size(); ++i) {
//            auto& container = spectraByMsLevel[i];
//            if (! container->empty())
//                this->launchPeakPicking(container, filetype, p);
//                progression += container->size();
//                vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
//                vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
//                container->getSpectra(highResSpectra, lowResSpectra);
//                this->buildAndInsertData(i, bbHeightManager[i], highResSpectra, lowResSpectra, runSlices[i]);

//                int newPercent = (int) (progression / nscans * HUNDRED);
//                if (newPercent != lastPercent ) {
//                    printProgBar(newPercent);
//                    lastPercent = newPercent;
//                }
//        }
//        printProgBar(100);
//-----------------------END FIX ---------------------------------------




        //--- metadata cv terms (user param and cv param)
        insertCollectedCVTerms();

        //--- set sqlite indexes
        setIndexes();

        sqlite3_exec(_mzdb.db, "COMMIT;", 0, 0, 0);

        //piece of code for doing a backup of the database, in memory test purpose
        /*
        struct sqlite3* dump = 0;
        sqlite3_open_v2( filename.c_str(), &dump, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
        sqlite3_backup* backup = sqlite3_backup_init(dump, "main", _mzdb.db, "main");
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
        */

        clock_t endTime = clock();
        LOG(INFO) << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << endl;
    }


    /**
     * @brief checkAndFixRunSliceNumber
     */
    void checkAndFixRunSliceNumberAnId();

}; //end class
} //end namespace
#endif
