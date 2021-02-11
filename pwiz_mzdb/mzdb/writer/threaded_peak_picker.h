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
 * @file threaded_peak_picker.h
 * @brief Performs peak-picking of several spectra using several threads.
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MULTITHREADEDPEAKPICKER_H
#define MULTITHREADEDPEAKPICKER_H

#include "boost/thread/thread.hpp"

#include "pwiz/data/msdata/MSData.hpp"

#include "peak_finder_utils.hpp"

using namespace std;

namespace mzdb {

/**
 * The mzMultiThreadedPeakPicker class
 * ====================================
 *
 * performs peak-picking of several spectra using several threads.
 */
class mzMultiThreadedPeakPicker {

private:

    /// wanted DataMode for each msLevel wanted by the user.
    map<int, DataMode>& m_dataModeByMsLevel;
    
    //bool m_safeMode;
    map<int, double> m_resolutions;

    /**
     * Launches peak picking for each spectrum in its own thread.
     *
     * This is a little aggressive.
     */
    template<typename mz_t, typename int_t>
    void _peakPicksTypedSpectra(vector<std::shared_ptr<mzSpectrum<mz_t, int_t> > >& spectra,
                                //DataMode m,
                                pwiz::msdata::CVID filetype,
                                mzPeakFinderUtils::PeakPickerParams& params,
                                size_t maxNbThreads) {

        for (size_t j = 0, N = spectra.size(); j < N; j += maxNbThreads) {
            boost::thread_group g;
            size_t counter = ( N - j < maxNbThreads ) ? N - j : maxNbThreads;
            for (size_t i = 0; i < counter; ++i) {
                g.create_thread(std::bind(&mzSpectrum<mz_t, int_t>::doPeakPicking, spectra.at(j + i), filetype, params, m_resolutions));
            }
            g.join_all();
        }
    }


    /**
     * wrapper: launches peak picking on several spectra
     */
    template<class h_mz_t, class h_int_t,
             class l_mz_t, class l_int_t>
    void _peakPicks(vector<std::shared_ptr<mzSpectrum<h_mz_t,h_int_t> > >& highResBuffer,
                    vector<std::shared_ptr<mzSpectrum<l_mz_t,l_int_t> > >& lowResBuffer,
                    //DataMode m,
                    pwiz::msdata::CVID filetype,
                    mzPeakFinderUtils::PeakPickerParams& params) {

        size_t nbProc = boost::thread::hardware_concurrency();
        //---heard that in theory should be around (nbProc - 4) / 2 + 1);
        size_t maxNbThreads = std::max<size_t>(1, 1); // JPM.TODO JPM.TEST : (1, nbProc); 

        this->_peakPicksTypedSpectra<h_mz_t, h_int_t>(highResBuffer, /*m,*/ filetype, params, maxNbThreads);
        this->_peakPicksTypedSpectra<l_mz_t, l_int_t>(lowResBuffer, /*m,*/ filetype, params, maxNbThreads);
    }

public:
    /* constructor */
    inline mzMultiThreadedPeakPicker (map<int, DataMode>& dataModeByMsLevel,
                                      map<int, double> resolutions):
        m_dataModeByMsLevel(dataModeByMsLevel),
        m_resolutions(resolutions) {}


    inline map<int, DataMode>& getDataModeByMsLevel() {
        return this->m_dataModeByMsLevel;
    }


    /**
     * wrapper one liner for launching peak picking on a cycle object
     */
    template<class h_mz_t, class h_int_t,
             class l_mz_t, class l_int_t>
    inline void start(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject,
                      pwiz::msdata::CVID filetype,
                      mzPeakFinderUtils::PeakPickerParams& params) {
        
        vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > highResSpectra;
        vector<std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > > lowResSpectra;

        cycleObject->getSpectra(highResSpectra, lowResSpectra);
        //auto& dataMode = m_dataModeByMsLevel[cycleObject->msLevel];
        this->_peakPicks<h_mz_t, h_int_t, l_mz_t, l_int_t>(highResSpectra, lowResSpectra, /*dataMode,*/ filetype, params);
    }

};

} // end namespace mzdb

#endif // MULTITHREADEDPEAKPICKER_H
