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

        const vector<double>& mzs = spectrum->getMZArray()->data;
        const vector<double>& ints = spectrum->getIntensityArray()->data;
    
        if (mzs.empty() || ints.empty()) {
            // actually, it can happen in practical cases, when  a large file is processed
            //printf("Empty spectrum which is obviously unusual !\n");
            return;
        }
        
        if(detectPeaks) {
            //construct peak picker
            std::pair<double, double> range = std::make_pair(mzs.front(),mzs.back());
            double resolution = 20000; // TODO find real value in metadata, default value = 20000
            double smoothwidth = 1; // default value = 1
            uint32_t integrationWidth = 0; // default value = 2, mzdb best value: 0
            double intensityThreshold = 0; // default value = 10, mzdb best value: 0
            bool area = 0; // default value = 1, if 0 stores intensity, mzdb best value: 0
            uint32_t maxnumberofpeaks = 0; // default value = 0 (no limit), mzdb best value: 0
            // call qtofpeakpicker
            ralab::base::ms::PeakPicker<double, ralab::base::ms::SimplePeakArea> pp(resolution, range, smoothwidth, integrationWidth, intensityThreshold, area, maxnumberofpeaks);
            pp(mzs.begin(), mzs.end(), ints.begin());
            // replace the centroids vector with these new points
            vector<std::shared_ptr<Centroid<mz_t, int_t> > > recalculatedCentroids;
            float rt = static_cast<float>(spectrum->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());
            auto& centroidMzs = pp.getPeakMass();
            auto& centroidInts = pp.getPeakArea();
            // final verification: keep the old centroids if the new one are missing
            if(centroidMzs.size() > 0) {
                for (size_t i = 0; i < centroidMzs.size(); ++i) {
                    mz_t c_mz = centroidMzs[i];
                    int_t c_int = centroidInts[i];
                    recalculatedCentroids.push_back(std::make_shared<Centroid<mz_t, int_t> >(c_mz, c_int, rt));
                }
                centroids = recalculatedCentroids;
            }
        }

        if(computeFWHM) {
            vector<std::shared_ptr<Centroid<mz_t, int_t> > > fittedCentroids;
            vector<mz_t> mzBuffer;
            vector<int_t> intBuffer;
    
            size_t i = 0;
            size_t lastCentroidId = 0;
            size_t mzSize = mzs.size();
    
            mz_t lastMz = 0;
            int_t lastInt = 0;
            int_t almostNothing = (int_t)1e-3;
            while(i < mzSize) {
                /*
                 * We are only looking at the current and previous lines
                 * 4 possible cases: 
                 * 0 -> 0 : nothing to do
                 * 0 -> X : new buffers
                 * X -> 0 : process buffers and clear
                 * X -> Y : append to buffers
                 *
                 * for each centroid, get all the positive points on the left and right
                 * create a mzPeak object from these points : PeakUPtr p(new mzPeak<mz_t, int_t>(lmzs, lints, spectrum));
                 * compute the fitted centroid: auto fc = peak->_computeFittedCentroid();
                 * add the fitted centroid to the fittedCentroids vector: fittedCentroids.push_back(fc);
                 */
                //if(lastInt == 0 && ints[i] == 0) {} else // case 1 : nothing to do
                if(lastInt == 0 && ints[i] > 0) { // case 2 : new buffers
                    // add an empty item at the beginning of the buffer, if i == 0 create a fake mz
                    mzBuffer.push_back(lastMz == 0 ? mzs[i] - almostNothing : lastMz);
                    intBuffer.push_back(almostNothing);
                    // append the first real point
                    mzBuffer.push_back(mzs[i]);
                    intBuffer.push_back(ints[i]);
                } else if(lastInt > 0 && ints[i] == 0) { // case 3 : process buffers
                    // add an empty item at the end of the buffer
                    mzBuffer.push_back(mzs[i]);
                    intBuffer.push_back(almostNothing);
                    // get all centroids contained in this mz range
                    size_t cId = lastCentroidId;
                    while(cId < centroids.size() && centroids[cId]->mz <= mzBuffer[mzBuffer.size() - 1]) {
                        if(centroids[cId]->mz >= mzBuffer[0]) {
                            // add the centroid if it has been recalculated (because it would not be part of the original points)
                            size_t iId = 0; // id of the future inserted item
                            if (detectPeaks) iId = insertCentroidIntoBuffers<mz_t, int_t>(mzBuffer, intBuffer, centroids[cId]);
                            // create a mzPeak object from these points
                            mzPeak<mz_t, int_t> peak(mzBuffer, intBuffer, spectrum);
                            //add the fitted centroid to the fittedCentroids vector
                            fittedCentroids.push_back(peak._computeFittedCentroid());
                            if (detectPeaks) removeCentroidFromBuffers<mz_t, int_t>(mzBuffer, intBuffer, iId);
                        } else { // single centroid point
                            // create new buffers with 3 points: 0, centroid, 0
                            mz_t tMz[3] = { centroids[cId]->mz - almostNothing, centroids[cId]->mz, centroids[cId]->mz + almostNothing };
                            vector<mz_t> mzBufferTemp(&tMz[0], &tMz[0]+3);
                            int_t tInt[3] = { almostNothing, centroids[cId]->intensity, almostNothing };
                            vector<int_t> intBufferTemp(&tInt[0], &tInt[0]+3);
                            // fit this centroids
                            mzPeak<mz_t, int_t> peak(mzBufferTemp, intBufferTemp, spectrum);
                            fittedCentroids.push_back(peak._computeFittedCentroid());
                        }
                        cId++;
                    }
                    lastCentroidId = cId;
                    mzBuffer.clear();
                    intBuffer.clear();
                } else if(lastInt > 0 && ints[i] > 0) { // case 4 : append to buffers
                    mzBuffer.push_back(mzs[i]);
                    intBuffer.push_back(ints[i]);
                }
                lastMz = mzs[i];
                lastInt = ints[i];
                i++;
            }
            
            // deal with eventual final peaks
            if(!mzBuffer.empty()) {
                // redo case 3 : process buffers (see comments above)
                mzBuffer.push_back(mzs[i]);
                intBuffer.push_back(almostNothing);
                size_t cId = lastCentroidId;
                while(cId < centroids.size() && centroids[cId]->mz) {
                    if(centroids[cId]->mz >= mzBuffer[0]) {
                        size_t iId = 0;
                        if (detectPeaks) iId = insertCentroidIntoBuffers<mz_t, int_t>(mzBuffer, intBuffer, centroids[cId]);
                        mzPeak<mz_t, int_t> peak(mzBuffer, intBuffer, spectrum);
                        fittedCentroids.push_back(peak._computeFittedCentroid());
                        if (detectPeaks) removeCentroidFromBuffers<mz_t, int_t>(mzBuffer, intBuffer, iId);
                    } else { // single centroid point
                        // TODO create a function for this !
                        // create new buffers with 3 points: 0, centroid, 0
                        mz_t tMz[3] = { centroids[cId]->mz - almostNothing, centroids[cId]->mz, centroids[cId]->mz + almostNothing };
                        vector<mz_t> mzBufferTemp(&tMz[0], &tMz[0]+3);
                        int_t tInt[3] = { almostNothing, centroids[cId]->intensity, almostNothing };
                        vector<int_t> intBufferTemp(&tInt[0], &tInt[0]+3);
                        // fit this centroids
                        mzPeak<mz_t, int_t> peak(mzBufferTemp, intBufferTemp, spectrum);
                        fittedCentroids.push_back(peak._computeFittedCentroid());
                    }
                    cId++;
                }
            }
            // clear buffers
            mzBuffer.clear();
            intBuffer.clear();
            
            // store the new centroids or fitted peaks
            centroids = fittedCentroids;
        }
    } catch(std::exception& e) {
        exitOnError(std::string("[QTOFPeakPicker] Error :") + e.what());
    }
}

}
}
#endif // PEAK_FINDER_TOF_H
