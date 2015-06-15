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
                continue;
                //break;
            }

            this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(cycleCollection, msdata, serializer);

            vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
            vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;
            cycleCollection->getSpectra(highResSpectra, lowResSpectra); //no const since will be deleted in buildAndInsertData

            progressionCount += cycleCollection->size();

            const int& msLevel = cycleCollection->msLevel;

            this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(msLevel, bbMzWidthByMsLevel[msLevel],
                                                                       highResSpectra, lowResSpectra, runSlices[msLevel]);

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
                  map<int, DataMode>& dataModeByMsLevel,
                  map<int, DataEncoding>& dataEncodingByID):
        QueueingPolicy(queue),
        mzSpectrumInserter(mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID),
        mzBBInserter(mzdbFile) {}

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
