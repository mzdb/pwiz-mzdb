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

    /// TODO get the value from MetadataExtractor if possible
    //static const double swathStart; // = 400.0;

    /// Contains predefined numbers of cycles, TODO: parametrize cycles number
    vector<SpectraContainerUPtr> m_cycles;

    /// Set containing the beginning of each start of isolation window in swath mode
    vector<double> m_isolationWindowStarts; // for fast prec finding
    vector<double> m_isolationWindowSizes;

    /// in case of fixed swath isolation window
    double m_swathIsolationWindow;

    const double swathStart;

    bool m_isSwathIsolationWindowComputed;

    //-----------------------------------------------------------------------------------------------------------
    void _consume( pwiz::msdata::MSDataPtr msdata,
                   ISerializer::xml_string_writer& serializer,
                   map<int, double>& bbMzWidthByMsLevel,
                   double& ms1RtWidth,
                   map<int, map<int, int> >& runSlices,
                   int& progressionCount,
                   int nscans ) {

        int lastPercent = 0;
        bool toBeBroken = false;

        while ( 1 ) {

            if (toBeBroken)
                break;

            while (m_cycles.empty() || m_cycles.back()->getBeginRt() - m_cycles.front()->getBeginRt() <= ms1RtWidth) { //size() <  4) {
                SpectraContainerUPtr cycleCollection(nullptr);

                this->get(cycleCollection);
                if ( cycleCollection == nullptr ) {
                    //LOG(INFO) << "Inserter consumer finished du to null pointer\n";
                    toBeBroken = true;
                    break;
                }
                if (! m_isSwathIsolationWindowComputed) {
                    this->_determineIsolationWindowStarts(msdata);
                }

                m_cycles.push_back(std::move(cycleCollection));

            }

            if(!toBeBroken) {
                SpectraContainerUPtr lastContainer = std::move(m_cycles.back());
    
                if (! m_cycles.empty())
                    m_cycles.erase(m_cycles.end() - 1);
    
                vector<HighResSpectrumSPtr> ms1Spectra;
    
                int bbFirstMs1ScanID = m_cycles.front()->parentSpectrum->id;
                //int bbFirstMs2ScanID = m_cycles.front()->getBeginId();
    
                for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
                    ms1Spectra.push_back( (*it)->parentSpectrum);
                }
    
                //handle first level
                progressionCount += ms1Spectra.size();
                this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(1, //msLevel
                                                                           bbMzWidthByMsLevel[1],
                                                                           ms1Spectra, //highresspectra
                                                                           vector<LowResSpectrumSPtr>(), //empty lowres spectra
                                                                           runSlices[1]);
    
                map<int, int> firstBBSpectrumIDBySpectrumID;
                this->_buildMSnBB(bbMzWidthByMsLevel, progressionCount, runSlices, firstBBSpectrumIDBySpectrumID);
    
                //insert scans using mzSpectrumInserter, insert cycle consecutively
                for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
                    //ms1Spectra.push_back( (*it)->parentSpectrum);
                    this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(*it, msdata, serializer, bbFirstMs1ScanID, &firstBBSpectrumIDBySpectrumID);
                }
    
                m_cycles.clear();
                m_cycles.push_back(std::move(lastContainer));
    
                // progression update computing the new progression
                int newPercent = (int) (((float) progressionCount / nscans * 100));
                if (newPercent == lastPercent + 2) {
                    printProgBar(newPercent);
                    lastPercent = newPercent;
                }
    
                if (progressionCount >= nscans ) {
                    //LOG(INFO) << "Inserter consumer finished: reaches final progression";
                    break;
                }

            } else {
                
                // last loop
                vector<HighResSpectrumSPtr> ms1Spectra;
                int bbFirstMs1ScanID = m_cycles.front()->parentSpectrum->id;
                for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
                    ms1Spectra.push_back( (*it)->parentSpectrum);
                }
                //handle first level
                progressionCount += ms1Spectra.size();
                this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(1, bbMzWidthByMsLevel[1], ms1Spectra, vector<LowResSpectrumSPtr>(), runSlices[1]);
                
                map<int, int> firstBBSpectrumIDBySpectrumID;
                this->_buildMSnBB(bbMzWidthByMsLevel, progressionCount, runSlices, firstBBSpectrumIDBySpectrumID);
                
                for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
                    this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(*it, msdata, serializer, bbFirstMs1ScanID, &firstBBSpectrumIDBySpectrumID);
                }
                m_cycles.clear();
            }

        }
        printProgBar(100);
    }

    /// Try to build the msn bounding box using the same process
    /// Will work a priori only for constant swath window
    void _buildMSnBB(map<int, double>& bbMzWidthByMsLevel, int& progressionCount, map<int, map<int, int> >& runSlices, map<int, int>& firstBBSpectrumIDBySpectrumID) {
        //first split all ms2 (using precursor mz) into good ms1 groups
        //auto ms1MzWidth = m_swathIsolationWindow;  //25.0;

        map<int, vector<HighResSpectrumSPtr> > ms2SpectraByPrecursorMzIndex;
        //map<int, double> isolationWidthByMzIndex;

        for (auto cycleIt = m_cycles.begin(); cycleIt != m_cycles.end(); ++cycleIt) {

            auto& cycle = *cycleIt;
            vector<HighResSpectrumSPtr> highResSpec;
            vector<LowResSpectrumSPtr> lowResSpec;
            cycle->getSpectra(highResSpec, lowResSpec);

            for (auto specIt = highResSpec.begin(); specIt != highResSpec.end(); ++specIt) {

                auto& spec = *specIt;
                auto precMz = spec->precursorMz();
                auto windowIndex = this->_findIsolationWindowIndexForMzPrec(precMz);
                //auto ms1MzWidth =  m_isolationWindowSizes[windowIndex];//this->_findIsolationWidthForMzPrec(precMz);
                //const int idx = precMz / ms1MzWidth;
                ms2SpectraByPrecursorMzIndex[windowIndex].push_back(spec);
                //isolationWidthByMzIndex[idx] = ms1MzWidth;
                ++progressionCount;
            }
        }

//        map<int, int> firstBBSpectrumIDBySpectrumID;

        //iterate over the mzwidth index / spectra map;
        for (auto mapIt = ms2SpectraByPrecursorMzIndex.begin(); mapIt != ms2SpectraByPrecursorMzIndex.end(); ++mapIt) {

            auto& windowIndex = mapIt->first;
            auto& ms2Spectra = mapIt->second;

            auto& sp = ms2Spectra.front();

//            printf("%d, %d, \n", sp->id, bbFirstMs2ScanID);
//            if (sp->id != bbFirstMs2ScanID)
//                    printf("GOOD to check this");

            for (auto specIt = ms2Spectra.begin(); specIt != ms2Spectra.end(); ++specIt) {
                auto& spec = *specIt;
                firstBBSpectrumIDBySpectrumID[spec->id] = sp->id;
            }


            //auto& ms1MzWidth = isolationWidthByMzIndex[idx];
            auto& minPrecMz = m_isolationWindowStarts[windowIndex];
            auto& ms1MzWidth = m_isolationWindowSizes[windowIndex];
            //double realMz = idx*ms1MzWidth;
            this->buildAndInsertData(2, bbMzWidthByMsLevel[2],
                    ms2Spectra, vector<LowResSpectrumSPtr>(),
                    runSlices[2], minPrecMz, minPrecMz + ms1MzWidth);

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
        m_isSwathIsolationWindowComputed = true;
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
        mzBBInserter(mzdbFile), m_swathIsolationWindow(0.0), m_isSwathIsolationWindowComputed(false), swathStart(400.0) {

        //m_isolationWindowStarts.push_back(this->swathStart);
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
        return boost::thread(//boost::bind(
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

//const double mzSwathConsumer::swathStart = 400.0;


} // end namespace mzdb

#endif // MZSWATHCONSUMER_HPP
