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

#include "../lib/sqlite3/sqlite3.h"

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

    /*
     * 1/ when calling insertScans, pass a map of <spectrum_id, first_bb_spectrum_id> with it
     *    but remember that it needs a cycle object, so there must be a first pass when i split
     *    the spectra for creating BBs, and during it storing the ids in a map for this cycle
     *    So it would be a map<cycle_id, map<spectrum_id, first_bb_spectrum_id>>
     * 2/ redo the entire function to separate MS1 and MS2 spectra
     *    there must be a specific buffering for MS1 and another for MS2
     */
    
    /*
     * This function is used to compare spectra
     * Is it required to sort vectors of spectra using precursor m/Z
     * TODO check that the sort function is ordering from lower to higher mz
     */
    static bool compareHR(const HighResSpectrumSPtr &a, const HighResSpectrumSPtr &b) { return a->precursorMz() < b->precursorMz(); }
    static bool compareLR(const LowResSpectrumSPtr &a, const LowResSpectrumSPtr &b) { return a->precursorMz() < b->precursorMz(); }
    
    void _consume( pwiz::msdata::MSDataPtr msdata,
                   ISerializer::xml_string_writer& serializer,
                   map<int, double>& bbMzWidthByMsLevel,
                   map<int, double>& bbTimeWidthByMsLevel,
                   map<int, map<int, int> >& runSlices,
                   int& progressionCount,
                   int nscans ) {
        double ms1RtWidth = bbTimeWidthByMsLevel[1];
        double msnRtWidth = bbTimeWidthByMsLevel[2];
        //ms1RtWidth = 60; msnRtWidth = 75;
        int lastPercent = 0;
        this->_determineIsolationWindowStarts(msdata);
        size_t bbFirstMs1ScanID = 0;
        
        vector<HighResSpectrumSPtr> ms1Buffer;
        HighResSpectrumSPtr firstMs1Spectrum;
        vector<HighResSpectrumSPtr> msnHighResBuffer;
        HighResSpectrumSPtr firstMsnHighResSpectrum;
        map<int, HighResSpectrumSPtr> parentSpectrumPerSpectrumId;
        while( 1 ) {
            SpectraContainerUPtr cycleCollection(nullptr);
            // get the current cycle
            this->get(cycleCollection);
            // if null, exit
            if(cycleCollection == nullptr) break;
            // deal with MS1 spectra (OK !!)
            if(ms1Buffer.size() > 0 && cycleCollection->parentSpectrum->rt() - ms1Buffer[0]->rt() > ms1RtWidth) {
                // deal with this buffer and empty it
                _createAndInsertMs1(ms1Buffer, msdata, serializer, bbMzWidthByMsLevel, runSlices);
                ms1Buffer.clear();
            }
            ms1Buffer.push_back(cycleCollection->parentSpectrum);
            // if not, loop on each spectrum (high and low res !)
            vector<HighResSpectrumSPtr> highResSpec;
            vector<LowResSpectrumSPtr> lowResSpec;
            cycleCollection->getSpectra(highResSpec, lowResSpec);
            // do not consider first spectrum because it is the parentSpectrum (and already treated)
            for (auto specIt = highResSpec.begin()+1; specIt != highResSpec.end(); ++specIt) {
                auto& spectrum = *specIt;
                float thisRt = spectrum->rt();
                if(msnHighResBuffer.size() == 0) firstMsnHighResSpectrum = spectrum;
                float firstRt = firstMsnHighResSpectrum->rt();
                if(thisRt - firstRt > msnRtWidth) {
                    _splitSpectraByIsolationWindow(msnHighResBuffer, msdata, serializer, bbMzWidthByMsLevel, runSlices, parentSpectrumPerSpectrumId);
                    msnHighResBuffer.clear();
                }
                msnHighResBuffer.push_back(spectrum);
                parentSpectrumPerSpectrumId[spectrum->id] = cycleCollection->parentSpectrum;
            }
            progressionCount += highResSpec.size();
            int newPercent = (int) (((float) progressionCount / nscans * 100));
            if (newPercent == lastPercent + 2) {
                printProgBar(newPercent);
                lastPercent = newPercent;
            }
        }
        // before exiting, deal with the two buffers if they are not empty
        if(!ms1Buffer.empty()) _createAndInsertMs1(ms1Buffer, msdata, serializer, bbMzWidthByMsLevel, runSlices);
        if(!msnHighResBuffer.empty()) _splitSpectraByIsolationWindow(msnHighResBuffer, msdata, serializer, bbMzWidthByMsLevel, runSlices, parentSpectrumPerSpectrumId);
        // exit properly
        //printProgBar(100);
    }
    
    /*
     * This procedure receives MS2 spectra in the same RT window
     * It will split these spectra according to the isolation windows to produce bounding boxes
     * Creation and insertions of bounding boxes, run slices and MS2 spectra is made here
     */
    void _splitSpectraByIsolationWindow(vector<HighResSpectrumSPtr> &spectra,
                                          pwiz::msdata::MSDataPtr msdata,
                                          ISerializer::xml_string_writer& serializer,
                                          map<int, double>& bbMzWidthByMsLevel,
                                          map<int, map<int, int> >& runSlices,
                                          map<int, HighResSpectrumSPtr> &parentSpectrumPerSpectrumId) {
        // we have a vector of spectra of same MS level and in the same RT window
        // we must split these spectra into smaller vectors matching isolation windows
        map<int, vector<HighResSpectrumSPtr>> spectraByPrecursorMzIndex;
        for (auto specIt = spectra.begin(); specIt != spectra.end(); ++specIt) {
            auto spectrum = *specIt;
            auto windowIndex = this->_findIsolationWindowIndexForMzPrec(spectrum->precursorMz());
            spectraByPrecursorMzIndex[windowIndex].push_back(spectrum);
        }
        // then create BBs and store first spectrum ids
        map<int, int> firstSpectrumIdByPrecursorMzIndex;
        for (auto mapIt = spectraByPrecursorMzIndex.begin(); mapIt != spectraByPrecursorMzIndex.end(); ++mapIt) {
            auto& windowIndex = mapIt->first;
            auto& ms2Spectra = mapIt->second;
            auto minPrecMz = m_isolationWindowStarts[windowIndex];
            auto maxPrecMz = minPrecMz + m_isolationWindowSizes[windowIndex];
            this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(2, bbMzWidthByMsLevel[2], ms2Spectra, vector<LowResSpectrumSPtr>(), runSlices[2], minPrecMz, maxPrecMz);
            firstSpectrumIdByPrecursorMzIndex[windowIndex] = ms2Spectra.front()->id;
        }
        // finally store all scans
        for (auto specIt = spectra.begin(); specIt != spectra.end(); ++specIt) {
            auto spectrum = *specIt;
            auto windowIndex = this->_findIsolationWindowIndexForMzPrec(spectrum->precursorMz());
            int firstSpectrumId = firstSpectrumIdByPrecursorMzIndex[windowIndex];
            this->insertScan<h_mz_t, h_int_t>(spectrum, spectrum->cycle, firstSpectrumId, parentSpectrumPerSpectrumId[spectrum->id], msdata, serializer);
            parentSpectrumPerSpectrumId.erase(spectrum->id);
        }
    }
    
    /*
     * This procedure receives MS1 spectra in the same RT window
     * Creation and insertions of bounding boxes, run slices and MS1 spectra is made here
     */
    void _createAndInsertMs1(vector<HighResSpectrumSPtr> &spectra,
                            pwiz::msdata::MSDataPtr msdata,
                            ISerializer::xml_string_writer& serializer,
                            map<int, double>& bbMzWidthByMsLevel,
                            map<int, map<int, int> >& runSlices) {
        int bbFirstScanIDMS1 = spectra.front()->id;
        // inserting spectra
        for (auto specIt = spectra.begin(); specIt != spectra.end(); ++specIt) {
            auto spectrum = *specIt;
            this->insertScan<h_mz_t, h_int_t>(spectrum, spectrum->cycle, bbFirstScanIDMS1, spectrum, msdata, serializer);
        }
        // creating and inserting BBs
        this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(1, bbMzWidthByMsLevel[1], spectra, vector<LowResSpectrumSPtr>(), runSlices[1]);
    }

    /// For now, this is really complicated to get the good swath isolation window sizes
    void _determineIsolationWindowStarts(pwiz::msdata::MSDataPtr msdata) {
        vector<double> refTargets = PwizHelper::determineIsolationWindowStarts(msdata->run.spectrumListPtr);
        if(refTargets.size() > 0) {
            float windowStartMz, windowSize;
            for(int i = 0; i < refTargets.size(); i++) {
                windowStartMz = refTargets[i];
                m_isolationWindowStarts.push_back(windowStartMz);
                // do not update window size for the last window
                if(i < refTargets.size() - 1) windowSize = refTargets[i+1] - refTargets[i];
                m_isolationWindowSizes.push_back(windowSize);
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
                                    map<int, double>& bbTimeWidthByMsLevel,
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
                             std::ref(bbTimeWidthByMsLevel),
                             std::ref(runSlices),
                             std::ref(progressionCount),
                             nscans
                             );

    }

}; //end class

} // end namespace mzdb

#endif // MZSWATHCONSUMER_HPP
