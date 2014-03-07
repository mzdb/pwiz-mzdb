/* 
 * File:   mzSpectrum.h
 * Author: Marco
 *
 * Created on 4 mai 2012, 15:06
 */

#ifndef MZSPECTRUM_H
#define	MZSPECTRUM_H

#include "pwiz/data/msdata/MSData.hpp"

#include "../../utils/mzUtils.hpp"
#include "../mzPeakFinderProxy.hpp"


namespace mzdb {
using namespace std;

struct ISpectrum {

    int id, cycle;
    float retentionTime;
    DataMode effectiveMode;
    pwiz::msdata::SpectrumPtr spectrum;

};

template<typename mz_t, typename int_t> // could be a reference or simple vec
struct PWIZ_API_DECL mzSpectrum {
    int id, cycle;
    float retentionTime;
    DataMode effectiveMode;

    vector<std::shared_ptr<Centroid<mz_t, int_t> > > peaks;
    pwiz::msdata::SpectrumPtr spectrum; //smart pointer

    inline mzSpectrum() :id(-1),cycle(-1){}

    inline mzSpectrum(int id_, int cycle_, const pwiz::msdata::SpectrumPtr & s) : id(id_), cycle(cycle_), spectrum(s), retentionTime(0.0) {}

    //copy constructor
    mzSpectrum(const mzSpectrum<mz_t, int_t>& p) {
        if (! peaks.empty()) {
            for (size_t i = 0; i < peaks.size(); ++i) {
                delete peaks[i];
            }
            peaks.clear();
        }
        for (size_t i = 0; i < p.peaks.size(); ++i) {
            peaks.push_back(std::make_shared<Centroid<mz_t, int_t> >(*(p.peaks[i])));
        }
        id = p.id;
        cycle = p.cycle;
        effectiveMode = p.effectiveMode;
        spectrum = p.spectrum;
    }

    //affectation
     mzSpectrum<mz_t, int_t>& operator=(const mzSpectrum<mz_t, int_t> &p) {
        if (&p != this) {
            if (!peaks.empty()) {
                for (size_t i = 0; i < peaks.size(); ++i)
                    delete peaks[i];
            }
            for (size_t i = 0; i < p.peaks.size(); ++i) {
                peaks.push_back(std::make_shared<Centroid<mz_t, int_t>(*(p.peaks[i])));
            }
            id = p.id;
            cycle = p.cycle;
            effectiveMode = p.effectiveMode;
            spectrum = p.spectrum;
        }
        return *this;
    }

     /*~mzSpectrum() {
        for (size_t i = 0; i < peaks.size(); ++i) {
            delete peaks[i];
            //peaks[i] = 0;
        }
        peaks.clear();
        //mzSpectrum::deleteCount ++;
    }*/


     inline DataMode getEffectiveMode(DataMode wantedMode) {
         const pwiz::msdata::CVParam& isCentroided = spectrum->cvParam(pwiz::msdata::MS_centroid_spectrum);
         DataMode currentMode = ( isCentroided.empty() ) ? PROFILE: CENTROID;
         DataMode effectiveMode;
         if (wantedMode == PROFILE && currentMode == PROFILE) {
             effectiveMode = PROFILE;
         } else if ((wantedMode == CENTROID && currentMode == PROFILE) || (wantedMode == FITTED && currentMode == PROFILE)) {//findPeak then centroidize}
             effectiveMode = wantedMode;
         } else { // current is CENTROID nothing to do
             effectiveMode = CENTROID;
         }
         return effectiveMode;
     }



     /**
      * @brief doPeakPicking : perform peak picking, fill peaks attribute
      * @param m
      * @param ppa
      * @param threshold
      */
     inline void doPeakPicking(DataMode m, pwiz::msdata::CVID ppa, mzPeakFinderUtils::PeakPickerParams& params) {
        effectiveMode = mzPeakFinderProxy::computePeaks(spectrum, peaks, m, ppa, params);
    }

     /**********************************************************
      *Wrapper functions to reach commons SpectrumPtr attributes
      *********************************************************/
     inline int initialPointsCount() const throw() {return spectrum->defaultArrayLength;}

     inline int nbPeaks() const throw(){
        if (effectiveMode != FITTED)
            return initialPointsCount();
        return peaks.size();
    }

     inline int msLevel() const {
         return spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
     }

     //kind of lazy attribute
     inline const float& rt() {
         if (! retentionTime)
            retentionTime =  (float)spectrum->scanList.scans.front().cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds();
         return retentionTime;
     }

     inline double precursorMz() const {
         if (msLevel() > 1) {
             const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
             return si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
         }
         return -1;
     }

     inline float precursorIntensity() const {
         if (msLevel() > 1) {
             const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
             return si.cvParam(pwiz::msdata::MS_peak_intensity).valueAs<float>();
         }
         return -1;
     }

     inline int precursorCharge() const {
         if (msLevel() > 1) {
             const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
             return si.cvParam(pwiz::msdata::MS_charge_state).valueAs<int>();
         }
         return -1;
     }

     /** refine the precursor mz, assign the closest mass present in the spectrum*/
     inline void refinedPrecursorMz(pwiz::msdata::SpectrumPtr& parentSpectrum) const {
         //printf("refined prec\n");
         if (msLevel() == 1)
             return;//-1;
         /*if (! parentSpectrum) {
             printf("[mzSpectrum::refinedPrecursorMz] Empty parent spectrum pointer\n");
             return -1;
         }
         if (! parentSpectrum->spectrum.get()) {
             printf("[mzSpectrum::refinedPrecursorMz] Empty pwiz spectrum pointer\n");
             return -1;
         }

         //printf("after testing pointer\n");
         if (parentSpectrum->peaks.empty()) {
             printf("[mzSpectrum::refinedPrecursorMz] Empty centroid\n");
             return -1;
         }*/

         const auto& isolationWindow = spectrum->precursors.front().isolationWindow;
         const double mzPrec = this->precursorMz();
         const int chargePrec = this->precursorCharge();

         const double leftMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_lower_offset).valueAs<double>();
         const double rightMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_upper_offset).valueAs<double>();

         vector<double>& mzs = parentSpectrum->getMZArray()->data;
         vector<double>& ints = parentSpectrum->getIntensityArray()->data;

         auto lower = lower_bound(mzs.begin(), mzs.end(), mzPrec - leftMzWidth);
         auto higher = lower_bound(mzs.begin(), mzs.end(), mzPrec + rightMzWidth);

         vector<double> mzsSubset(lower, higher);
         vector<double> intsSubset(ints.begin() + (lower - mzs.begin()), ints.begin() + (higher - mzs.begin()));

         if (mzsSubset.empty())
             return;

         //centroidize this portion of spectra
         DataPointsCollection<double, double> collec(mzsSubset, intsSubset, parentSpectrum);
         //---TODO: add custom valye to this PeakPickerParams
         mzPeakFinderUtils::PeakPickerParams params;
         auto& peaksD = collec._detectPeaksCLASSIC(params);

         vector<double> mzsSubsetCentroided, intsSubsetCentroided;
         for (auto peakIt = peaksD.begin(); peakIt != peaksD.end(); ++peakIt) {
             auto* c = (*peakIt)->_computeCentroid();
             mzsSubsetCentroided.push_back(c->mz);
             intsSubsetCentroided.push_back(c->intensity);
             delete c;
         }

         if (mzsSubsetCentroided.empty())
             return;

         double sum = 0;
         for(auto it_ = intsSubsetCentroided.begin(); it_!=intsSubsetCentroided.end(); ++it_)
             sum += *it_;

         //test if the value exist
         //normally should never pass
         /*bool found = (find(mzsSubset.begin(), mzsSubset.end(), mzPrec) != mzsSubset.end());
         if (found) {
             printf("found in data\n");
             spectrum->precursors.front().selectedIons.front().userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(intPrec/sum), "xsd:float"));
             return mzPrec;
         }*/
         //not found
         auto it = lower_bound(mzsSubsetCentroided.begin(), mzsSubsetCentroided.end(), mzPrec);
         if (it == mzsSubsetCentroided.end()) { //no value greater than mzPrec, no need to look for the isotopic pattern
             it--;
             auto index = it - mzsSubsetCentroided.begin();
             spectrum->userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(intsSubsetCentroided[index]/sum), "xsd:float"));
             spectrum->userParams.push_back(pwiz::msdata::UserParam("nearestPrecMz", boost::lexical_cast<string>(*it), "xsd:float"));
             //return *it;

         }else {
             pwiz::msdata::MZIntensityPair r = mzSpectrum::getClosestPeak(mzsSubsetCentroided, intsSubsetCentroided, mzPrec);
             double& closestMz = r.mz;
             double& closestInt = r.intensity;
             if (closestMz = 0.0) {
                 spectrum->userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>( 0.0), "xsd:float"));
                 spectrum->userParams.push_back(pwiz::msdata::UserParam("nearestPrecMz", boost::lexical_cast<string>(0.0), "xsd:float"));
             }
             double s = mzSpectrum::getSumIsotopicPatternIntensities(mzsSubsetCentroided, intsSubsetCentroided, closestMz, closestInt, chargePrec, mzPrec + rightMzWidth, 10);
             spectrum->userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>( (float)(s/sum)), "xsd:float"));
             spectrum->userParams.push_back(pwiz::msdata::UserParam("nearestPrecMz", boost::lexical_cast<string>(closestMz), "xsd:float"));
             //return closestMz;
         }
     }

     static double getSumIsotopicPatternIntensities(const vector<double>& mzs, const vector<double>& ints,
                                                    const double& seedMz,
                                                    const double& seedInt,
                                                    const int& charge,
                                                    const int maxMz,
                                                    const double mzTol) {
        double sum = seedInt;
        double deltaMass = 1.002 / charge;
        double startMass = seedMz + deltaMass;
        //printf("startMass:%f, seedMz:%f\n", startMass);
        while (startMass < maxMz) {
            //printf("In the while loop\n");
            pwiz::msdata::MZIntensityPair result = mzSpectrum::getClosestPeak(mzs, ints, startMass);
            if (result ==  pwiz::msdata::MZIntensityPair(0.0,0.0))
                return sum;
            if ( fabs(startMass - result.mz) > mzTol * startMass / 1e6 ) {
                return sum;
            }
            sum += result.intensity;
            startMass += deltaMass;
            //printf("%f, %f\n", startMass, maxMz);
        }
        return sum;
     }


     static pwiz::msdata::MZIntensityPair getClosestPeak(const vector<double>& mzs, const vector<double>& ints, const double& mz) {
         auto it = lower_bound(mzs.begin(), mzs.end(), mz);

         //checks
         if (mzs.size() == 1 && ints.size() == 1) {
             //[mzSpectrum::getClosestPeak]: Unusual only one peak in isolation window
             return pwiz::msdata::MZIntensityPair(mzs.front(), ints.front());
         }
         if (it == mzs.end()) {
             //[mzSpectrum::getClosestPeak]: Error
             return pwiz::msdata::MZIntensityPair(0.0,0.0);
         }

         const double upperMz = *it; // first value larger
         auto maxIndex = it - mzs.begin(); // index of the larger value
         it--;
         const double lowerMz = *it;
         auto minIndex = it - mzs.begin();
         if (fabs(mz - upperMz) < fabs(mz - lowerMz))
             return pwiz::msdata::MZIntensityPair(upperMz, ints[maxIndex]);
         return pwiz::msdata::MZIntensityPair(lowerMz, ints[minIndex]);
    }


     /*static int getClosestIndex(const vector<double>& mzs, const double& mz) {
         auto it = lower_bound(mzs.begin(), mzs.end(), mz);
         if (it == mzs.end()) {
             return -1;
         }
         const double upperMz = *it;
         const double lowerMz = *(--it);
         if (fabs(mz - upperMz) < fabs(mz - lowerMz)) {
             return (++it) - mzs.begin();
         } else {
             return it - mzs.begin();
         }
     }*/
};

} // end namespace

#endif	/* MZSPECTRUM_H */

