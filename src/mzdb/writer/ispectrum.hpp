#pragma once


#include "pwiz/data/msdata/MSData.hpp"
#include "../../utils/mzUtils.hpp"

#include "peak.hpp"

namespace mzdb {


using namespace std;

class mzAbstractSpectrum {};


template<class mz_t, class int_t, int msLevel, int IS_CENTROID>
class mzISpectrum : public mzAbstractSpectrum {

protected:
    int id, cycle, msLevel;
    float retentionTime;
    DataMode effectiveMode;
    pwiz::msdata::SpectrumPtr spectrum;

public:

    inline int initialPointsCount() const throw() {return spectrum->defaultArrayLength;}

    inline int nbPeaks() const throw(){
       if (effectiveMode != FITTED)
           return initialPointsCount();
       return peaks.size();
   }

    inline int msLevel() const {
        return msLevel;
    }

    //kind of lazy attribute
    inline const float& rt() {
        if (! retentionTime)
           retentionTime =  (float)spectrum->scanList.scans.front().cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds();
        return retentionTime;
    }

    inline double precursorMz() const {
        if (msLevel > 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            return si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
        }
        return -1;
    }

    inline float precursorIntensity() const {
        if (msLevel> 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            return si.cvParam(pwiz::msdata::MS_peak_intensity).valueAs<float>();
        }
        return -1;
    }

    inline int precursorCharge() const {
        if (msLevel > 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            return si.cvParam(pwiz::msdata::MS_charge_state).valueAs<int>();
        }
        return -1;
    }

};


template<class mz_t, class int_t, int msLevel>
class mzCentroidSpectrum : public mzISpectrum<mz_t, int_t, msLevel, 1> {
    const vector<mz_t>& mzData;
    const vector<int_t>& intData;

    public

};


template<class mz_t, class int_t, int msLevel>
class mzProfileSpectrum : public mzISpectrum<mz_t, int_t, msLevel, 0> {
    vector<Centroid<mz_t, int_t>*> centroids;

};


}
