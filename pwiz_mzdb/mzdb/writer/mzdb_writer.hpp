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

#ifdef _WIN32
#include "ppl.h"
#endif

//--- Pwiz import
#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz_tools/common/FullReaderList.hpp"
#include "pwiz/data/msdata/IO.hpp"
#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h"

//---version
#include "pwiz/Version.hpp"
#include "pwiz/data/msdata/Version.hpp"
#include "pwiz/analysis/Version.hpp"


//#include "pwiz/analysis/spectrum_processing/SpectrumList_PrecursorRecalculator.hpp"
//#include "pwiz/data/vendor_readers/Thermo/SpectrumList_Thermo.hpp"
//#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"
#include "boost/thread/thread.hpp"

//--- SQLite import
//#include "../lib/sqlite3/include/sqlite3.h"
#include "libraries/SQLite/sqlite3.h"
#include "../lib/pugixml/include/pugixml.hpp"

//--- MzDB import
#include "../utils/mzDBFile.h"
#include "../utils/mzISerializer.h"

#include "../threading/Queue.hpp"
#include "../threading/SpScQueue.hpp"

#include "cycle_obj.hpp"
#include "user_text.h"

#include "metadata_extractor.hpp"
#include "bb_computer.hpp"

#include "params_collecter.h"
#include "queueing_policy.hpp"

#include "prod_cons_builder.hpp"

#include "threaded_peak_picker.h"

//--- Compression import
#ifdef USE_COMPRESSION
#include "../utils/lz4.h"
#include "../utils/md5.hpp"
#endif

using namespace std;

namespace mzdb {

#define STEP_PERCENT 2
#define HUNDRED 100
#define MAX_MS 2

#define bb_ms1_mz_width_default 5
#define bb_ms1_rt_width_default 15
#define bb_msn_mz_width_default 10000
#define bb_msn_rt_width_default 0

/**
 * @brief The mzDBWriter class
 */
class mzDBWriter: private boost::noncopyable {

protected:

    /// Direct reference to the newly created mzdb file
    MzDBFile& m_mzdbFile;

    /// This is the common data structure provided by Pwiz, holding all data about one experiment
    pwiz::msdata::MSDataPtr m_msdata;

    /// Origin file format. Defined with Pwiz method calling. It only cares about the previous format
    /// resulting from a conversion  or not
    pwiz::msdata::CVID m_originFileFormat;

    ///Several metadata extractor exist. Thermo for example, provides many metadata instead
    /// of ABI.
    std::unique_ptr<mzIMetadataExtractor> m_metadataExtractor;

    /// Will collect all controlled-vocabulary or user-defined parameters, on pwiz::msdata::
    /// ParamContainer objects.
    mzParamsCollecter m_paramsCollecter;

    /// Useful to determine in which DataMode (FITTED, PROFILE, CENTROID) the user want
    /// to convert each mslevel
    map<int, DataMode>& m_dataModeByMsLevel;

    /// map containing data encoding ID (ID which will be registered in the DB) as key and
    /// the corresponding `DataEncoding` object
    /// @see DataEncoding
    map<int, DataEncoding> m_dataEncodingByID;

    /// counter in order to know about the conversion progression
    int m_progressionCounter;

    /// number of precursor not found at  50ppm in the provided isolation window
    /// when refining precursor enabled
    int m_emptyPrecCount;

    /// Boolean enbling or not the compression of each bounding box.
    bool m_compress;

    /// Swath mode enabled or not
    bool m_swathMode;

    /// xml serializer using pugi:xml
    ISerializer::xml_string_writer m_serializer;


    //------------------ UTILITY FUNCTIONS -------------------------------------

    /**
     * @brief hasDataMode
     * Test if a dataMode exists in the requested datamodes by the user
     * only useful to compute member `m_dataEncodingByID`.
     *
     * @param mode: DataMode tested
     * @return bool if the datamode exists or not in the map
     * @see buildDataEncodingRowByID
     */
    inline bool hasDataMode(DataMode mode) {
        for (auto it = m_dataModeByMsLevel.begin(); it != m_dataModeByMsLevel.end(); ++it) {
            if (it->second == mode) return true;
        }
        return false;
    }

    /**
     * @brief buildDataEncodingRowByID
     * Function that computes the dataEncoding ID from the DataMode wanted by msLevel
     */
    void buildDataEncodingRowByID();

    /**
     * @brief createTables
     * Create sqlite tables, abort if it fails
     */
    void createTables();

    /**
     * @brief createIndexes
     * Create indexes, if fails lead to very poor performance
     */
    void createIndexes();

    /**
     * @brief getMetadataExtractor
     * Create the appropriate metadata extractor given input file format
     * @see m_originFileFormat
     *
     * @return unique_ptr subclass of mzIMetadataExtractor
     * @see mzIMetadataExtractor
     */
    inline std::unique_ptr<mzIMetadataExtractor> getMetadataExtractor() {
#ifdef _WIN32
        switch ( (int) m_originFileFormat ) {
        case (int) pwiz::msdata::MS_Thermo_RAW_format: {
            return  std::unique_ptr<mzIMetadataExtractor>(new mzThermoMetadataExtractor(this->m_mzdbFile.name));
        }
        case (int) pwiz::msdata::MS_ABI_WIFF_format: {
            LOG(INFO) << "AB Sciex metadata extractor created";
            return std::unique_ptr<mzIMetadataExtractor>(new mzABSciexMetadataExtractor(this->m_mzdbFile.name));
        }
        default: {
            LOG(WARNING) << "Metadata extraction is not supported for this file format:" << pwiz::msdata::cvTermInfo(m_originFileFormat).name;
            return std::unique_ptr<mzIMetadataExtractor>(new mzEmptyMetadataExtractor(this->m_mzdbFile.name));
        }
        }
#else
        //---fallback
        return std::unique_ptr<mzIMetadataExtractor>(new mzEmptyMetadataExtractor(this->_mzdb.name));
#endif
    }

    /**
     * @brief insertCollectedCVTerms
     * Insert controlled vocabulary terms and cv units in the database
     */
    void insertCollectedCVTerms();

public:

    /**
     * @brief checkMetaData
     *  build and modify missing metadata
     *  Note: not const because we modify _msdata
     */
    PWIZ_API_DECL void checkMetaData();

    /**
     * @brief insertMetaData
     * insert metadata bundle into the database
     */
    PWIZ_API_DECL void insertMetaData();

    /**
     */
    inline PWIZ_API_DECL void setSwathAcquisition(bool swath) {m_swathMode = swath;}

    /**
     */
    inline PWIZ_API_DECL bool getSwathAcquisition() {return m_swathMode;}

    /**
     * @brief isSwathAcquisition()
     * Function fetching the first cycles to test if we have a swath acquisition
     */
    PWIZ_API_DECL bool isSwathAcquisition();

    /**
     * @brief checkAndFixRunSliceNumberAnId
     */
    PWIZ_API_DECL void checkAndFixRunSliceNumberAnId();

    /**
     * @brief mzDBWriter
     * Primary ctor, receive import parameters for performing the conversion
     *
     * @param f: MzDBFile
     * @param msdata: MSDataPtr pwiz object resulting of the pwiz reading of the raw file
     * @param originFileFormat
     * @param m: map containing msLevel vs DataMode
     * @param compress: using compression performed by Numpress algorithm
     * @return
     */
    PWIZ_API_DECL mzDBWriter(MzDBFile& f,
                             pwiz::msdata::MSDataPtr msdata,
                             pwiz::msdata::CVID originFileFormat,
                             std::map<int, DataMode>& m,
                             bool compress);

    /**
     * @brief mzDBWriter::writeMzDB
     *
     * @param filenam: path of raw file to convert, could be also mzML, mzXML
     * @param nscans: #scans to convert into the mzdb file form the beginning
     * @param nbCycles
     * @param params: specify the algorithm and parameters used for peak picking to perform profile to fitted mode
     */
    template<typename h_mz_t, typename h_int_t,
             typename l_mz_t, typename l_int_t>
    PWIZ_API_DECL void writeMzDB(string& filename,
                                 int nscans,
                                 int nbCycles, // primary needed for swath acquisition
                                 mzPeakFinderUtils::PeakPickerParams& params) {

        //---base container
        typedef mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> SpectraContainer;
        typedef std::unique_ptr<SpectraContainer> SpectraContainerUPtr;

        //---queue definition
        typedef folly::ProducerConsumerQueue<SpectraContainerUPtr> FollyQueue;
        typedef BlockingQueue<SpectraContainerUPtr> MzDBQueue;

        //---queue managing policy
        typedef FollyQueueingPolicy<SpectraContainerUPtr> FollyNonBlockingQueueing;
        typedef BlockingQueueingPolicy<SpectraContainerUPtr> MzDBBlockingQueueing;

        //---open database
        if (filename == "")
            filename = m_mzdbFile.name + ".mzDB";

        if (! nscans)
            nscans = m_msdata->run.spectrumListPtr->size();

        //---just log sqlite version for information
        printf("\n");
        LOG(INFO) << "SQLITE VERSION: " << SQLITE_VERSION;
        LOG(INFO) << "ProteoWizard release: " << pwiz::Version::str() << " (" << pwiz::Version::LastModified() << ")";
        LOG(INFO) << "ProteoWizard MSData: " << pwiz::msdata::Version::str() << " (" << pwiz::msdata::Version::LastModified() << ")\n";
        //LOG(INFO) << "ProteoWizard Analysis: " << pwiz::analysis::Version::str() << " (" << pwiz::analysis::Version::LastModified() << ")";

        //---create and open database
        if ( m_mzdbFile.open(filename) != SQLITE_OK) {
            LOG(FATAL) << "Can not create database...Locked, space ? Exiting\n";
            exit(0);
        }

        //---create tables within a transaction
        this->createTables();

        try {
            this->insertMetaData();
        } catch(exception& e) {
            LOG(ERROR) << "Error occured during metadata insertion.";
            LOG(ERROR) << "\t->" << e.what();
        } catch(...) {
            LOG(ERROR) << "Unknown error during metadata insertion.";
        }

        //--- always prefer vendor centroiding
        pwiz::util::IntegerSet levelsToCentroid;
        for (auto it = this->m_dataModeByMsLevel.begin(); it != this->m_dataModeByMsLevel.end(); ++it) {
            auto dm = it->second;
            if ( dm == CENTROID || (m_originFileFormat == pwiz::msdata::MS_ABI_WIFF_format && dm == FITTED)) {
                levelsToCentroid.insert(it->first);
            }
        }

        pwiz::msdata::SpectrumListPtr spectrumList = m_msdata->run.spectrumListPtr;

        int spectrumListSize = spectrumList->size();
        nscans = (nscans > spectrumListSize) ? spectrumListSize : nscans;

        if (! nscans) {
            LOG(INFO) << "Empty spectrum list...Exiting\n";
            return;
        }

        LOG(INFO) << "SpectrumList size: " << nscans;

        map<int, map<int, int> > runSlices;
        map<int, double> bbHeightManager, bbWidthManager;

        //---faster to do a multiplication than a division so calculate just once the inverse
        double invms1 = 1.0 / m_mzdbFile.bbHeight;
        double invmsn = 1.0 / m_mzdbFile.bbHeightMsn;

        bbHeightManager[1] = invms1;
        bbWidthManager[1] = m_mzdbFile.bbWidth;

        for (int msnb = 2; msnb <= MAX_MS; ++msnb) {
            if (! this->getSwathAcquisition()) {
                bbHeightManager[msnb] = invmsn;
                bbWidthManager[msnb] = m_mzdbFile.bbWidthMsn;
            } else {
                // in swath acquisition bb sizes are the same as DDA msLevel1
                bbHeightManager[msnb] = invms1;
                bbWidthManager[msnb] = m_mzdbFile.bbWidth;
            }
        }

        //---begin a new transaction
        sqlite3_exec(m_mzdbFile.db, "BEGIN;", 0, 0, 0);

        int progressCount = 0;

        size_t nbProc = boost::thread::hardware_concurrency();
        LOG(INFO) << "#detected core(s): " << nbProc;

        // iterate (get spectra for example) on it. Increasing nbCycles leads to an increase of performance
        // (time of conversion) but can use many more RAM. Could also use non blocking FollyQueue follyQueue(100)
        // from facebook;
        MzDBQueue mzdbQueue(100);
        PCBuilder<MzDBBlockingQueueing, mzMultiThreadedPeakPicker> pcThreadBuilder(
                    mzdbQueue, m_mzdbFile, m_paramsCollecter,
                    m_originFileFormat, m_dataModeByMsLevel, m_dataEncodingByID);

        if (m_originFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
            LOG(INFO) << "Thermo raw file format detected";
            auto* spectrumListThermo =
                    static_cast<pwiz::msdata::detail::SpectrumList_Thermo*>(spectrumList.get());

            if (m_swathMode) {
                LOG(INFO) << "DIA producer/consumer";
                auto prod = pcThreadBuilder.getDIAThermoProducerThread(levelsToCentroid, spectrumListThermo,
                                                                       nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDIAThermoConsumerThread(m_msdata, m_serializer,
                                                                       bbHeightManager, runSlices, progressCount, nscans);
                prod.join(); cons.join();

            } else {
                LOG(INFO) << "DDA producer/consumer";
                auto prod = pcThreadBuilder.getDDAThermoProducerThread( levelsToCentroid, spectrumListThermo,
                                                                        nscans, bbWidthManager, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDDAThermoConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                        runSlices, progressCount, nscans );
                prod.join(); cons.join();
            }
        }
        else if (m_originFileFormat == pwiz::msdata::MS_ABI_WIFF_format) {
            auto* s = static_cast<pwiz::msdata::detail::SpectrumList_ABI*>(spectrumList.get());
            LOG(INFO) << "AB Sciex file format detected";
            if (m_swathMode) {
                LOG(INFO) << "Swath producer/consumer";
                auto prod = pcThreadBuilder.getSwathABIProducerThread( levelsToCentroid, s,
                                                                       nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getSwathABIConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                       runSlices, progressCount, nscans );
                prod.join(); cons.join();

            } else {
                LOG(INFO) << "DDA producer/consumer";

                auto prod = pcThreadBuilder.getDDAABIProducerThread( levelsToCentroid, s,
                                                                     nscans, bbWidthManager, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDDAABIConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                     runSlices, progressCount, nscans );
                prod.join(); cons.join();
            }
        }

        else {
            LOG(INFO) << "Beta support spectrumList";
            if (m_swathMode) {
                LOG(INFO) << "Swath producer/consumer";
                auto prod = pcThreadBuilder.getSwathGenericProducerThread( levelsToCentroid, spectrumList.get(),
                                                                           nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getSwathGenericConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                           runSlices, progressCount, nscans );
                prod.join(); cons.join();

            } else {
                LOG(INFO) << "DDA producer/consumer";
                auto prod = pcThreadBuilder.getDDAGenericProducerThread( levelsToCentroid, spectrumList.get(),
                                                                         nscans, bbWidthManager, m_originFileFormat,params);
                auto cons = pcThreadBuilder.getDDAGenericConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                         runSlices, progressCount, nscans );
                prod.join(); cons.join();
            }

        } // end file type

        //--- metadata cv terms (user param and cv param)
        m_paramsCollecter.insertCollectedCVTerms();

        //--- insert tmp_spectrum into permanent spectrum table
        LOG(INFO) << "Creating permanent spectrum table";
        sqlite3_exec(m_mzdbFile.db, "CREATE TABLE spectrum AS SELECT * from tmp_spectrum;", 0, 0, 0);


        //--- set sqlite indexes
        this->createIndexes();

        //---finally commit
        sqlite3_exec(m_mzdbFile.db, "COMMIT;", 0, 0, 0);

        const char* query = "INSERT INTO main.sqlite_sequence(name, seq) VALUES ('spectrum', ?);";
        sqlite3_prepare_v2(m_mzdbFile.db, query, -1, &(m_mzdbFile.stmt), 0);
        sqlite3_bind_int(m_mzdbFile.stmt, 1, nscans);
        sqlite3_step(m_mzdbFile.stmt);

        //finalize statement
        sqlite3_finalize(m_mzdbFile.stmt);

        //--- in case of precursor calculations
        //LOG(INFO) << "Precursors not found at 50ppm (using vendor peak-piking) count: "<< this->m_emptyPrecCount;

    }

    /// All informations are encoded as 64-bits double.
    void PWIZ_API_DECL writeNoLossMzDB(string& filename,
                                       int nscans,
                                       int nbCycles,
                                       mzPeakFinderUtils::PeakPickerParams& params) {
        this->writeMzDB<double, double, double, double>(filename, nscans, nbCycles, params);
    }

    /// Mz of all spectrum are encoded as 64-bits double. Intensities are stored
    /// as 32-bits float
    void PWIZ_API_DECL  writeMzDBMzHi(string& filename,
                                      int nscans,
                                      int nbCycles,
                                      mzPeakFinderUtils::PeakPickerParams& params) {
        this->writeMzDB<double, float, double, float>(filename, nscans, nbCycles, params);

    }

    /// only Mz of spectrum from msLevel = 1 are converted into 64-bits double
    void PWIZ_API_DECL  writeMzDBMzMs1Hi(string& filename,
                                         int nscans,
                                         int nbCycles,
                                         mzPeakFinderUtils::PeakPickerParams& params) {
        this->writeMzDB<double, float, float, float>(filename, nscans, nbCycles, params);
    }

    /// all data encoded as 32-bits float. Useful for low resolution instrument ?
    void PWIZ_API_DECL  writeMzDBAllLow(string& filename,
                                        int nscans,
                                        int nbCycles,
                                        mzPeakFinderUtils::PeakPickerParams& params) {
        this->writeMzDB<float, float, float, float>(filename, nscans, nbCycles, params);
    }


}; //end class
} //end namespace
#endif
