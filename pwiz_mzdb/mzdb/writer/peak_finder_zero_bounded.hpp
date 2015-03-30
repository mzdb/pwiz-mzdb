#ifndef MZPEAKFINDERTHERMO_H
#define MZPEAKFINDERTHERMO_H

#include <fstream>

#include "ceres/ceres.h"
#include "pwiz/data/msdata/MSData.hpp"

#include "data_points_collection.hpp"
#include "ceres_problems.hpp"
#include "peak_models.hpp"


namespace mzdb {

using namespace std;

namespace mzPeakFinderZeroBounded {


template<class mz_t, class int_t>
static void findPeaks(const pwiz::msdata::SpectrumPtr& s,
                      vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                      mzPeakFinderUtils::PeakPickerParams& params,
                      mzPeakFinderUtils::USE_CWT useCWT = mzPeakFinderUtils::CWT_DISABLED) {

    const vector<double>& mzs = s->getMZArray()->data;
    const vector<double>& ints = s->getIntensityArray()->data;

    if (mzs.empty() || ints.empty()) {
        /* actually, it can happen in practical cases, when  a
         * large file is processed */
        printf("Empty spectrum which is obviously unusual !\n");
        return;
    }


    if (params.adaptiveBaselineAndNoise) {
        const pair<double, double> baselineNoise = mzPeakFinderUtils::getBaselineAndNoise(ints);
        params.baseline = baselineNoise.first;
        params.noise = baselineNoise.second;
    }

    vector<mz_t> mzBuffer;
    vector<int_t> intBuffer;

    size_t i = 0;
    size_t mzSize = mzs.size();
    while ( i < mzSize ) {

        size_t j = (i > 0 ) ? i - 1 : 0;

        //---bufferize peaks above zero
        while ( ints[i] > 0 && i < mzSize ) {
            mzBuffer.push_back(mzs[i]);
            intBuffer.push_back(ints[i]);
            i++;
        }
        //---happens very often in large file (in practical cases)
        if (mzBuffer.size() >= 1) {
            if (j) {
                //--- ceres is complaining when intensities are set to zero...
                //--- dupe ceres putting 1e-3
                mzBuffer.insert(mzBuffer.begin(), mzs[j]);
                intBuffer.insert(intBuffer.begin(), (int_t)1e-3);
                mzBuffer.push_back(mzs[i]);
                intBuffer.push_back((int_t)1e-3);
            }

            DataPointsCollection<mz_t, int_t> collec(mzBuffer, intBuffer, s);
            collec.detectPeaks(params, useCWT);
            collec.optimize(centroids, params.optimizationOpt);

        }

        mzBuffer.clear(); //complexity: linear in size
        intBuffer.clear();
        i++;
    }
}

}//end namesapce mzThermo
}//end namespace mzdb





#endif // MZPEAKFINDERTHERMO_H


