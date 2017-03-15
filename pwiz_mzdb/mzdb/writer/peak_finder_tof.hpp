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
 * @file peak_finder_tof.hpp
 * @brief Peakpicking algorithm for AB Sciex spectra that must be centroided or fitted
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef PEAK_FINDER_TOF_H
#define PEAK_FINDER_TOF_H

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/utility/findmf/base/ms/peakpickerqtof.hpp"

#include "peak_finder_utils.hpp"

using namespace std;

namespace mzdb {

namespace mzPeakFinderQTof {

template<class mz_t, class int_t>
size_t insertCentroidIntoBuffers(vector<mz_t> &mzBuffer, vector<int_t> &intBuffer, std::shared_ptr<Centroid<mz_t, int_t>> centroid) {
    // get the position where the current centroid should be inserted
    size_t iId = 0;
    while(iId < mzBuffer.size()) {
        if(mzBuffer[iId] >= centroid->mz) break;
        iId++;
    }
    mzBuffer.insert(mzBuffer.begin()+iId, centroid->mz);
    intBuffer.insert(intBuffer.begin()+iId, centroid->intensity);
    return iId;
}

template<class mz_t, class int_t>
void removeCentroidFromBuffers(vector<mz_t> &mzBuffer, vector<int_t> &intBuffer, size_t id) {
    // remove the centroid
    mzBuffer.erase(mzBuffer.begin()+id);
    intBuffer.erase(intBuffer.begin()+id);
}

template<class mz_t, class int_t>
static void findPeaks(const pwiz::msdata::SpectrumPtr& spectrum,
                      vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                      mzPeakFinderUtils::PeakPickerParams& params,
                      bool detectPeaks = false,
                      bool computeFWHM = true) {
    try {
        // get mz and intensity vectors
        const vector<double>& mzs = spectrum->getMZArray()->data;
        const vector<double>& ints = spectrum->getIntensityArray()->data;
        if (mzs.empty() || ints.empty()) return;
        int_t almostNothing = (int_t)1e-3;
        // re-detect peaks if needed
        // on AB Sciex data, we should always use qtofpeakpicker to detect centroids
        // TODO make sure to detect peaks only once with AB Sciex data (don't detect peaks before this point, or run qtofpeakpicker in the first place and not here)

        if(detectPeaks) {
            //construct peak picker
            std::pair<double, double> range = std::make_pair(mzs.front(),mzs.back());
            // TODO qtofpeakpicker options should be automatically computed or left to the user through CLI options
            double resolution = 20000; // TODO find real value if it's in metadata or try to compute it, default value = 20000
            double smoothwidth = 1; // default value = 1
            uint32_t integrationWidth = 0; // default value = 2, mzdb best value: 0
            double intensityThreshold = 0; // default value = 10, mzdb best value: 0
            bool area = 0; // default value = 1, if 0 stores intensity, mzdb best value: 0
            uint32_t maxnumberofpeaks = 0; // default value = 0 (no limit), mzdb best value: 0
            // call qtofpeakpicker
            ralab::base::ms::PeakPicker<double, ralab::base::ms::SimplePeakArea> pp(resolution, range, smoothwidth, integrationWidth, intensityThreshold, area, maxnumberofpeaks);
            pp(mzs.begin(), mzs.end(), ints.begin());
            // replace the centroids vector with these new points
            float rt = static_cast<float>(spectrum->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());
            auto& centroidMzs = pp.getPeakMass();
            auto& centroidInts = pp.getPeakArea();
            // final verification: keep the old centroids if the new one are missing
            if(centroidMzs.size() > 0) {
                centroids.clear();
                for (size_t i = 0; i < centroidMzs.size(); ++i) {
                    mz_t c_mz = centroidMzs[i];
                    int_t c_int = centroidInts[i];
                    centroids.push_back(std::make_shared<Centroid<mz_t, int_t> >(c_mz, c_int, rt));
                }
            } else if(centroids.size() == 0) {
                // this may happen on noise-only spectra, all peaks have a low intensity (such as 21) and no centroid is returned
                // in this case, just use the most intense peak of profile spectrum and take it as the only centroid found
                // if it's noise, the position of the chosen peak in the spectrum doesn't matter
                mz_t mostIntenseMz = 0;
                int_t highestIntensity = 0;
                for (size_t i = 0; i < mzs.size(); ++i) {
                    if(ints[i] > highestIntensity) {
                        mostIntenseMz = mzs[i] + almostNothing; // adding a little something to avoid identical values (may cause problem with fitting)
                        highestIntensity = ints[i] + almostNothing; // adding a little something to avoid identical values (may cause problem with fitting)
                    }
                }
                centroids.clear();
                if(mostIntenseMz != 0) {
                    centroids.push_back(std::make_shared<Centroid<mz_t, int_t> >(mostIntenseMz, highestIntensity, rt));
                } else {
                    LOG(WARNING) << "No centroids given for spectrum '" << spectrum->id << "' and qtofpeakpicker failed to recalculate anything, this will result in an empty spectrum which will probably cause problems later !";
                }
            }
        }
        if(computeFWHM) {
            // make a loop on all profile peaks and create blocks of peaks
            // each block of peaks should contain one centroid and some peaks before and after
            // start at 0, continue until first centroid is met, continue until next centroid looking at the lowest peak between the two centroids
            // the block will contain peaks from position 0 to the lowest peak met
            // calculate fwhm on this block and then use ceres optimizer
            // start again until the end
            size_t cId = 0;
            size_t pIdStart = 0;
            size_t lastLowestPeakId = 0;
            int_t lastLowestPeakIntensity = centroids[0]->intensity; // first lowest intensity must be lower than that
            vector<std::shared_ptr<Centroid<mz_t, int_t>>> optimizedCentroids;
            // unique loop on each peak
            for (size_t i = 0; i < mzs.size(); ++i) {
                if(cId == centroids.size() - 1 || mzs[i] >= centroids[cId+1]->mz) {
                    if(cId == centroids.size() - 1) {
                        lastLowestPeakId = mzs.size() - 1;
                    }
                    // set vectors of peaks around the current centroid
                    vector<mz_t> mzBuffer;
                    vector<int_t> intBuffer;
                    for(size_t j = pIdStart; j <= lastLowestPeakId; j++) {
                        mzBuffer.push_back(mzs[j]);
                        if(ints[j] == 0) {
                            intBuffer.push_back(almostNothing);
                        } else {
                            intBuffer.push_back(ints[j]);
                        }
                    }
                    // create the data points collection with the current centroid
                    DataPointsCollection<mz_t, int_t> collec(mzBuffer, intBuffer, spectrum);
                    vector<std::shared_ptr<Centroid<mz_t, int_t> > > centroidBuffer;
                    centroidBuffer.push_back(centroids[cId]);
                    collec.setDetectedPeaks(centroidBuffer, params, mzPeakFinderUtils::CWT_DISABLED);
                    // compute fitted data and optimize it with ceres
                    collec.optimize(optimizedCentroids, mzPeakFinderUtils::GAUSS_OPTIMIZATION, computeFWHM);
                    // prepare for next block of peaks or quit
                    if(cId == centroids.size() - 1) {
                        break; // just exit for loop
                    } else {
                        pIdStart = lastLowestPeakId;
                        lastLowestPeakIntensity = centroids[cId+1]->intensity; // next lowest intensity must be lower than that
                        cId++;
                    }
                } else if(mzs[i] > centroids[cId]->mz && ints[i] < lastLowestPeakIntensity) {
                    lastLowestPeakId = i;
                    lastLowestPeakIntensity = ints[i];
                }
            }
            centroids = optimizedCentroids;
        }
    } catch(std::exception& e) {
        exitOnError(std::string("[QTOFPeakPicker] Error :") + e.what());
    }
}

}
}
#endif // PEAK_FINDER_TOF_H
