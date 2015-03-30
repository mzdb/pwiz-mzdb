#include "BinnedSpectrum.h"

namespace mzdb {

using namespace std;


//BinnedSpectrum::BinnedSpectrum() {}

//vector<double> BinnedSpectrum::mzData = BinnedSpectrum::getMzDataInterval(300., 2001., 0.);


/*vector<double> BinnedSpectrum::getMzDataInterval(double minmz, double maxmz, double interval) {
    vector<double> mzData;
    BinnedSpectrum::IntervalType type;
    if ( interval == 0 ) {
        printf("\nInterval : Thermo defined\n");
        type = BinnedSpectrum::THERMO_DEFINED;
    } else {
        type = BinnedSpectrum::CONSTANT;
    }
    double min_ = minmz;
    while (min_ <= maxmz) {
        double inter = type == BinnedSpectrum::CONSTANT ? interval : mzMath::mzToDeltaMz(min_);
        mzData.push_back(min_);
        min_ += inter;

    }
    return mzData;
}*/

/*vector<double>& BinnedSpectrum::getMzDataInterval(pwiz::msdata::SpectrumPtr ptr) {
    return ptr->getMZArray()->data;
}*/


/*
size_t BinnedSpectrum::getNearestIndex(double mz) {
    auto low = lower_bound(mzData.begin(), mzData.end(), mz);
    size_t index = 0;
    //if (low != mzData.end())
    auto index_left = (low - mzData.begin() - 1) >= 0 ? low - mzData.begin() - 1: 0;
    auto index_right = low - mzData.begin();
    double left_diff = abs(mzData[index_left] - mz);
    double right_diff = abs(mzData[index_right] - mz);
    if (left_diff > right_diff)
        return index_right;
    else
        return index_left;
    //else
    //   printf("Error on inserting one data: mz = %f...\n", mz);
    //return index;
}*/


size_t BinnedSpectrum::getLeftIndex(double mz) {
    auto low = lower_bound(mzData.begin(), mzData.end(), mz);
    size_t index = 0;
    //if (low != mzData.end())
        index = (low - mzData.begin() - 1) >= 0 ? low - mzData.begin() - 1: 0;
    //else
    //   printf("Error on inserting one data: mz = %f...\n", mz);
    return index;
}

void BinnedSpectrum::putFromIndex(size_t index, double intens) {
    pair<double, double> range = make_pair(mzData[index], mzData[index + 1]);
    binnedSpectrum[range] += intens;
}

void BinnedSpectrum::resetFromIndex(size_t index, double intens) {
    pair<double, double> range = make_pair(mzData[index], mzData[index + 1]);
    binnedSpectrum[range] = intens;
}

double& BinnedSpectrum::getOrInitIntensityFromIndex(size_t index) {
    pair<double, double> range = make_pair(mzData[index], mzData[index + 1]);
    return binnedSpectrum[range];
}

pair<double, double> BinnedSpectrum::getPairFromIndex(size_t index) {
    return make_pair(mzData[index], mzData[index + 1]);
}

void BinnedSpectrum::put(pwiz::msdata::MZIntensityPair &p) {
    double& mz = p.mz;
    double& intens = p.intensity;
    bool found = false;
    auto low = lower_bound(mzData.begin(), mzData.end(), mz);
    if (low != mzData.end()) {
        size_t index = low - mzData.begin() - 1 >= 0 ? low - mzData.begin() - 1 : 0;
        pair<double, double> range = make_pair(mzData[index], mzData[index + 1]);
        binnedSpectrum[range] += intens;
        found = true;
    } else
        printf("Error on inserting one data...\n");
    if (! found ) {
        //TODO:expand
        printf("Mz: %f\n", mz);
        throw exception("[BinnedSpectrum::put()] a mz value could be binned\n");
    }
}


pair<vector<double>, vector<double> > BinnedSpectrum::getData() {
    //the original id was to remove bin which intensity was set to 0
    // but it's little bit tricky...
    vector<double> mzData, intData;
    //mzData.reserve(binnedSpectrum.size());
    //intData.reserve(binnedSpectrum.size());

    for (auto it = binnedSpectrum.begin(); it != binnedSpectrum.end(); ++it) {
        mzData.push_back(it->first.first);
        intData.push_back(it->second);
    }
    return make_pair(mzData, intData);
}

}
