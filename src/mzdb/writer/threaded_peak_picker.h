#ifndef MULTITHREADEDPEAKPICKER_H
#define MULTITHREADEDPEAKPICKER_H

#include "boost/thread/thread.hpp"

#include "pwiz/data/msdata/MSData.hpp"

#include "peak_finder_utils.hpp"

using namespace std;

namespace mzdb {

class mzMultiThreadedPeakPicker {

private:
    /*attribute */
    map<int, DataMode>& m_dataModeByMsLevel;


    /* methods */
    template<typename mz_t, typename int_t>
    void _peakPicksTypedSpectra(vector<std::shared_ptr<mzSpectrum<mz_t, int_t> > >& spectra,
                   DataMode m,
                   pwiz::msdata::CVID filetype,
                   mzPeakFinderUtils::PeakPickerParams& params,
                   size_t maxNbThreads) {

        size_t maxVal = 0;
        for (size_t j = 0, N = spectra.size(); j < N; j += maxNbThreads) {
            maxVal = j;
            boost::thread_group g;
            size_t counter = ( N - j < maxNbThreads ) ? N - j : maxNbThreads;
            for (size_t i = 0; i < counter; ++i) {
                g.create_thread(std::bind(&mzSpectrum<mz_t, int_t>::doPeakPicking, spectra[j + i], m, filetype, params));
            }
            g.join_all();
        }
    }


    /*
     *  launch peakPicking using a thread group
     *
     */
    template<class h_mz_t, class h_int_t,
                   class l_mz_t, class l_int_t>
    void _peakPicks(vector<std::shared_ptr<mzSpectrum<h_mz_t,h_int_t> > >& highResBuffer,
                            vector<std::shared_ptr<mzSpectrum<l_mz_t,l_int_t> > >& lowResBuffer,
                            DataMode m,
                            pwiz::msdata::CVID filetype,
                            mzPeakFinderUtils::PeakPickerParams& params) {

        size_t nbProc = boost::thread::hardware_concurrency();
        size_t maxNbThreads = std::max<size_t>(1, nbProc);//(nbProc - 4) / 2 + 1);

        this->_peakPicksTypedSpectra<h_mz_t, h_int_t>(highResBuffer, m, filetype, params, maxNbThreads);
        this->_peakPicksTypedSpectra<l_mz_t, l_int_t>(lowResBuffer, m, filetype, params, maxNbThreads);
//#endif
    }

public:
    /* constructor */
    inline mzMultiThreadedPeakPicker (map<int, DataMode>& dataModeByMsLevel): m_dataModeByMsLevel(dataModeByMsLevel) {}


    inline map<int, DataMode>& getDataModeByMsLevel() {
        return this->m_dataModeByMsLevel;
    }


    /**
     * wrapper of the function above
     */
    template<class h_mz_t, class h_int_t,
                 class l_mz_t, class l_int_t>
    inline void start(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject,
                      pwiz::msdata::CVID filetype,
                      mzPeakFinderUtils::PeakPickerParams& params) {

        vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
        vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;

        cycleObject->getSpectra(highResSpectra, lowResSpectra);
        auto& dataMode = m_dataModeByMsLevel[cycleObject->msLevel];
        this->_peakPicks<h_mz_t, h_int_t, l_mz_t, l_int_t>(highResSpectra, lowResSpectra, dataMode, filetype, params);
    }

};

} // end namespace mzdb

#endif // MULTITHREADEDPEAKPICKER_H
