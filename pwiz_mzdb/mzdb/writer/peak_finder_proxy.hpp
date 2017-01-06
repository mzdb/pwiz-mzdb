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
 * @file peak_finder_proxy.hpp
 * @brief Class that manage the launch of the good algorithm for peak picking each spectrum.
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef __PEAKFINDER__
#define __PEAKFINDER__


#include <string>
#include <stdint.h>
#include <stdio.h>
#include <exception>


#include "../utils/mzUtils.hpp"
#include "peak_finder_zero_bounded.hpp"
#include "peak_finder_tof.hpp"
#include "peak_finder_wavelet.hpp"

namespace mzdb {
using namespace std;

/**
 * The mzPeakFinderProxy class
 * ============================
 *
 * Class that manage the launch of the good algorithm for peak picking each spectrum.
 * `Effective mode` are distinguished than `wanted mode` (will be DEPRECATED). For some
 * reasons it will be easier to allow centroid to fit conversion. The resulting HWHMs
 * will be null.
 */
class PWIZ_API_DECL mzPeakFinderProxy {

private:
    
    template<class mz_t, class int_t> inline static bool detectPeaksAgain(const pwiz::msdata::SpectrumPtr &spectrum, vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids) {
        // conditions to force re-peakpeacking are:
        // original data must be PROFILE (therefore centroidable)
        if(spectrum->hasCVParam(pwiz::msdata::MS_profile_spectrum)) {
            // original peakpicking has failed for some reason to produce any centroids at all
            if(centroids.size() == 0)
                return true;
            // original peakpicking fails when there are no zero intensities, in this case all peaks are centroids
            if(spectrum->getIntensityArray()->data.size() == centroids.size())
                return true;
        }
        return false;
    }
    
    // full width at half-maximum should only be calculated for fitted mode
    inline static bool computeFullWidthAtHalfMaximum(DataMode wantedMode) {
        if(wantedMode == FITTED)
            return true;
        return false;
    }

public:

    inline mzPeakFinderProxy() {}
    
    template<class mz_t, class int_t> static void computeCentroidsWidths(const pwiz::msdata::SpectrumPtr &spectrum,
                                                                         vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                                                                         pwiz::msdata::CVID fileType,
                                                                         mzPeakFinderUtils::PeakPickerParams& peakPickerParams,
                                                                         DataMode wantedMode) {
        if (!spectrum || spectrum == nullptr) {
            // do nothing is the spectrum is null
            LOG(ERROR) << "Call to computeCentroidsWidths with a null spectrum";
            return;
        }
    
        bool detectPeaks = detectPeaksAgain<mz_t, int_t>(spectrum, centroids); // true means that peaks must be (re)centroided, false means that we use the vendors' centroids
        bool computeFWHM = computeFullWidthAtHalfMaximum(wantedMode); // true means that FWHM must be calculated, false means that we want centroid peaks only
        switch (fileType) {
            case pwiz::msdata::MS_ABI_WIFF_format :{
                detectPeaks = true; // re-detect peaks anyway because the default algorithm is not efficient enough
                mzPeakFinderQTof::findPeaks<mz_t, int_t>(spectrum, centroids, peakPickerParams, detectPeaks, computeFWHM);
                break;
            }
            case pwiz::msdata::MS_Thermo_RAW_format : {
                // detectPeaks should always be false here
                peakPickerParams.adaptiveBaselineAndNoise = false;
                peakPickerParams.noise = 0;
                peakPickerParams.baseline = 0;
                peakPickerParams.minSNR = 0;
                mzPeakFinderZeroBounded::findPeaks<mz_t, int_t>(spectrum, centroids, peakPickerParams, detectPeaks, computeFWHM);
                //mzPeakFinderWavelet::findPeaks<mz_t, int_t>(spectrum, centroids, peakPickerParams, detectPeaks, computeFWHM, mzPeakFinderUtils::CWT_DISABLED);
                break;
            }
            case pwiz::msdata::MS_Bruker_BAF_format : {
                // same as default mode, but always recentroid spectra
                detectPeaks = true; // re-detect peaks anyway because the default algorithm is not efficient enough
                peakPickerParams.adaptiveBaselineAndNoise = true;
                peakPickerParams.optimizationOpt = mzPeakFinderUtils::NO_OPTIMIZATION;
                peakPickerParams.minSNR = 0.0;
                peakPickerParams.fwhm = TOF_FWHM;
                mzPeakFinderWavelet::findPeaks<mz_t, int_t>(spectrum, centroids, peakPickerParams, detectPeaks, computeFWHM, mzPeakFinderUtils::CWT_ENABLED);
                break;
            }
            default: {
                // FIXME mzML files will use fall into this case, instead of their original mode
                // TODO check how it works when detectPeaks == false
                peakPickerParams.adaptiveBaselineAndNoise = true;
                peakPickerParams.optimizationOpt = mzPeakFinderUtils::NO_OPTIMIZATION;
                peakPickerParams.minSNR = 0.0;
                peakPickerParams.fwhm = TOF_FWHM;
                mzPeakFinderWavelet::findPeaks<mz_t, int_t>(spectrum, centroids, peakPickerParams, detectPeaks, computeFWHM, mzPeakFinderUtils::CWT_ENABLED);
                break;
            }
        }
    }
};


} //end namespace
#endif
