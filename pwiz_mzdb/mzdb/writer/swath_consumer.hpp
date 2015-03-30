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

    /// TODO get the value from MetadataExtractor
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
                   map<int, map<int, int> >& runSlices,
                   int& progressionCount,
                   int nscans ) {

        int lastPercent = 0;
        bool toBeBroken = false;

        while ( 1 ) {

            if (toBeBroken)
                break;

            while (m_cycles.size() <  4) {
                SpectraContainerUPtr cycleCollection(nullptr);

                this->get(cycleCollection);
                if ( cycleCollection == nullptr ) {
                    LOG(INFO) << "Inserter consumer finished du to null pointer\n";
                    toBeBroken = true;
                    break;
                }
                if (! m_isSwathIsolationWindowComputed) {
                    this->_determineIsolationWindowStarts(cycleCollection);
                }

                m_cycles.push_back(std::move(cycleCollection));

            }
            vector<HighResSpectrumSPtr> ms1Spectra;

            int bbFirstMs1ScanID = m_cycles.front()->parentSpectrum->id;

            //insert scans using mzSpectrumInserter, insert cycle consecutively
            for (auto it = m_cycles.begin(); it != m_cycles.end(); ++it ) {
                ms1Spectra.push_back( (*it)->parentSpectrum);
                this->insertScans<h_mz_t, h_int_t, l_mz_t, l_int_t>(*it, msdata, serializer, bbFirstMs1ScanID );
            }

            //handle first level
            progressionCount += ms1Spectra.size();
            this->buildAndInsertData<h_mz_t, h_int_t, l_mz_t, l_int_t>(1, //msLevel
                                                                       bbMzWidthByMsLevel[1],
                    ms1Spectra, //highresspectra
                    vector<LowResSpectrumSPtr>(), //empty lowres spectra
                    runSlices[1]);

            this->_buildMSnBB(bbMzWidthByMsLevel, progressionCount, runSlices);

            m_cycles.clear();


            // progression update computing the new progression
            int newPercent = (int) (((float) progressionCount / nscans * 100));
            if (newPercent == lastPercent + 2) {
                printProgBar(newPercent);
                lastPercent = newPercent;
            }

            if (progressionCount >= nscans ) {
                LOG(INFO) << "Inserter consumer finished: reaches final progression";
                break;
            }

        }
        printProgBar(100);
    }

    /// Try to build the msn bounding box using the same process
    /// Will work a priori only for constant swath window
    void _buildMSnBB(map<int, double>& bbMzWidthByMsLevel, int& progressionCount, map<int, map<int, int> >& runSlices) {
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
        //iterate over the mzwidth index / spectra map;
        for (auto mapIt = ms2SpectraByPrecursorMzIndex.begin(); mapIt != ms2SpectraByPrecursorMzIndex.end(); ++mapIt) {

            auto& windowIndex = mapIt->first;
            auto& ms2Spectra = mapIt->second;
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
    void _determineIsolationWindowStarts(SpectraContainerUPtr& cycle) {
        vector<HighResSpectrumSPtr> highResSpec;
        vector<LowResSpectrumSPtr> lowResSpec;
        cycle->getSpectra(highResSpec, lowResSpec);

        // fails if we will miss spectra in all swath spectra
        //assert(lowResSpec.empty());
        //assert(! higResSpec.empty());

        //assume the precurors mz are ordered
        // using the reporting method
        auto lastMz = swathStart;
        auto cc = 2;
        for (auto& it = highResSpec.begin(); it != highResSpec.end(); ++it) {
            auto windowCenter = (*it)->precursorMz();
            auto diffWithPrev = windowCenter - lastMz;
            auto newMz = windowCenter + diffWithPrev;
            m_isolationWindowStarts.push_back(newMz - 1);
            m_isolationWindowSizes.push_back(newMz - lastMz);
            LOG(INFO) << "Swath Window #" << cc <<", Start: " << newMz - 1 << ",  Size: " <<  newMz - lastMz;
            lastMz = newMz - 1;
            ++cc;
        }
        m_isSwathIsolationWindowComputed = true;

        //fastest solution some
        //m_swathIsolationWindow = (highResSpec.front()->precursorMz() - mzSwathConsumer.swathStart) * 2;
    }

    ///
    int _findIsolationWindowIndexForMzPrec(double mz) {
        auto beginIt = m_isolationWindowStarts.begin();
        auto it = std::lower_bound(beginIt, m_isolationWindowStarts.end(), mz);
        auto idx = std::distance(beginIt, it);

        if (! idx)
            LOG(ERROR) << "mz < to the lowest bound value:" << swathStart << "mz";

        //  decrease once the index
        --idx;

        return idx;


    }

public:

    mzSwathConsumer( typename QueueingPolicy::QueueType& queue,
                     MzDBFile& mzdbFile,
                     mzParamsCollecter& paramsCollecter,
                     pwiz::msdata::CVID rawFileFormat,
                     map<int, DataMode>& dataModeByMsLevel):
        QueueingPolicy(queue),
        mzSpectrumInserter(mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
        mzBBInserter(mzdbFile), m_swathIsolationWindow(0.0), m_isSwathIsolationWindowComputed(false), swathStart(400.0) {

        m_isolationWindowStarts.push_back(this->swathStart);
    }

    /*void startAndJoin(
            pwiz::msdata::MSDataPtr msdata,
            ISerializer::xml_string_writer& serializer,
            map<int, double>& bbMzWidthByMsLevel,
            map<int, map<int, int> >& runSlices,
            int& progressionCount,
            int nscans ) {
        auto consumer = boost::thread(boost::bind(&mzSwathConsumer<
                                                  QueueingPolicy,
                                                  SpectrumListType>::_consume,
                                                  this,
                                                  std::ref(msdata),
                                                  std::ref(serializer),
                                                  std::ref(bbMzWidthByMsLevel),
                                                  std::ref(runSlices),
                                                  std::ref(progressionCount),
                                                  nscans));
        consumer.join();


    }*/

    boost::thread getConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                    ISerializer::xml_string_writer& serializer,
                                    map<int, double>& bbMzWidthByMsLevel,
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
                             std::ref(runSlices),
                             std::ref(progressionCount),
                             nscans
                             );

    }

}; //end class

//const double mzSwathConsumer::swathStart = 400.0;


} // end namespace mzdb

#endif // MZSWATHCONSUMER_HPP
