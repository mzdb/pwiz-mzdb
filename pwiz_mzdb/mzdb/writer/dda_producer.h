#ifndef MZDDAPRODUCER_H
#define MZDDAPRODUCER_H

#include <unordered_map>
#include <set>

#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz/utility/misc/IntegerSet.hpp"

#include "cycle_obj.hpp"
#include "spectrum_inserter.h"
#include "bb_inserter.hpp"
#include "spectrumlist_policy.h"

using namespace std;

namespace mzdb {

template< class QueueingPolicy,
          class MetadataExtractionPolicy,
          class PeakPickerPolicy,            // ability to launch peak picking process
          class SpectrumListType>          // Wiff or raw proteowizard object
class mzDDAProducer:  QueueingPolicy, MetadataExtractionPolicy {


    //several useful typedefs
    typedef typename QueueingPolicy::Obj SpectraContainer;//typedef mzSpectraContainer<> SpectraContainer;//typedef mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> SpectraContainer;
    typedef typename QueueingPolicy::UPtr SpectraContainerUPtr; //typedef unique_ptr<SpectraContainer> SpectraContainerUPtr;

    typedef typename SpectraContainer::h_mz_t h_mz_t;
    typedef typename SpectraContainer::h_int_t h_int_t;
    typedef typename SpectraContainer::l_mz_t l_mz_t;
    typedef typename SpectraContainer::l_int_t l_int_t;

    typedef std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > HighResSpectrumSPtr;

private:

    PeakPickerPolicy m_peakPicker;
    set<int> m_msLevels;

    void _peakPickAndPush(SpectraContainerUPtr& cycle, pwiz::msdata::CVID filetype,
                          mzPeakFinderUtils::PeakPickerParams& params) {

        this->m_peakPicker.start<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycle, filetype, params);
        this->put(std::move(cycle));
    }

    void _addSpectrum(SpectraContainerUPtr& cycle, int scanCount, int cycleCount, bool isInHighRes, pwiz::msdata::SpectrumPtr spec) {
        if (isInHighRes) {
            auto s = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spec);
            s->isInHighRes = isInHighRes;
            cycle->addHighResSpectrum(s);
        } else {
            auto s = std::make_shared<mzSpectrum<l_mz_t, l_int_t> >(scanCount, cycleCount, spec);
            s->isInHighRes = isInHighRes;
            printf("%s\n", s->spectrum->id.c_str());
            cycle->addLowResSpectrum(s);
        }
    }


public:
    void _produce(  pwiz::util::IntegerSet& levelsToCentroid,
                    SpectrumListType* spectrumList,
                    int nscans,
                    map<int, double>& bbRtWidth,
                    pwiz::msdata::CVID filetype,
                    mzPeakFinderUtils::PeakPickerParams& params) {

        /* initializing counter */
        int cycleCount = 0, scanCount = 1;

        unordered_map<int, SpectraContainerUPtr> spectraByMsLevel;
        HighResSpectrumSPtr currMs1(nullptr);

        for (size_t i = 0; i < nscans; ++i) {

            pwiz::msdata::SpectrumPtr spectrum;
            try {
                spectrum =  mzdb::getSpectrum<SpectrumListType>(spectrumList, i, true, levelsToCentroid);
            }catch (exception &e) {
                printf("Catch an exception!\n");
                printf("%s\n", e.what());
            }

            const int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
            m_msLevels.insert(msLevel);
            const float rt = PwizHelper::rtOf(spectrum);

            //method comes from MetadataExtractionPolicy inheritage
            bool isInHighRes = this->isInHighRes(spectrum);

            if (msLevel == 1 ) {
                ++cycleCount;
                currMs1 = std::make_shared<mzSpectrum<h_mz_t, h_int_t> >(scanCount, cycleCount, spectrum);
            }

            //init
            if (spectraByMsLevel.find(msLevel) == spectraByMsLevel.end()) {
                SpectraContainerUPtr spectraCollection(new SpectraContainer(msLevel));
                spectraCollection->parentSpectrum = currMs1;
                spectraByMsLevel[msLevel] = std::move(spectraCollection);
            }

            //get a reference to the unique pointer corresponding to the current mslevel
            auto& container = spectraByMsLevel[msLevel];


            //check if the bounding box is well sized. If yes, launch peak-picking and add it to the queue
            auto& bbRtWidthBound =  bbRtWidth[msLevel];

            bool added = false;

            if ( container->empty() ) {
                this->_addSpectrum(container, scanCount, cycleCount, isInHighRes, spectrum);
                ++scanCount;
                added = true;
            }

            if ( (rt - container->getBeginRt()) >= bbRtWidthBound)  {

                this->_peakPickAndPush(container, filetype, params);

                this->put(std::move(container));

                SpectraContainerUPtr c(new SpectraContainer(msLevel));

                c->parentSpectrum = currMs1;
                if (!added) {
                    this->_addSpectrum(c, scanCount, cycleCount, isInHighRes, spectrum);
                    ++scanCount;
                }
                spectraByMsLevel[msLevel] = std::move(c);
            } else {
                if (!added) {
                    this->_addSpectrum(container, scanCount, cycleCount, isInHighRes, spectrum);
                    ++scanCount;
                }
            }

        } // end for

        //handles ending, i.e non finished cycles
        for(int i = 1; i <= spectraByMsLevel.size(); ++i) {
            // FIX: when there are only MS2 spectra
            if (spectraByMsLevel.find(i) != spectraByMsLevel.end()) {
                auto& container = spectraByMsLevel[i];
                while (container && ! container->empty()) {
                    this->_peakPickAndPush(container, filetype, params);
                }
            }
        }

        //just logging if we did not found any spectrea with mslvl = 1
        if (m_msLevels.find(1) == m_msLevels.end()) {
            LOG(WARNING) << "Did not see any msLevel 1 !";
        }

        //signify that we finished producing
        SpectraContainerUPtr nullContainer(nullptr);
        this->put(nullContainer);
        LOG(INFO) << "Producer finished...\n";
    }

    /// constructor
    mzDDAProducer( typename QueueingPolicy::QueueType& queue,
                   std::string& mzdbPath,
                   map<int, DataMode>& dataModeByMsLevel):
        QueueingPolicy(queue),
        MetadataExtractionPolicy(mzdbPath),
        m_peakPicker(dataModeByMsLevel) {}

    /** launch the thread reading the spectrumList and fill the queue */
    /*void startAndJoin( pwiz::util::IntegerSet& levelsToCentroid,
                       SpectrumListType* spectrumList,
                       int nscans,
                       map<int, double>& bbWidthManager,
                       pwiz::msdata::CVID filetype,
                       mzPeakFinderUtils::PeakPickerParams& params) {

        auto producer = boost::thread(boost::bind(&mzDDAProducer<
                                                  QueueingPolicy,
                                                  MetadataExtractionPolicy, // TODO: create a policy which claims isInHighRes always true
                                                  PeakPickerPolicy, // ability to launch peak picking process
                                                  SpectrumListType>::_produce,
                                                  this,
                                                  std::ref(levelsToCentroid),
                                                  spectrumList,
                                                  nscans,
                                                  std::ref(bbWidthManager),
                                                  filetype,
                                                  std::ref(params)));
        producer.join();
    }*/


    boost::thread getProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                    SpectrumListType* spectrumList,
                                    int nscans,
                                    map<int, double>& bbWidthManager,
                                    pwiz::msdata::CVID filetype,
                                    mzPeakFinderUtils::PeakPickerParams& params) {

        return boost::thread(//boost::bind(
                             &mzDDAProducer<QueueingPolicy,
                             MetadataExtractionPolicy, // TODO: create a policy which claims isInHighRes always true
                             PeakPickerPolicy, // ability to launch peak picking process
                             SpectrumListType>::_produce,
                             this,
                             std::ref(levelsToCentroid),
                             spectrumList,
                             nscans,
                             std::ref(bbWidthManager),
                             filetype,
                             std::ref(params)
                             );
    } // end function

};


}
#endif // MZDDACONSUMER_H
