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
 * @file swath_consumer.hpp
 * @brief Read cycle objects from queue, create and insert data in the mzDB file
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZSWATHCONSUMER_HPP
#define MZSWATHCONSUMER_HPP

#include <set>

#include "../lib/sqlite3/include/sqlite3.h"

#include "cycle_obj.hpp"
#include "spectrum_inserter.h"
#include "bb_inserter.hpp"
#include "bb_computer.hpp"

using namespace std;

namespace mzdb {

/**
 * Same function than mzDDAConsumer but adapted to DIA/SWATH
 * @ see mzDDAConsumer
 */
template<class QueueingPolicy,
         class SpectrumListType> // Wiff or raw proteowizard object
class  mzSwathConsumer:  QueueingPolicy, mzSpectrumInserter, mzBBInserter {

    typedef typename QueueingPolicy::Obj SpectraContainer;
    typedef  typename QueueingPolicy::UPtr SpectraContainerUPtr;

    typedef typename SpectraContainer::h_mz_t h_mz_t;
    typedef typename SpectraContainer::h_int_t h_int_t;
    typedef typename SpectraContainer::l_mz_t l_mz_t;
    typedef typename SpectraContainer::l_int_t l_int_t;

    typedef  std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > HighResSpectrumSPtr;
    typedef  std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > LowResSpectrumSPtr;

private:

    /// Contains predefined numbers of cycles, TODO: parametrize cycles number
    vector<SpectraContainerUPtr> m_cycles;

    /// Set containing the beginning of each start of isolation window in swath mode
    vector<double> m_isolationWindowStarts; // for fast prec finding
    vector<double> m_isolationWindowSizes;

    void _consume( pwiz::msdata::MSDataPtr msdata,
                   ISerializer::xml_string_writer& serializer,
                   map<int, double>& bbMzWidthByMsLevel,
                   double& ms1RtWidth,
                   map<int, map<int, int> >& runSlices,
                   int& progressionCount,
                   int nscans ) {
        int lastPercent = 0;
        this->_determineIsolationWindowStarts(msdata);
        
        while( 1 ) {
            SpectraContainerUPtr cycleCollection(nullptr);
            this->get(cycleCollection);

            // create BB and RS when cycleCollection are grouper by RT of 15 (default value)
            if(!m_cycles.empty() && m_cycles.back()->getBeginRt() - m_cycles.front()->getBeginRt() > ms1RtWidth) {
                // put last cycleCollection away (to have a collection that remains below the ms1RtWidth window)
                SpectraContainerUPtr lastContainer = std::move(m_cycles.back());
                if(!m_cycles.empty()) m_cycles.erase(m_cycles.end() - 1);
                size_t nbCycles = m_cycles.size();
                
                // create and insert BB and RS on the current group of cycles
                _createAndInsertBBs(bbMzWidthByMsLevel, runSlices, progressionCount);
                
                // clear container and put back last container
                m_cycles.clear();
                m_cycles.push_back(std::move(lastContainer));
            }

            // quit loop when cycleCollection is null
            if(cycleCollection == nullptr) break;
            
            // otherwise, insert scans in the database and append cycleCollection to the current group of cycleCollection
            this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycleCollection, msdata, serializer, cycleCollection->parentSpectrum->id);
            m_cycles.push_back(std::move(cycleCollection));

            // deal with progression counter
            int newPercent = (int) (((float) progressionCount / nscans * 100.0));
            if (newPercent == lastPercent + 2.0) {
                mzdb::printProgBar(newPercent);
                lastPercent = newPercent;
            }
        }
        // when exiting the loop, create and insert BB and RS for the remaining cycles
        if(!m_cycles.empty()) {
            _createAndInsertBBs(bbMzWidthByMsLevel, runSlices, progressionCount);
        }
        // exit properly
        printProgBar(100);
    }
    
    void _createAndInsertBBs(map<int, double>& bbMzWidthByMsLevel, map<int, map<int, int> >& runSlices, int& progressionCount) {
        // prepare variables before inserting scans
        vector<HighResSpectrumSPtr> ms1Spectra;
        int bbFirstMs1ScanID = m_cycles.front()->parentSpectrum->id;
        map<int, int> firstBBSpectrumIDBySpectrumID;
        map<int, vector<HighResSpectrumSPtr> > ms2SpectraByPrecursorMzIndex;
        
        for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
            auto& cycle = *it;
            ms1Spectra.push_back(cycle->parentSpectrum);
            vector<HighResSpectrumSPtr> highResSpec;
            vector<LowResSpectrumSPtr> lowResSpec;
            cycle->getSpectra(highResSpec, lowResSpec);
            for (auto specIt = highResSpec.begin(); specIt != highResSpec.end(); ++specIt) {
                auto& spec = *specIt;
                auto windowIndex = this->_findIsolationWindowIndexForMzPrec(spec->precursorMz());
                ms2SpectraByPrecursorMzIndex[windowIndex].push_back(spec);
            }
        }
        progressionCount += ms1Spectra.size();
        // handle MS1 spectra
        this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(1, bbMzWidthByMsLevel[1], ms1Spectra, vector<LowResSpectrumSPtr>(), runSlices[1]);
        // handle MS2 spectra
        for (auto mapIt = ms2SpectraByPrecursorMzIndex.begin(); mapIt != ms2SpectraByPrecursorMzIndex.end(); ++mapIt) {
            auto& windowIndex = mapIt->first;
            auto& ms2Spectra = mapIt->second;
            for (auto specIt = ms2Spectra.begin(); specIt != ms2Spectra.end(); ++specIt) {
                firstBBSpectrumIDBySpectrumID[(*specIt)->id] = ms2Spectra.front()->id;
            }
            progressionCount += ms2Spectra.size();
            auto minPrecMz = m_isolationWindowStarts[windowIndex];
            auto maxPrecMz = minPrecMz + m_isolationWindowSizes[windowIndex];
            this->buildAndInsertData(2, bbMzWidthByMsLevel[2], ms2Spectra, vector<LowResSpectrumSPtr>(), runSlices[2], minPrecMz, maxPrecMz);
        }
    }

    /// For now, this is really complicated to get the good swath isolation window sizes
    void _determineIsolationWindowStarts(pwiz::msdata::MSDataPtr msdata) {
        vector<double> refTargets = PwizHelper::determineIsolationWindowStarts(msdata->run.spectrumListPtr);
        if(refTargets.size() > 0) {
            for(int i = 0; i < refTargets.size() - 1; i++) {
                int windowIndex = i + 1;
                float windowStartMz = refTargets[i];
                float windowSize = refTargets[i+1] - refTargets[i];
                m_isolationWindowStarts.push_back(refTargets[i]);
                m_isolationWindowSizes.push_back(refTargets[i+1] - refTargets[i]);
                //LOG(INFO) << "Swath Window #" << windowIndex <<", Start: " << windowStartMz << ",  Size: " <<  windowSize;
            }
        }
    }

    ///
    int _findIsolationWindowIndexForMzPrec(double mz) {
        for(int i = 0; i < m_isolationWindowStarts.size() - 1; i++) {
            if(mz >= m_isolationWindowStarts[i] && mz < m_isolationWindowStarts[i+1]) return i;
        }
        return m_isolationWindowStarts.size() - 1;
    }

public:

    mzSwathConsumer( typename QueueingPolicy::QueueType& queue,
                     MzDBFile& mzdbFile,
                     mzParamsCollecter& paramsCollecter,
                     pwiz::msdata::CVID rawFileFormat,
                     vector<DataEncoding> dataEncodings):
        QueueingPolicy(queue),
        mzSpectrumInserter(mzdbFile, paramsCollecter, rawFileFormat, dataEncodings),
        mzBBInserter(mzdbFile) {
    }
    
    ~mzSwathConsumer() {
        // at the end of the consuming (only one instance of it), write a description of what have been seen and done
        printGlobalInformation();
    }

    boost::thread getConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                    ISerializer::xml_string_writer& serializer,
                                    map<int, double>& bbMzWidthByMsLevel,
                                    double& ms1RtWidth,
                                    map<int, map<int, int> >& runSlices,
                                    int& progressionCount,
                                    int nscans ) {
        return boost::thread(
                             &mzSwathConsumer<QueueingPolicy,
                             SpectrumListType>::_consume,
                             this,
                             std::ref(msdata),
                             std::ref(serializer),
                             std::ref(bbMzWidthByMsLevel),
                             std::ref(ms1RtWidth),
                             std::ref(runSlices),
                             std::ref(progressionCount),
                             nscans
                             );

    }

}; //end class

} // end namespace mzdb

#endif // MZSWATHCONSUMER_HPP
