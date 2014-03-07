#ifndef BINNEDSPECTRUM_H
#define BINNEDSPECTRUM_H

#include <vector>
#include <map>

#include "pwiz/data/msdata/MSData.hpp"

#include "../utils/mzUtils.hpp"

namespace mzdb {

using namespace std;

struct BinnedSpectrum{

    enum IntervalType {
        CONSTANT,
        THERMO_DEFINED
    };

    vector<double> mzData;
    //static vector<double> getMzDataInterval(double minmz, double maxmz, double interval);
    //static vector<double> getMzDataInterval(pwiz::msdata::SpectrumPtr);


    double interval;
    /** pair <mz_min, mz_max>, intensity */
    //map<pair<double, double>, double> summedSpectrum;
    map<pair<double, double>, double> binnedSpectrum;

    /** put this pwiz object to the correct bin*/
    void put(pwiz::msdata::MZIntensityPair& p);

    /** return mz data and intensities data contained in the map */
    pair<vector<double>, vector<double> > getData();

    size_t getLeftIndex(double mz);
    void putFromIndex(size_t index, double intens);
    void resetFromIndex(size_t index, double intens);
    double& getOrInitIntensityFromIndex(size_t index);
    pair<double, double> getPairFromIndex(size_t index);

    /** constructor*/
    inline BinnedSpectrum(pwiz::msdata::SpectrumPtr ptr): mzData(ptr->getMZArray()->data){
        for (size_t i = 0; i < mzData.size() - 1; ++i) {
            binnedSpectrum[make_pair(mzData[i], mzData[i+1])] = 0;
        }
    }
    //inline BinnedSpectrum(){}

};

}
#endif // BINNEDSPECTRUM_H
