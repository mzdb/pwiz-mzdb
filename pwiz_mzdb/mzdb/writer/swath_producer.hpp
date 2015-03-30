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

    void _peakPickAndPush(SpectraContainerUPtr& cycle, pwiz::msdata::CVID filetype,
                          mzPeakFinderUtils::PeakPickerParams& params) {
        this->m_peakPicker.start<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycle, filetype, params);

        //do not forget to fit first msLevel
        cycle->parentSpectrum->doPeakPicking(this->m_peakPicker.getDataModeByMsLevel()[1], filetype, params);

        this->put(std::move(cycle));
    }


    void _produce(
            pwiz::util::IntegerSet& levelsToCentroid,
            SpectrumListType* spectrumList,
            int nscans,
            pwiz::msdata::CVID filetype,
            mzPeakFinderUtils::PeakPickerParams& params) {

        /* initialiazing counter */
        int cycleCount = 0, scanCount = 1;

        HighResSpectrumSPtr currMs1(nullptr);
        SpectraContainerUPtr cycle; //(nullptr);

        for (size_t i = 0; i < nscans; ++i) {

            pwiz::msdata::SpectrumPtr spectrum =  mzdb::getSpectrum<SpectrumListType>(spectrumList, i, true, levelsToCentroid);
            const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
            bool isInHighRes = this->isInHighRes(spectrum);

            if (msLevel == 1 ) {
                ++cycleCount;
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
        LOG(INFO) << "Producer finished...\n";
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

    /** launch the thread reading the spectrumList and fill the queue */
    /*void startAndJoin(
            pwiz::util::IntegerSet& levelsToCentroid,
            SpectrumListType* spectrumList,
            int nscans,
            pwiz::msdata::CVID filetype,
            mzPeakFinderUtils::PeakPickerParams& params) {

        auto producer = boost::thread(
                    boost::bind(
                        &mzSwathProducer<//h_mz_t, h_int_t, //high resolution data points encoding
                        //l_mz_t, l_int_t, //low resolution data points encoding
                        QueueingPolicy,
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
                        )
                    );
        producer.join();
    }*/

    boost::thread getProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                    SpectrumListType* spectrumList,
                                    int nscans,
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
