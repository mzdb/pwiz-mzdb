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
 * @file peak_finder_wavelet.hpp
 * @brief Peakpicking algorithm for Bruker spectra that must be centroided or fitted
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZPEAKPICKINGWAVELET_HPP
#define MZPEAKPICKINGWAVELET_HPP

#include "pwiz/data/msdata/MSData.hpp"

#include "peak.hpp"
#include "../../utils/cwtlib/cwtlib"
#include "peak_finder_utils.hpp"
#include "data_points_collection.hpp"


using namespace std;

namespace mzdb {

/**
 * find mass peaks using continuous wavelet transform at a specified scale
 * Algorihtm adapted to ABI, Bruker instruments (TOF instruments)
 */
namespace mzPeakFinderWavelet {

template<class mz_t, class int_t>
static void findPeaks(const pwiz::msdata::SpectrumPtr& spectrum,
                      vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                      mzPeakFinderUtils::PeakPickerParams& params,
                      bool detectPeaks = false, // not used but always true
                      bool computeFWHM = true) {

    const vector<double>& mzs = spectrum->getMZArray()->data;
    const vector<double>& ints = spectrum->getIntensityArray()->data;

    if (mzs.empty() || ints.empty()) {
        // actually, it can happen in practical cases, when  a large file is processed
        //printf("Empty spectrum which is obviously unusual !\n");
        return;
    }
    
    if (params.adaptiveBaselineAndNoise) {
        try {
            const pair<double, double> baselineNoise = mzPeakFinderUtils::getBaselineAndNoise(ints);
            params.baseline = baselineNoise.first;
            params.noise = baselineNoise.second;
        //} catch (exception& e) {
            // failed to compute baseline and noise, using default values (no need to log this)
            //printf("Spectrum '%s' exception: %s\n", spectrum->id.c_str(), e.what());
        } catch(...) {}
    }

    // TODO check if centroiding is ok
    // TODO add the detectPeaks condition anyway
    //---copy data du non corresponding template parameters ! TODO: how to change this
    const vector<mz_t> mzBuffer(mzs.begin(), mzs.end());
    const vector<int_t> intBuffer(ints.begin(), ints.end());
    DataPointsCollection<mz_t, int_t> spectrumData(mzBuffer, intBuffer, spectrum);

    //---detectPeaks using CWT, SNR filtered ( snr taken from params.minSNR )
    spectrumData._detectPeaksCWT(params);
    
    // optimize the new centroids and if requested calculate the FWHM values
    vector<std::shared_ptr<Centroid<mz_t, int_t> > > optimizedCentroids;
    spectrumData.optimize(optimizedCentroids, mzPeakFinderUtils::GAUSS_OPTIMIZATION, computeFWHM);
    
    // it may happen when the number of points is too low (ie. one point)
    // in this case, fit the original centroids
    if(optimizedCentroids.size() == 0 && centroids.size() > 0) {
        //printf("No optimized centroids for Spectrum %s, using %d default centroids\n", spectrum->id.c_str(), centroids.size());
        // just fit each centroid independently
        int_t almostNothing = (int_t)1e-3;
        for(size_t i = 0; i < centroids.size(); i++) {
            // create new buffers with 3 points: 0, centroid, 0
            mz_t tMz[3] = { centroids[i]->mz - almostNothing, centroids[i]->mz, centroids[i]->mz + almostNothing };
            vector<mz_t> mzBufferTemp(&tMz[0], &tMz[0]+3);
            int_t tInt[3] = { almostNothing, centroids[i]->intensity, almostNothing };
            vector<int_t> intBufferTemp(&tInt[0], &tInt[0]+3);
            // fit this centroids
            mzPeak<mz_t, int_t> peak(mzBufferTemp, intBufferTemp, spectrum);
            optimizedCentroids.push_back(peak._computeFittedCentroid());
        }
    }
    
    // store the new centroids or fitted peaks
    if(optimizedCentroids.size() > 0) {
        centroids = optimizedCentroids;
    }
}


}//end generic namespace

}//end mzdb nm



#endif // MZPEAKPICKINGWAVELET_HPP
