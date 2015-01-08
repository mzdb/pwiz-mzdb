#ifndef MZPEAKPICKINGWAVELET_HPP
#define MZPEAKPICKINGWAVELET_HPP

#include "pwiz/data/msdata/MSData.hpp"

#include "peak.hpp"
#include "../../utils/cwtlib/cwtlib"
#include "peak_finder_utils.hpp"
#include "data_points_collection.hpp"


namespace mzdb {

using namespace std;

namespace mzPeakFinderWavelet {


/**
 *
 */
template<class mz_t, class int_t>
static void findPeaks(const pwiz::msdata::SpectrumPtr& s,
                      vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                      mzPeakFinderUtils::PeakPickerParams& params) {

    const vector<double>& mzs = s->getMZArray()->data;
    const vector<double>& ints = s->getIntensityArray()->data;

    if (mzs.empty() || ints.empty()) {
        //--- actually, it can happen in practical cases, when  a
        //--- large file is processed
        LOG(WARNING) << "Empty spectrum which is obviously unusual !";
        return;
    }

//    if (params.adaptiveBaselineAndNoise) {
//        const pair<double, double> c = mzPeakFinderUtils::getBaselineAndNoise(ints);
//        params.baseline = c.first;
//        params.noise = c.second;
//    }

    //---copy data ! TODO: how to change this
    const vector<mz_t> nmzs(mzs.begin(), mzs.end());
    const vector<int_t> nints(ints.begin(), ints.end());
    DataPointsCollection<mz_t, int_t> spectrumData(nmzs, nints, s);

    //---detectPeaks using CWT, snr filtered ( snr taken from params.minSNR )
    spectrumData._detectPeaksCWT(params);
    spectrumData.optimize(centroids, 0x01); //params.optimizationOpt);
}


}//end generic namespace

}//end mzdb nm



#endif // MZPEAKPICKINGWAVELET_HPP
