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
 * @file spectrum.hpp
 * @brief Represents a spectrum object
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef MZSPECTRUM_H
#define	MZSPECTRUM_H

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/analysis/spectrum_processing/PrecursorRecalculatorDefault.hpp"
#include "pwiz/analysis/peakdetect/PeakFamilyDetectorFT.hpp"

#include "../../utils/mzUtils.hpp"
#include "peak_finder_proxy.hpp"
#include "../utils/Lazy.hpp"

namespace mzdb {
using namespace std;
using namespace pwiz::analysis;

struct ISpectrum {

    int id, cycle;
    float retentionTime;
    DataMode effectiveMode;

    pwiz::msdata::SpectrumPtr spectrum;

};

/**
 * The mzSpectrum class
 * ====================
 *
 * Represents spectrum object.
 *
 * This a templated object:
 *  1. mz_t: type for mass over charge encoding
 *  2. int_t: type for intensity encoding
 */
template<typename mz_t=double, typename int_t=float>
struct PWIZ_API_DECL mzSpectrum {

    /// database ID
    int id;

    /// cycle number
    int cycle;

    /// retention time in seconds
    float retentionTime;

    /// effective storage mode
    DataMode effectiveMode;

    /// actual mode in raw file
    DataMode originalMode;

    /// isInHighRes true if ms1 or HCD ms2 scan false otherwise
    bool isInHighRes, isInserted;

    /// precusorCharge if spectrum has msLevel==2
    int _precursorCharge;

    /// precusorMz if spectrum has msLevel==2
    double _precursorMz;

    ///msLevel of the spectrum
    int _msLevel;

    /// vector containing the result of the fitting i.e. centroids
    vector<std::shared_ptr<Centroid<mz_t, int_t> > > peaks;
    
    /// fixed value for storing peak number
    /// peaks vector is lost when computing BBs and we still need to know its size if scan insertion is not done yet
    int _nbPeaks;

    /// pointer to the original pwiz spectrum
    pwiz::msdata::SpectrumPtr spectrum;


    ///Default ctor
    inline mzSpectrum() :
        id(-1),
        cycle(-1),
        isInHighRes(true),
        isInserted(false),
        _precursorCharge(0),
        _precursorMz(0.0),
        _msLevel(0) {
    }

    ///Most used ctor
    inline mzSpectrum(
        int id_,
        int cycle_,
        const pwiz::msdata::SpectrumPtr& s,
        const pwiz::msdata::SpectrumPtr& cs, // spectrum containing centroids
        DataMode wantedMode,
        bool safeMode
    ) :
        id(id_),
        cycle(cycle_),
        spectrum(s),
        retentionTime(0.0),
        isInHighRes(true),
        isInserted(false),
        _precursorCharge(0),
        _precursorMz(0.0),
        _msLevel(0) {
        // setup peaks vector
        if (cs != NULL) {
            // effectiveMode != PROFILE
            spectrumToMzDbPeaks(cs, peaks); // fills this.peaks with peaks centroided with vendors algorithms (TODO check if it's true !)
        } else {
            // effectiveMode == PROFILE
            spectrumToMzDbPeaks(s, peaks); // fils this.peaks with input peaks
        }
        // at this  point, this.peaks cannot be empty
        
        // set original mode
        originalMode = s->hasCVParam(pwiz::msdata::MS_profile_spectrum) ? PROFILE: CENTROID;
        /*
         * FIXME
         * This fix is related to https://github.com/mzdb/pwiz-mzdb/issues/45
         * Remove it when the ProteoWizard libraries will be updated
         * It is a bug seen on file IB12302LMU_1-D,1_01_5930.d, spectra have both CVs but they are centroided
         */
        if(spectrum->hasCVParam(pwiz::msdata::MS_profile_spectrum) && spectrum->hasCVParam(pwiz::msdata::MS_centroid_spectrum)) {
            LOG(WARNING) << "Spectrum '" << s->id << "' has both PROFILE and CENTROID tags and will be considered as CENTROID";
            originalMode = CENTROID;
        }
        
        // set effective mode
        if(originalMode == CENTROID && wantedMode != CENTROID) {
            if(safeMode) {
                effectiveMode = CENTROID;
            } else {
                //LOG(ERROR) << "Error: MS" << _msLevel << " is " << modeToString(originalMode) << " and cannot be turned into " << modeToString(wantedMode);
                //exitOnError("Current file contains centroid data that cannot be turned into profile/fitted data");
                std::stringstream errorMessage;
                errorMessage << "Current spectrum '" << s->id << "' is in " << modeToString(originalMode) << " and cannot be turned into " << modeToString(wantedMode) << "\nYou may use the '--safeMode' option to fall back to CENTROID mode automatically";
                exitOnError(errorMessage.str());
            }
        } else {
            effectiveMode = wantedMode;
        }
    }

    ///copy ctor
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
        originalMode = p.originalMode;
        spectrum = p.spectrum;
    }
    
    ~mzSpectrum() {
        if (! peaks.empty()) {
            for (size_t i = 0; i < peaks.size(); ++i) {
                //delete peaks[i];
                peaks[i].reset();
            }
            peaks.clear();
        }
        spectrum.reset();
    }

    ///affectation
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
            originalMode = p.originalMode;
            spectrum = p.spectrum;
        }
        return *this;
    }
    
    /**
     * Converts a proteowizard spectrum to mzdb peaks and fills this.peaks
     * When no peak picking is needed, we convert each data points into a centroid object.
     */
    inline void spectrumToMzDbPeaks(const pwiz::msdata::SpectrumPtr &s,
                                    vector<std::shared_ptr<Centroid<mz_t, int_t> > >& results) {
        /*turn into centroids objects */
        vector<pwiz::msdata::MZIntensityPair> pairs;
        s->getMZIntensityPairs(pairs);

        //---following lines remove all zeros of the intensity array
//        pairs.erase(std::remove_if(pairs.begin(), pairs.end(), [](const pwiz::msdata::MZIntensityPair& p) { return p.intensity == (int_t)0.0; }),
//                    pairs.end());

        results.resize(pairs.size());
        float rt = static_cast<float>(s->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());

        // std::transform will apply the given function to the specified range [pairs.begin -> pairs.end]
        // the result of the function will be put in this.peaks, represented by an iterator on its first element
        std::transform(pairs.begin(), pairs.end(), results.begin(), [&rt, s](pwiz::msdata::MZIntensityPair& p) -> std::shared_ptr<Centroid<mz_t, int_t> > {
            mz_t mz = (mz_t)p.mz; //std::move((mz_t)p.mz);
            int_t ints = (int_t)p.intensity; //std::move((int_t)p.intensity);
            return std::make_shared<Centroid<mz_t, int_t> >(mz, ints, rt);
        });
    }

    /**
      * Perform peak picking, fill peaks member et set the member
      * _effectiveMode_.
      *
      * @param ppa CVID representing filetype
      * @param peak picking params object
      */
    inline void doPeakPicking(pwiz::msdata::CVID ppa, mzPeakFinderUtils::PeakPickerParams& params) {
        if (effectiveMode != PROFILE) {
            int nb = peaks.size();
            mzPeakFinderProxy::computeCentroidsWidths<mz_t, int_t>(spectrum, peaks, ppa, params, effectiveMode);
            // update peak number if already set
            _nbPeaks = peaks.size();
        }
    }

    //------------------------------------------------------------------------------------------------
    //Wrapper functions to reach commons SpectrumPtr attributes

    //inline int initialPointsCount() const throw() {return spectrum->defaultArrayLength;}

    /**
     * return #mass peaks after fitting process
     * @return
     */
    inline int nbPeaks() {
        if (! _nbPeaks)
            _nbPeaks = peaks.size();
        return _nbPeaks;
    }

    inline int msLevel()  {
        if (! _msLevel)
            _msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
        return _msLevel;
    }
    
    inline DataMode getOriginalMode() {
        if(! originalMode) {
            // really ?? it should not happen !!!
            LOG(ERROR) << "Spectrum " << id << " has no original mode !!";
            originalMode = CENTROID;
        }
        return originalMode;
    }
    
    inline DataMode getEffectiveMode() {
        if(! effectiveMode) {
            // really ?? it should not happen !!!
            LOG(ERROR) << "Spectrum " << id << " has no effective mode !!";
            effectiveMode = CENTROID;
        }
        return effectiveMode;
    }

    //kind of lazy attribute
    inline const float& rt() {
        if (! retentionTime)
                retentionTime =  (float)spectrum->scanList.scans.front().cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds();

        return retentionTime;
    }

    inline double precursorMz()  {
        if (! _precursorMz) {
            //if (msLevel() > 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            _precursorMz = si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
        }
        //}
        //return -1;
        return _precursorMz;
    }

    inline float precursorIntensity() const {
        if (msLevel() > 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            return si.cvParam(pwiz::msdata::MS_peak_intensity).valueAs<float>();
        }
        return -1;
    }

    inline int precursorCharge()  {
        if (! _precursorCharge) {
            //if (msLevel() > 1) {
            const pwiz::msdata::SelectedIon& si = spectrum->precursors.front().selectedIons.front();
            _precursorCharge = si.cvParam(pwiz::msdata::MS_charge_state).valueAs<int>();
        }
        return _precursorCharge;
                //}
                //return -1;
    }

    //------------------------------------------------------------------------------------------------
    //Precursor refining
    /**
      * Really slow function. Leads to crash sometimes
      * @brief refinedPrecursorMzPwiz
      * @param parentSpectrum
      */
    //     template<typename U, typename V>
    //     void refinedPrecursorMzPwiz(std::shared_ptr<mzSpectrum<U, V> >& parentSpectrum,
    //                                 vector<pwiz::msdata::SelectedIon>& selectedIons) const{

    //         const auto& isolationWindow = spectrum->precursors.front().isolationWindow;
    //         const double mzPrec = this->precursorMz();

    //         const double leftMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_lower_offset).valueAs<double>() != 0.0 ?
    //                     isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_lower_offset).valueAs<double>(): 1.0;
    //         const double rightMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_upper_offset).valueAs<double>() != 0.0 ?
    //                      isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_upper_offset).valueAs<double>() : 1.0;

    //         PeakFamilyDetectorFT::Config pfdftConfig;
    //         pfdftConfig.cp = pwiz::msdata::CalibrationParameters::thermo_FT();
    //         boost::shared_ptr<PeakFamilyDetector> pfd(new PeakFamilyDetectorFT(pfdftConfig));

    //         // instantiate PrecursorRecalculatorDefault

    //         PrecursorRecalculatorDefault::Config config;
    //         config.peakFamilyDetector = pfd;
    //         config.mzLeftWidth = leftMzWidth;
    //         config.mzRightWidth = rightMzWidth;
    //         PrecursorRecalculatorDefault pr(config);

    //         // recalculate
    //         vector<pwiz::msdata::MZIntensityPair> pairs;
    //         parentSpectrum->getMZIntensityPairs(pairs);

    //         auto lower = lower_bound(pairs.begin(), pairs.end(),  mzPrec - leftMzWidth,  [](const pwiz::msdata::MZIntensityPair& lhs, const double rhs) {
    //             return lhs.mz < rhs;});

    //         auto higher = lower_bound(pairs.begin(), pairs.end(),  mzPrec + rightMzWidth,  [](const pwiz::msdata::MZIntensityPair& lhs, const double rhs) {
    //             return lhs.mz < rhs;});

    //         vector<pwiz::msdata::MZIntensityPair> subset(lower, higher);

    //         PrecursorRecalculator::PrecursorInfo initialEstimate;
    //         initialEstimate.mz = mzPrec;
    //         vector<PrecursorRecalculator::PrecursorInfo> result;
    //         pr.recalculate(&(*subset.begin()), &(*subset.end()), initialEstimate, result);

    //         if (result.empty())
    //             return;

    //          spectrum->userParams.push_back(
    //                      pwiz::msdata::UserParam("pwiz refined precursor mz",
    //                                              boost::lexical_cast<string>(result[0].mz),
    //                      "xsd:float"));
    //     }


    /** refine the precursor mz, assign the closest mass present in the spectrum*/
    //template<typename U, typename V>
    pwiz::msdata::SelectedIon refinedPrecursorMz(pwiz::msdata::SpectrumPtr& parentSpectrum, int& notFound,

                                                 //vector<pwiz::msdata::SelectedIon>& selectedIons,
                                                 float minPIP=0.20, size_t topN = 3) const {

        typedef std::shared_ptr<Centroid<double, double> > CentroidSPtr;

        //LOG(INFO) << "parentSpectrum centroids length:" << parentSpectrum->peaks.size();

        if (this->msLevel() == 1)
            return pwiz::msdata::SelectedIon();

        const auto& isolationWindow = spectrum->precursors.front().isolationWindow;
        const double mzPrec = this->spectrum->scanList.scans[0].userParam("[Thermo Trailer Extra]Monoisotopic M/Z:").valueAs<double>();//this->precursorMz();
        const double diff = mzPrec * 50 / 1e6;
        const double mxmz = mzPrec + diff;
        const double mnmz = mzPrec - diff;
        const int chargePrec = this->precursorCharge();

        const double leftMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_lower_offset).valueAs<double>() != 0.0 ?
                    isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_lower_offset).valueAs<double>(): 1.0;
        const double rightMzWidth = isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_upper_offset).valueAs<double>() != 0.0 ?
                    isolationWindow.cvParam(pwiz::msdata::MS_isolation_window_upper_offset).valueAs<double>() : 1.0;

        // get points corresponding to this region
        //         vector<double>& mzs = parentSpectrum->getMZArray()->data;
        //         vector<double>& ints = parentSpectrum->getIntensityArray()->data;

        //         auto lower = lower_bound(mzs.begin(), mzs.end(), mzPrec - leftMzWidth);
        //         auto higher = lower_bound(mzs.begin(), mzs.end(), mzPrec + rightMzWidth);

        //         vector<double> mzsSubset(lower, higher);
        //         vector<double> intsSubset(ints.begin() + (lower - mzs.begin()), ints.begin() + (higher - mzs.begin()));

        //         if (mzsSubset.empty()) {
        //             LOG(ERROR) << "No data points detected in isolation window...CRITIC";
        //             return pwiz::msdata::SelectedIon();
        //         }

        //         // centroidize this portion of spectra
        //         DataPointsCollection<double, double> collec(mzsSubset, intsSubset, parentSpectrum);
        //         //---TODO: add custom value to this PeakPickerParams
        //         mzPeakFinderUtils::PeakPickerParams params;
        //         params.optimizationOpt = 0x02;
        //         auto& peaksD = collec._detectPeaksCLASSIC(params);

        //vector<double> mzsSubsetCentroided, intsSubsetCentroided;
        vector<double>& mzs = parentSpectrum->getMZArray()->data;
        vector<double>& ints = parentSpectrum->getIntensityArray()->data;

        vector<CentroidSPtr> centroids;
        for (size_t i = 0, l = mzs.size(); i < l; ++i) {
            if (mzs[i] > mnmz && mzs[i] < mxmz) {
                CentroidSPtr c = std::make_shared<Centroid<double, double> >(mzs[i], ints[i], 0.0);
                centroids.push_back(c);
            }
        }
        //printf("length mzs:%d, %f, %f, %d\n", mzs.size(), mnmz, mxmz, centroids.size());


        //         for (auto peakIt = parentSpectrum->peaks.begin(); peakIt != parentSpectrum->peaks.end(); ++peakIt) {
        //             auto& ctr = *peakIt;
        //             if (ctr->mz > mnmz && ctr->mz < mxmz)
        //                centroids.push_back(ctr);
        //         }

        //--- no peaks detected
        if (centroids.empty()) {
            //LOG(ERROR) << "No peaks detected in isolation window...CRITIC";
            //printf("scanId nb: %s, precursor mz: %.6f, parent scanId:%s\n", spectrum->id.c_str(), mzPrec, parentSpectrum->id.c_str());
            notFound++;
            return pwiz::msdata::SelectedIon();
        }

        // sort by intensity
        std::sort(centroids.begin(), centroids.end(), [](const CentroidSPtr& a, const CentroidSPtr& b){
            return a->intensity < b->intensity;
        });

        auto& choosenOne = centroids.back();

        //calculate sum of intensities
        double sumCentroidsIntensities = 0;
        for(auto it_ = centroids.begin(); it_!=centroids.end(); ++it_)
            sumCentroidsIntensities += (*it_)->intensity;

        double sumIPIntensity = getSumIsotopicPatternIntensities(centroids, choosenOne, chargePrec, mzPrec + rightMzWidth, 10);
        double PIP = sumIPIntensity / sumCentroidsIntensities;
        pwiz::msdata::SelectedIon sion(choosenOne->mz, choosenOne->intensity, pwiz::msdata::MS_intensity_unit);
        sion.userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(PIP), XML_FLOAT));
        return sion;
        // sure we can remove old informations
        //selectedIons.clear();
        //         if (centroids.size() == 1) {
        //             auto& c = centroids.front();
        //             float PIP = c->intensity /sumCentroidsIntensities;
        //             pwiz::msdata::SelectedIon si(c->mz, c->intensity, pwiz::msdata::MS_intensity_unit);
        //             si.userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(PIP), xml_float));
        //             selectedIons.push_back(si);
        //             return;
        //         }



        //         // keep top 3
        //         int count = std::max<int>(0, centroids.size() - topN);// - 1);
        //         auto beginIt = centroids.begin() + count;
        //         auto endIt = centroids.end();

        //         // sum of intensities of detected centroids
        //         for (auto it = beginIt; it != endIt; ++it) {
        //            auto& c = *it;
        //            float PIP = c->intensity /sumCentroidsIntensities;
        //            if (PIP > minPIP) {
        //                // create a new Selected Ion
        //                pwiz::msdata::SelectedIon si(c->mz, c->intensity, pwiz::msdata::MS_intensity_unit);
        //                si.userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(PIP), xml_float));
        //                selectedIons.push_back(si);
        //            }
        //         }



        //         //calculate PIP for each centroids
        //         map<CentroidSPtr, float> PIPByCentroid;
        //         for (auto it = centroids.begin(); it != centroids.end(); ++it) {
        //            float PIP = (*it)->intensity /sumCentroidsIntensities;
        //            PIPByCentroid[(*it)] = PIP;
        //         }



        //not found
        //         auto it = lower_bound(mzsSubsetCentroided.begin(), mzsSubsetCentroided.end(), mzPrec);
        //         //no value greater than mzPrec, no need to look for the isotopic pattern
        //         if (it == mzsSubsetCentroided.end()) {
        //             it--;
        //             auto index = it - mzsSubsetCentroided.begin();
        //             spectrum->userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>(intsSubsetCentroided[index]/sum), xml_float));
        //             spectrum->userParams.push_back(pwiz::msdata::UserParam("mzdb refined precursor mz", boost::lexical_cast<string>(*it), xml_float));
        //         } else {
        //             pwiz::msdata::MZIntensityPair r = mzSpectrum::getClosestPeak(mzsSubsetCentroided, intsSubsetCentroided, mzPrec);
        //             double& closestMz = r.mz;
        //             double& closestInt = r.intensity;

        //             double s = mzSpectrum::getSumIsotopicPatternIntensities(mzsSubsetCentroided, intsSubsetCentroided, closestMz, closestInt, chargePrec, mzPrec + rightMzWidth, 10);
        //             spectrum->userParams.push_back(pwiz::msdata::UserParam("PIP", boost::lexical_cast<string>( (float)(s/sum)), xml_float));
        //             spectrum->userParams.push_back(pwiz::msdata::UserParam("mzdb refined precursor mz", mzdb_to_string(closestMz), xml_float));
        //         }
    }

    /** */
    static string mzdb_to_string(double& d) {
        char buff[100];
        sprintf(buff, "%1.9f", d);
        return string(buff);
    }

    /** */
    static double getSumIsotopicPatternIntensities(const vector<double>& mzs, const vector<double>& ints,
                                                   const double& seedMz,
                                                   const double& seedInt,
                                                   const int& charge,
                                                   const int maxMz,
                                                   const double mzTol) {
        double sum = seedInt;
        double deltaMass = 1.0027 / charge;
        double startMass = seedMz + deltaMass;
        while (startMass < maxMz) {
            pwiz::msdata::MZIntensityPair result = mzSpectrum::getClosestPeak(mzs, ints, startMass);
            if (result ==  pwiz::msdata::MZIntensityPair(0.0,0.0))
                return sum;
            if ( fabs(startMass - result.mz) > mzTol * startMass / 1e6 ) {
                return sum;
            }
            sum += result.intensity;
            startMass += deltaMass;
        }
        return sum;
    }

    template<typename U, typename V>
    static double
    getSumIsotopicPatternIntensities( vector<std::shared_ptr<Centroid<U, V> > >& centroids,
                                      std::shared_ptr<Centroid<U, V> > targetedCentroid,
                                      const int charge,
                                      const int maxMz,
                                      const double& mzTol) {

        typedef std::shared_ptr<Centroid<U, V> > CentroidSPtr;

        double sum = targetedCentroid->intensity;
        double deltaMass = 1.0027 / charge;
        double startMass = targetedCentroid->mz + deltaMass;
        while (startMass < maxMz) {
            CentroidSPtr c = mzSpectrum::getClosestCentroids(centroids, startMass, mzTol);
            if (c == nullptr)
                return sum;
            sum += c->intensity;
            startMass += deltaMass;
        }
        return sum;
    }

    template<typename U, typename V>
    static std::shared_ptr<Centroid<U, V> >
    getClosestCentroids(vector<std::shared_ptr<Centroid<U, V> > >& centroids, const double& mz, const double& mzTol) {
        typedef std::shared_ptr<Centroid<U, V> > CentroidSPtr;

        if (centroids.empty())
            return nullptr;

        //ensure this is correctly sorted in mz dimension
        std::sort(centroids.begin(), centroids.end(), [](CentroidSPtr& a, CentroidSPtr& b){
            return a->mz < b->mz;
        });

        auto it = std::lower_bound(centroids.begin(), centroids.end(), mz, [](CentroidSPtr& a, double d){
                return a->mz < d;
        });

        //checks
        if (it == centroids.end()) {
            auto& c = centroids.back();
            if ( fabs(mz - c->mz) > mzTol * mz / 1e6 ) {
                return nullptr;
            }
            return c;
        }

        const double upperMz = (*it)->mz; // first value larger
        auto maxIndex = it - centroids.begin(); // index of the larger value
        it--;
        const double lowerMz = (*it)->mz;
        auto minIndex = it - centroids.begin();
        if (fabs(mz - upperMz) < fabs(mz - lowerMz)) {
            auto& c = centroids[maxIndex];
            if ( fabs(mz - c->mz) > mzTol * mz / 1e6 ) {
                return nullptr;
            }else {
                return c;
            }
        }

        auto& c = centroids[minIndex];
        if ( fabs(mz - c->mz) > mzTol * mz / 1e6 ) {
            return nullptr;
        }
        return c;
    }


    static pwiz::msdata::MZIntensityPair getClosestPeak(const vector<double>& mzs, const vector<double>& ints, const double& mz) {
        auto it = lower_bound(mzs.begin(), mzs.end(), mz);

        //checks
        if (mzs.size() == 1 && ints.size() == 1) {
            return pwiz::msdata::MZIntensityPair(mzs.front(), ints.front());
        }
        if (it == mzs.end()) {
            return pwiz::msdata::MZIntensityPair(mzs.back(),ints.back());
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
};

} // end namespace

#endif	/* MZSPECTRUM_H */

