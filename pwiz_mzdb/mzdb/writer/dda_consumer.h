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
 * @file dda_consumer.h
 * @brief Read cycle objects from queue, create and insert data in the mzDB file
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZDDACONSUMER_H
#define MZDDACONSUMER_H

#include "../lib/sqlite3/include/sqlite3.h"

#include "cycle_obj.hpp"
#include "spectrum_inserter.h"
#include "bb_inserter.hpp"
#include "bb_computer.hpp"

namespace mzdb {

/**
 * mzDDAConsumer class
 * ====================
 *
 * The role of this class is to:
 *  - read cycle objects from queue
 *  - insert spectrum in sqlite table `spectrum`
 *  - create and insert run slice in sqlite table `run_slice`
 *  - create and insert bounding box in sqlite table `bounding_box`,
 *    and sqlite virtual tables `bounding_box_rtree`, `bounding_box_msn_rtree`.
 *
 */
template<class QueueingPolicy, class SpectrumListType>
class mzDDAConsumer:  QueueingPolicy, mzSpectrumInserter, mzBBInserter {

    //Some typedef
    typedef typename QueueingPolicy::Obj SpectraContainer;

    typedef typename QueueingPolicy::UPtr SpectraContainerUPtr;

    typedef typename SpectraContainer::h_mz_t h_mz_t;
    typedef typename SpectraContainer::h_int_t h_int_t;
    typedef typename SpectraContainer::l_mz_t l_mz_t;
    typedef typename SpectraContainer::l_int_t l_int_t;

    typedef  std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > HighResSpectrumSPtr;
    typedef  std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > LowResSpectrumSPtr;

private:
    // list of all possible data encodings, those who are really used will be inserted on the fly in spectrum_inserter.h

public:

    /**
     * @param msdata
     * @param serializer
     * @param bbMzWidthByMsLevel
     * @param runSlices
     * @param progressionCount
     * @param nscans
     */
    void _consume( pwiz::msdata::MSDataPtr& msdata,
                   ISerializer::xml_string_writer& serializer,
                   map<int, double>& bbMzWidthByMsLevel,
                   map<int, map<int, int> >& runSlices,
                   int& progressionCount,
                   int nscans)  {

        int lastPercent = 0;
        while ( 1 ) {

            SpectraContainerUPtr cycleCollection(nullptr);

            this->get(cycleCollection);

            if ( cycleCollection == nullptr ) {
                //LOG(WARNING) << "Inserter finished to null pointer\n";
                //continue;
                break;
            }

            // TODO check if spectra are correctly inserted (spectrum values are distinguished for PROFILE and CENTROID/FITTED)
            this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycleCollection, msdata, serializer);

            vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
            vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
            cycleCollection->getSpectra(highResSpectra, lowResSpectra); //no const since will be deleted in buildAndInsertData

            progressionCount += cycleCollection->size();

            const int& msLevel = cycleCollection->msLevel;

            // TODO check if spectra are correctly inserted (spectrum values are distinguished for PROFILE and CENTROID/FITTED)
            double bbMzWidth = (msLevel == 1 ? bbMzWidthByMsLevel[1] : bbMzWidthByMsLevel[2]);
            this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel, bbMzWidth, highResSpectra, lowResSpectra, runSlices[msLevel]);
            
            int newPercent = (int) (((float) progressionCount / nscans * 100.0));
            if (newPercent == lastPercent + 2.0) {
                mzdb::printProgBar(newPercent);
                lastPercent = newPercent;
            }

            if (progressionCount == nscans) {
                //LOG(INFO) << "Inserter consumer finished: reaches final progression";
                break;
            }
        }
    }

    //public:
    mzDDAConsumer(typename QueueingPolicy::QueueType& queue,
                  MzDBFile& mzdbFile,
                  mzParamsCollecter& paramsCollecter,
                  pwiz::msdata::CVID rawFileFormat,
                  vector<DataEncoding> dataEncodings):
        QueueingPolicy(queue),
        mzSpectrumInserter(mzdbFile, paramsCollecter, rawFileFormat, dataEncodings),
        mzBBInserter(mzdbFile) {}
        
    ~mzDDAConsumer() {
        // at the end of the consuming (only one instance of it), write a description of what have been seen and done
        printGlobalInformation();
    }

    /// return thread, need call join
    boost::thread getConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                    ISerializer::xml_string_writer& serializer,
                                    map<int, double>& bbMzWidthByMsLevel,
                                    map<int, map<int, int> >& runSlices,
                                    int& progressionCount,
                                    int nscans ) {
        return boost::thread( //boost::bind(
                              &mzDDAConsumer<QueueingPolicy,
                              SpectrumListType>::_consume,
                              this,
                              std::ref(msdata),
                              std::ref(serializer),
                              std::ref(bbMzWidthByMsLevel),
                              std::ref(runSlices),
                              std::ref(progressionCount),
                              nscans
                              );

    } // end function
};


} // end namespace

#endif // MZDDACONSUMER_H
