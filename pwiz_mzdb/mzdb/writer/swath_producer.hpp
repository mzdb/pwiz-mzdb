#ifndef MZSWATHPRODUCER_H
#define MZSWATHPRODUCER_H

#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz/utility/misc/IntegerSet.hpp"
//#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"

#include "../threading/SpScQueue.hpp"
#include "cycle_obj.hpp"
#include "spectrumlist_policy.h"

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

    /**
     * @brief _checkCycleNumber
     * checks if cycle number matches the "real" cycle number given in the spectrum title
     *
     * @param filetype origin file type (titles are differents for each file type)
     * @param spectrumTitle in which the good cycle number may be found
     * @param cycleNumber, the computed cycle number that may be wrong
     */
    void _checkCycleNumber(pwiz::msdata::CVID filetype, string spectrumTitle, int& cycleNumber) {
        // add a special case for ABSciex files
        if(filetype == pwiz::msdata::MS_ABI_WIFF_format) {
            std::regex e ("cycle=(\\d+)");
            std::smatch match;
            if (std::regex_search(spectrumTitle, match, e) && match.size() > 0) {
                // real cycle extracted from title
                // if the spray gets lost, the spectra will not be written in the wiff file and the cycles wont be a list from 1 to the end
                int cycle = std::stoi(match.str(1));
                if(cycle != cycleNumber) {
                    LOG(INFO) << "ABU cycle changed from " << cycleNumber << " to " << cycle << "\n";
                    cycleNumber = cycle;
                }
            }
        }
    }

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
        cycle->parentSpectrum->doPeakPicking(this->m_peakPicker.getDataModeByMsLevel()[1], filetype, params);

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
            pair<int, int>& nscans,
            pwiz::msdata::CVID filetype,
            mzPeakFinderUtils::PeakPickerParams& params) {

        /* initialiazing counter */
        int cycleCount = 0, scanCount = 1;

        HighResSpectrumSPtr currMs1(nullptr);
        SpectraContainerUPtr cycle(nullptr);

        for (size_t i = nscans.first; i < nscans.second; ++i) {

            pwiz::msdata::SpectrumPtr spectrum;
            spectrum =  mzdb::getSpectrum<SpectrumListType>(spectrumList, i, true, levelsToCentroid);
            const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
            bool isInHighRes = this->isInHighRes(spectrum);

            if (msLevel == 1 ) {
                ++cycleCount;
                this->_checkCycleNumber(filetype, spectrum->id, cycleCount);
                currMs1 = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spectrum);
                if (cycle) {
                    //peak picks only ms == 2
                    this->_peakPickAndPush(cycle, filetype, params);
                }
                cycle = std::move(SpectraContainerUPtr(new SpectraContainer(2)));
                cycle->parentSpectrum = currMs1;
                ++scanCount;
            }
            if (msLevel > 1) {
                if (isInHighRes) {
                    auto s = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spectrum);
                    s->isInHighRes = isInHighRes;
                    cycle->addHighResSpectrum(s);
                } else {
                    LOG(WARNING) << "LOW RES SPEC IS NOT NORMAL IN SWATH MODE \n";
                    auto s = std::make_shared<mzSpectrum<l_mz_t, l_int_t> >(scanCount, cycleCount, spectrum);
                    s->isInHighRes = isInHighRes;
                    cycle->addLowResSpectrum(s);
                }
                ++scanCount;
            }

        } // end for

        //ensure everyone is sent to the queue
        if ( cycle && ! cycle->empty() ) {
            this->_peakPickAndPush(cycle, filetype, params);
        }

        //signify that we finished producing
        //LOG(INFO) << "Producer finished...\n";
        this->put(std::move(SpectraContainerUPtr(nullptr)));
    }

public:
    mzSwathProducer(typename QueueingPolicy::QueueType& queue,
                    std::string& pathway,
                    map<int, DataMode>& dataModeByMsLevel):
        QueueingPolicy(queue),
        MetadataExtractionPolicy(pathway),
        m_peakPicker(dataModeByMsLevel){

    }

    boost::thread getProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                    SpectrumListType* spectrumList,
                                    pair<int, int>& nscans,
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
                    nscans,
                    filetype,
                    std::ref(params)
                    //)
                    );

    } // end function

};


} // end namespace mzdb

#endif // MZSWATHPRODUCER_H
