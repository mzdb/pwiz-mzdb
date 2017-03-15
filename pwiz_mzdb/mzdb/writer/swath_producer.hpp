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
 * @file swath_producer.hpp
 * @brief Read spectra from file, run peakpicking algorithm and push the spectra in the cycle objects
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZSWATHPRODUCER_H
#define MZSWATHPRODUCER_H

#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz/utility/misc/IntegerSet.hpp"
//#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"

#include "../threading/SpScQueue.hpp"
#include "cycle_obj.hpp"
#include "spectrumlist_policy.h"

#include "../../utils/mzUtils.hpp"

using namespace std;

namespace mzdb {

/**
 * Same function than mzDDAProcuder but adapted for SWATH/DIA
 * @see mzDDAProducer
 */
template<
        class QueueingPolicy,
        class MetadataExtractionPolicy, // TODO: create a policy which claims isInHighRes always true
        class PeakPickerPolicy, // ability to launch peak picking process
        class SpectrumListType> // Wiff or raw proteowizard object

class mzSwathProducer: QueueingPolicy, MetadataExtractionPolicy {


    /* declare useful typedefs */
    typedef  typename QueueingPolicy::Obj SpectraContainer;
    typedef  typename QueueingPolicy::UPtr SpectraContainerUPtr;

    typedef typename SpectraContainer::h_mz_t h_mz_t;
    typedef typename SpectraContainer::h_int_t h_int_t;
    typedef typename SpectraContainer::l_mz_t l_mz_t;
    typedef typename SpectraContainer::l_int_t l_int_t;

    typedef  std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > HighResSpectrumSPtr;


private:

    PeakPickerPolicy m_peakPicker;
    bool firstCycleMs1IsMissing;
    bool m_safeMode;
    
    /// DataMode for each MS level
    map<int, DataMode> m_dataModeByMsLevel;
    
    /// true if user specifically asked to store data with double variables
    bool isNoLoss;

    /**
     * @brief _peakPickAndPush
     * performs peak-picking then push it to the queue
     *
     * @param cycle, cycle number
     * @param filetype origin file type
     * @param params for peak picking algorithm
     */
    void _peakPickAndPush(SpectraContainerUPtr& cycle, pwiz::msdata::CVID filetype,
                          mzPeakFinderUtils::PeakPickerParams& params) {
        this->m_peakPicker.start<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycle, filetype, params);

        //do not forget to fit first msLevel
        //cycle->parentSpectrum->doPeakPicking(this->m_peakPicker.getDataModeByMsLevel()[1], filetype, params);

        if(!firstCycleMs1IsMissing) {
            // TODO check why dataModeByMsLevel[1] is used ???
            cycle->parentSpectrum->doPeakPicking(this->m_peakPicker.getDataModeByMsLevel()[0], filetype, params, m_safeMode);
        } else {
            firstCycleMs1IsMissing = false;
        }

        this->put(std::move(cycle));
    }


    /**
     * Read all spectra from data file, performs peak-picking and push cycle objects
     * into the queue
     *
     * @param levelsToCentroid
     * @param spectrumList
     * @param nscans
     * @param filetype
     * @param params
     */
    void _produce(
            pwiz::util::IntegerSet& levelsToCentroid,
            SpectrumListType* spectrumList,
            pair<int, int>& cycleRange,
            pwiz::msdata::CVID filetype,
            mzPeakFinderUtils::PeakPickerParams& params) {

        /* initialiazing counter */
        int cycleCount = 0, scanCount = 1;
        firstCycleMs1IsMissing = false;

        HighResSpectrumSPtr currMs1(nullptr);
        SpectraContainerUPtr cycle(nullptr);

        for (size_t i = 0; i < spectrumList->size(); i++) {
            try {
                pwiz::msdata::SpectrumPtr spectrum;
                pwiz::msdata::SpectrumPtr centroidSpectrum;
                
                // Retrieve the input spectrum as is
                spectrum = spectrumList->spectrum(i, true);
                
                // Retrieve the MS level
                const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
                bool isInHighRes = this->isInHighRes(spectrum, isNoLoss);
                // Retrieve the effective mode
                DataMode wantedMode = m_dataModeByMsLevel[msLevel];
                
                // If effective mode is not profile
                if (wantedMode != PROFILE) {
                    // --- Retrieve the CENTROID spectrum ---
                    // The content of the retrieved spectrum depends on the levelsToCentroid settings
                    // If the set is empty then we will get a PROFILE spectrum
                    // Else we will obtain a CENTROID spectrum if its msLevel belongs to levelsToCentroid
                    centroidSpectrum = mzdb::getSpectrum<SpectrumListType>(spectrumList, i, true, levelsToCentroid);
                }
                
                // increment cycle number
                if (msLevel == 1 ) {
                    PwizHelper::checkCycleNumber(filetype, spectrum->id, ++cycleCount);
                }
                // do not process if the cycle is not asked
                if(cycleCount < cycleRange.first) {
                    continue;
                } else if(cycleRange.second != 0 && cycleCount > cycleRange.second) {
                    break;
                }

                if (msLevel == 1 ) {
                    currMs1 = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spectrum, centroidSpectrum);
                    if (cycle) {
                        //peak picks only ms == 2
                        this->_peakPickAndPush(cycle, filetype, params);
                    }
                    // it may happen that the first MS1 is missing, in such case we shouldn't increment cycle number
                    if(firstCycleMs1IsMissing) {
                        firstCycleMs1IsMissing = false;
                        LOG(WARNING) << "First cycle has been skipped because MS1 spectrum was missing";
                    }
                    cycle = std::move(SpectraContainerUPtr(new SpectraContainer(2)));
                    cycle->parentSpectrum = currMs1;
                    ++scanCount;
                } else {
                    if (!cycle) {
                        LOG(WARNING) << "Spectrum \"" << spectrum->id << "\" has no precursor, creating a fake one";
                        firstCycleMs1IsMissing = true;
                        ++cycleCount;
                        PwizHelper::checkCycleNumber(filetype, spectrum->id, cycleCount);
                        currMs1 = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >();
                        currMs1->id = scanCount;
                        currMs1->cycle = cycleCount;
                        cycle = std::move(SpectraContainerUPtr(new SpectraContainer(2)));
                        cycle->parentSpectrum = currMs1;
                        ++scanCount;
                    }
                    if (isInHighRes) {
                        auto s = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spectrum, centroidSpectrum);
                        s->isInHighRes = isInHighRes;
                        cycle->addHighResSpectrum(s);
                    } else {
                        LOG(WARNING) << "LOW RES SPEC IS NOT NORMAL IN SWATH MODE";
                        auto s = std::make_shared<mzSpectrum<l_mz_t, l_int_t> >(scanCount, cycleCount, spectrum, centroidSpectrum);
                        s->isInHighRes = isInHighRes;
                        cycle->addLowResSpectrum(s);
                    }
                    ++scanCount;
                }
                // delete spectra objects
                spectrum.reset();
                centroidSpectrum.reset();
            } catch(std::exception& e) {
                LOG(INFO) << "Exception detected: " << e.what();
            }

        } // end for

        //ensure everyone is sent to the queue
        if ( cycle && ! cycle->empty()) {
            this->_peakPickAndPush(cycle, filetype, params);
        }

        //signify that we finished producing
        LOG(INFO) << "Producer finishing...\n";
        this->put(std::move(SpectraContainerUPtr(nullptr)));
        //LOG(INFO) << "Producer finished...\n";
    }

public:
    mzSwathProducer(typename QueueingPolicy::QueueType& queue,
                    MzDBFile& mzdbFile,
                    map<int, DataMode>& dataModeByMsLevel,
                    bool safeMode):
        QueueingPolicy(queue),
        MetadataExtractionPolicy(mzdbFile.name),
        isNoLoss(mzdbFile.isNoLoss()),
        m_peakPicker(dataModeByMsLevel, safeMode),
        m_safeMode(safeMode),
        m_dataModeByMsLevel(dataModeByMsLevel) {

    }

    boost::thread getProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                    SpectrumListType* spectrumList,
                                    pair<int, int>& cycleRange,
                                    pwiz::msdata::CVID filetype,
                                    mzPeakFinderUtils::PeakPickerParams& params) {

        return boost::thread(
                    //boost::bind(
                    &mzSwathProducer<QueueingPolicy,
                    MetadataExtractionPolicy, // TODO: create a policy which claims isInHighRes always true
                    PeakPickerPolicy, // ability to launch peak picking process
                    SpectrumListType>::_produce,
                    this,
                    //arguments
                    std::ref(levelsToCentroid),
                    spectrumList,
                    cycleRange,
                    filetype,
                    std::ref(params)
                    //)
                    );

    } // end function

};


} // end namespace mzdb

#endif // MZSWATHPRODUCER_H
