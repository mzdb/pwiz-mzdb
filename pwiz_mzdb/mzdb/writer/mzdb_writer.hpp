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
//#include "pwiz/analysis/spectrum_processing/SpectrumList_PrecursorRecalculator.hpp"
//#include "pwiz/data/vendor_readers/Thermo/SpectrumList_Thermo.hpp"
//#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"
#include "boost/thread/thread.hpp"

//--- SQLite import
#include "../lib/sqlite3/include/sqlite3.h"
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

//#include "mzDDAConsumer.h"
//#include "mzDDAProducer.h"

//#include "mzSwathConsumer.hpp"
//#include "mzSwathProducer.hpp"

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
#define MAX_MS 3

#define bb_ms1_mz_width_default 5
#define bb_ms1_rt_width_default 15
#define bb_msn_mz_width_default 10000
#define bb_msn_rt_width_default 0


/// Base class for writing a raw to mzDBFile
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

    /// Two counters in order to know about the conversion progression and, if enabled, the number
    /// of precursor not found at  50ppm in the provided isolation window
    int m_progressionCounter, m_emptyPrecCount;

    /// Boolean enbling or not the compression of each bounding box. Swath mode
    bool m_compress, m_swathMode;

    /// xml serializer using pugi:xml
    ISerializer::xml_string_writer m_serializer;


    //------------------ UTILITY FUNCTIONS -------------------------------------

    /// Create sqlite tables, abort if it fails
    void createTables();

    /// Create indexes
    void createIndexes();

    /// get the good data extractor
    inline std::unique_ptr<mzIMetadataExtractor> getMetadataExtractor() {
#ifdef _WIN32
        switch ( (int) m_originFileFormat ) {
        case (int) pwiz::msdata::MS_Thermo_RAW_format: {
            return  std::unique_ptr<mzIMetadataExtractor>(new mzThermoMetadataExtractor(this->m_mzdbFile.name));
        }
        default: {
            LOG(WARNING) << "Metadata extraction is not supported for this file format:" << pwiz::msdata::cvTermInfo(m_originFileFormat).name;
            return std::unique_ptr<mzIMetadataExtractor>(new mzEmptyMetadataExtractor(this->m_mzdbFile.name));
        }
        }
#else
        return std::unique_ptr<mzIMetadataExtractor>(new mzEmptyMetadataExtractor(this->_mzdb.name));
#endif
    }

    /// Insert controlled vocabulary terms and cv units in the database
    void insertCollectedCVTerms();

public:

    /// build empty needed DataProessing... not const because we modify _msdata
    PWIZ_API_DECL void checkMetaData();

    /// insert metadata into the database
    PWIZ_API_DECL void insertMetaData(bool noLoss);


    /// Function fetching the first cycles to test if we have a swath acquisition
    PWIZ_API_DECL bool isSwathAcquisition();

    /// checkAndFixRunSliceNumber
    PWIZ_API_DECL void checkAndFixRunSliceNumberAnId();

    /// Ctor, receives premiminary tests and structures provided by Pwiz FullListReader
    PWIZ_API_DECL mzDBWriter(MzDBFile& f,
                             std::map<int, DataMode>& m,
                             pwiz::msdata::CVID originFileFormat,
                             pwiz::msdata::MSDataPtr msdata,
                             bool compress);




    /**
     * @brief mzDBWriter::writeMzRTreeDB
     * @param noLoss
     * @param ppm
     * @param filename
     * @param nscans
     */
    template<typename h_mz_t, typename h_int_t,
             typename l_mz_t, typename l_int_t>
    PWIZ_API_DECL void writeMzDB(string& filename,
                                 int nscans,
                                 int nbCycles, // primary needed for swath acquisition
                                 mzPeakFinderUtils::PeakPickerParams& params) {

        //base container
        typedef mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> SpectraContainer;
        typedef std::unique_ptr<SpectraContainer> SpectraContainerUPtr;

        // queue definition
        typedef folly::ProducerConsumerQueue<SpectraContainerUPtr> FollyQueue;
        typedef BlockingQueue<SpectraContainerUPtr> MzDBQueue;

        // queue managing policy
        typedef FollyQueueingPolicy<SpectraContainerUPtr> FollyNonBlockingQueueing;
        typedef BlockingQueueingPolicy<SpectraContainerUPtr> MzDBBlockingQueueing;

        clock_t beginTime = clock();

        //open database
        if (filename == "")
            filename = m_mzdbFile.name + ".mzDB";

        if (! nscans)
            nscans = m_msdata->run.spectrumListPtr->size();

        //create and open database
        if ( m_mzdbFile.open(filename) != SQLITE_OK) {
            LOG(FATAL) << "Can not create database...Locked, space ? Exiting\n";
            exit(0);
        }

        //create tables within a transaction
        this->createTables();

        _TRY_BEGIN
            this->insertMetaData(false);
        _CATCH(exception& e)
            LOG(ERROR) << "Error occured during metadata insertion.";
            LOG(ERROR) << "\t->" << e.what();
        _CATCH_END

        //--- always prefer vendor centroiding
        pwiz::util::IntegerSet levelsToCentroid;
        for (auto it = this->m_dataModeByMsLevel.begin(); it != this->m_dataModeByMsLevel.end(); ++it) {
            if ( it->second == CENTROID)
                levelsToCentroid.insert(it->first);
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
        map<int, double> bbHeightManager, bbWidthManager; //currentRt
        //currentRt[1] = PwizHelper::rtOf(spectrumList->spectrum(0, false));
        bbWidthManager[1] = m_mzdbFile.bbWidth;

        //it is faster to do a multiplication than a division so calculate just once the inverse
        double invms1 = 1.0 / m_mzdbFile.bbHeight;
        double invmsn = 1.0 / m_mzdbFile.bbHeightMsn;

        bbHeightManager[1] = invms1;
        for (int msnb = 2; msnb <= MAX_MS; ++msnb) {
            //currentRt[msnb] = 0;
            bbHeightManager[msnb] = invmsn;
            bbWidthManager[msnb] = m_mzdbFile.bbWidthMsn;
        }

        //begin a new transaction
        sqlite3_exec(m_mzdbFile.db, "BEGIN;", 0, 0, 0);

        int progressCount = 0;

        size_t nbProc = boost::thread::hardware_concurrency();
        LOG(INFO) << "#detected core(s): " << nbProc;

        //iterate (get spectra for example) on it
        //increasing nbCycles leads to an increase of performance (time of conversion) but can use many more RAM
        //FollyQueue follyQueue(100);
        MzDBQueue mzdbQueue(100);

        PCBuilder<MzDBBlockingQueueing, mzMultiThreadedPeakPicker> pcThreadBuilder(
                    mzdbQueue, m_mzdbFile, m_paramsCollecter,
                    m_originFileFormat, m_dataModeByMsLevel
        );

        if (m_originFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
            //LOG(INFO) << "Thermo raw file format detected";

            auto* spectrumListThermo =
                    static_cast<pwiz::msdata::detail::SpectrumList_Thermo*>(spectrumList.get());

            if (m_swathMode) {
                auto prod = pcThreadBuilder.getDIAThermoProducerThread(levelsToCentroid, spectrumListThermo,
                                                                       nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDIAThermoConsumerThread(m_msdata, m_serializer,
                                                                       bbHeightManager, runSlices, progressCount, nscans);
                prod.join(); cons.join();

            } else {

                //LOG(INFO) << "Data dependant acquisition detected";
                auto prod = pcThreadBuilder.getDDAThermoProducerThread( levelsToCentroid, spectrumListThermo,
                                                                        nscans, bbWidthManager, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDDAThermoConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                        runSlices, progressCount, nscans );
                prod.join(); cons.join();
            }


        }
        else if (m_originFileFormat == pwiz::msdata::MS_ABI_WIFF_format) {
            auto* s = static_cast<pwiz::msdata::detail::SpectrumList_ABI*>(spectrumList.get());

            //LOG(INFO) << "AB Sciex file format detected";
            if (m_swathMode) {
                //LOG(INFO) << "Swath mode detected";


                auto prod = pcThreadBuilder.getSwathABIProducerThread( levelsToCentroid, s,
                                                                       nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getSwathABIConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                       runSlices, progressCount, nscans );
                prod.join(); cons.join();

            } else {
                //LOG(INFO) << "Data dependant acquisition detected";

                auto prod = pcThreadBuilder.getDDAABIProducerThread( levelsToCentroid, s,
                                                                     nscans, bbWidthManager, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getDDAABIConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                     runSlices, progressCount, nscans );

                prod.join(); cons.join();
            }
        }

        else {
            //LOG(INFO) << "mzML / mzXML spectrumList";

            if (m_swathMode) {
               // LOG(INFO) << "Swath mode detected";


                auto prod = pcThreadBuilder.getSwathGenericProducerThread( levelsToCentroid, spectrumList.get(),
                                                                           nscans, m_originFileFormat, params);
                auto cons = pcThreadBuilder.getSwathGenericConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                           runSlices, progressCount, nscans );
                prod.join(); cons.join();

            } else {
                //LOG(INFO) << "Data dependant acquisition detected";

                auto prod = pcThreadBuilder.getDDAGenericProducerThread( levelsToCentroid, spectrumList.get(),
                                                                         nscans, bbWidthManager, m_originFileFormat,params);
                auto cons = pcThreadBuilder.getDDAGenericConsumerThread( m_msdata, m_serializer, bbHeightManager,
                                                                         runSlices, progressCount, nscans );

                prod.join();cons.join();
            }

        }
        //--- metadata cv terms (user param and cv param)
        m_paramsCollecter.insertCollectedCVTerms();

        //--- set sqlite indexes
        this->createIndexes();

        sqlite3_exec(m_mzdbFile.db, "COMMIT;", 0, 0, 0);
        LOG(INFO) << "Precursors not found at 50ppm (using vendor peak-piking) count: "<< this->m_emptyPrecCount;

        clock_t endTime = clock();
        LOG(INFO) << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << endl;
    }

     /// TODO: Make this clearer: no mslevel intervention, but instead the notion of HighResMode
     /// LowResMode
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
