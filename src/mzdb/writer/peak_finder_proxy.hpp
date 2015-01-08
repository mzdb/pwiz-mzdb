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

//author marc.dubois@ipbs.fr

#ifndef __PEAKFINDER__
#define __PEAKFINDER__


#include <string>
#include <stdint.h>
#include <stdio.h>
#include <exception>


#include "../utils/mzUtils.hpp"
#include "peak_finder_zero_bounded.hpp"
#include "peak_finder_wavelet.hpp"

namespace mzdb {
using namespace std;

enum PeakPickingAlgorithm{
    GENERIC = 1,
    THERMO = 2
};


//Deprecated, not used anymore
/*struct PeakFinderResults {
    int nbPeaks;
    double maxmz;
    double minmz;
    DataMode effectiveMode;
    PeakPickingAlgorithm peakPickingMode;
};*/

class PWIZ_API_DECL mzPeakFinderProxy {

private:

    //PeakFinderResults* peakFinderResults;

    template<class mz_t, class int_t>
    static void computeCentroids(const pwiz::msdata::SpectrumPtr &s,
                                 vector<std::shared_ptr<Centroid<mz_t, int_t> > >& results) {
        /*turn into centroids objects */
        vector<pwiz::msdata::MZIntensityPair> pairs;
        s->getMZIntensityPairs(pairs);

        //---following lines remove all zeros of the intensity array
        /*pairs.erase(std::remove_if(pairs.begin(), pairs.end(), [](const pwiz::msdata::MZIntensityPair& p) { return p.intensity == (int_t)0.0; }),
                    pairs.end());*/

        results.resize(pairs.size());
        float rt = static_cast<float>(s->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());
        std::transform(pairs.begin(), pairs.end(), results.begin(), [&rt, s](pwiz::msdata::MZIntensityPair& p) -> std::shared_ptr<Centroid<mz_t, int_t> > {
                mz_t mz = std::move((mz_t)p.mz);
                int_t ints = std::move((int_t)p.intensity);
                return std::make_shared<Centroid<mz_t, int_t> >(mz, ints, rt);
        });
    }

public:

    inline mzPeakFinderProxy() {}

    /*
     * Determine the encoding dataMode knowing the current dataMode deduced
     * by decoding cvparam, classically 'MS_centroid_spectum' and the wanted
     * mode.
     */
    template<class mz_t, class int_t>
    static DataMode computePeaks(const pwiz::msdata::SpectrumPtr &s,
                                 vector<std::shared_ptr<Centroid<mz_t, int_t> > >& results,
                                 DataMode wantedMode,
                                 pwiz::msdata::CVID fileType,
                                 mzPeakFinderUtils::PeakPickerParams& peakPickerParams) {

        const pwiz::msdata::CVParam& isCentroided = s->cvParam(pwiz::msdata::MS_centroid_spectrum);
        DataMode currentMode = ( isCentroided.empty() ) ? PROFILE: CENTROID;

        DataMode effectiveMode;

        if (wantedMode == PROFILE && currentMode == PROFILE) {
            effectiveMode = PROFILE;
            computeCentroids<mz_t, int_t>(s, results);

        } else if ((wantedMode == CENTROID && currentMode == PROFILE) || (wantedMode == FITTED && currentMode == PROFILE)) {//findPeak then centroidize}
            effectiveMode = wantedMode;
            findPeaks<mz_t, int_t>(s, results, fileType, peakPickerParams);

        } else {
            // current is CENTROID nothing to do
            effectiveMode = CENTROID;
            computeCentroids<mz_t, int_t>(s, results);
        }
        return effectiveMode;
    }

    /*
     *  findPeaks: will the choose the appropriate algorithm based
     *  on the detected raw file type.
     */
    template<class mz_t, class int_t>
    static void findPeaks(const pwiz::msdata::SpectrumPtr &s,
                                  vector<std::shared_ptr<Centroid<mz_t, int_t> > >& v,
                                  pwiz::msdata::CVID fileType,
                                  mzPeakFinderUtils::PeakPickerParams& peakPickerParams ) {


        switch (fileType) {
            case pwiz::msdata::MS_ABI_WIFF_format :{
                //---modify test purposes
                peakPickerParams.adaptiveBaselineAndNoise = false;
                //peakPickerParams.optimizationOpt = 0x01;
                peakPickerParams.noise = 0;
                peakPickerParams.baseline = 0;
                peakPickerParams.minSNR = 0;
                peakPickerParams.fwhm = TOF_FWHM;
                mzPeakFinderWavelet::findPeaks(s, v, peakPickerParams);
                break;
            }
            case pwiz::msdata::MS_Thermo_RAW_format : {
                //all default, ideal case
                peakPickerParams.adaptiveBaselineAndNoise = false;
                peakPickerParams.noise = 0;
                peakPickerParams.baseline = 0;
                peakPickerParams.minSNR = 0;
                mzPeakFinderZeroBounded::findPeaks<mz_t, int_t>(s, v, peakPickerParams);
                break;
            }
        default: {
             LOG(ERROR) << "Was not able to recognize original input: will launch wavelet algorithm";
             peakPickerParams.adaptiveBaselineAndNoise = true;
             peakPickerParams.optimizationOpt = 0x01;
             peakPickerParams.minSNR = 0.0;
             peakPickerParams.fwhm = TOF_FWHM;
             mzPeakFinderWavelet::findPeaks(s, v, peakPickerParams);
             break;
        }
        }
    }

    //deprecated
    //inline PeakFinderResults* getResults() {return peakFinderResults;}
};


} //end namespace
#endif
