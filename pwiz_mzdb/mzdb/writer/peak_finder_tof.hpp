#ifndef PEAK_FINDER_TOF_H
#define PEAK_FINDER_TOF_H

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/utility/findmf/base/ms/peakpickerqtof.hpp"

#include "peak_finder_utils.hpp"

using namespace std;

namespace mzdb {

namespace mzPeakFinderQTof {


template<class mz_t, class int_t>
static void findPeaks(const pwiz::msdata::SpectrumPtr& s,
                      vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids,
                      mzPeakFinderUtils::PeakPickerParams& params) {
    try {
        std::vector<double>& mzs = s->getMZArray()->data;
        std::vector<double>& intensities = s->getIntensityArray()->data;

        /// if empty spectrum return withouth processing
        if(mzs.empty() ){
            return;
        }
        std::pair<double, double> range = std::make_pair(mzs.front(),mzs.back());

        //construct peak picker
        ralab::base::ms::PeakPicker<double, ralab::base::ms::SimplePeakArea> pp(params.ppm, range);
//                    smoothwidth_,
//                    integrationWidth_,
//                    intensityThreshold_,
//                    area_,
//                    maxnumberofpeaks_
//                    );
        pp( mzs.begin(), mzs.end(), intensities.begin());


        float rt = static_cast<float>(s->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());
        auto& mzsCent = pp.getPeakMass();
        auto& intensitiesCent = pp.getPeakArea();
        for (size_t i = 0; i < mzsCent.size(); ++i) {
            mz_t m = mzsCent[i];
            int_t ints = intensitiesCent[i];
            auto centroid = std::make_shared<Centroid<mz_t, int_t> >(m, ints, rt);
            centroids.push_back(centroid);
        }

    } catch(std::exception& e) {
        throw std::runtime_error(std::string("[QTOFPeakPicker] Error :") + e.what());
    }
}

}
}
#endif // PEAK_FINDER_TOF_H
