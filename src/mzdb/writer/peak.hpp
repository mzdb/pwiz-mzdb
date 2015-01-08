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

//author marc.dubois@ipbs.fr

#ifndef __MZPEAK__
#define __MZPEAK__

#include <string>
#include <utility>
#include <exception>
#include <algorithm>
#include <math.h>
#include <cstdio>
#include <fstream>

#include "pwiz/data/msdata/MSData.hpp"
#include "../../utils/mzUtils.hpp"

#define MIN_SEMI_WIDTH 0.001f

namespace mzdb {
using namespace std;

template<class mz_t, class int_t>
struct PWIZ_API_DECL Centroid {

    mz_t mz;
    int_t intensity;
    float lwhm, rwhm, rt; //, background;


    inline Centroid() :mz(0.0), intensity(0.0), lwhm(0.0), rwhm(0.0), rt(0.0){}

    inline Centroid(mz_t& mz, int_t& intensity, float rt) : mz(mz), intensity(intensity), lwhm(0.0), rwhm(0.0), rt(rt){}

    inline Centroid(mz_t& mz, int_t& intensity, float lwhm, float rwhm) : mz(mz), intensity(intensity), lwhm(lwhm), rwhm(rwhm), rt(0.0) {}

    inline Centroid(const Centroid<mz_t, int_t>& c) {
        mz = c.mz;
        intensity = c.intensity;
        lwhm = c.lwhm;
        rwhm = c.rwhm;
        rt = c.rt;
    }
};



template<class mz_t, class int_t>
class PWIZ_API_DECL mzPeak {

    typedef std::shared_ptr<Centroid<mz_t, int_t> > CentroidSPtr;

protected:

    vector<mz_t> _mzData; //mz data points constituting the peak
    vector<int_t> _intData; //intensities data points stored float/double
    pwiz::msdata::SpectrumPtr _spectrum; //ref to the spectrum it belongs
    int _index; //index of its apex on Spectrum
    double snr;

public:

    inline int apexIndex() const throw() {
        if (_intData.empty())
            return -1;
        return max_element(_intData.begin(), _intData.end()) - _intData.begin();
    }

    inline mz_t apexMz() const throw() {return _mzData[apexIndex()];}
    inline int_t apexIntensity() const throw() {return _intData[apexIndex()];}


    inline float sumIntensities() const throw(){
        float sum = 0.0;
        for_each(_intData.begin(), _intData.end(), [&sum](int_t& v) { sum += v;});
        return sum;
    }

    //getters
    inline pwiz::msdata::SpectrumPtr spectrum() const throw() {return _spectrum;}
    inline vector<mz_t>& mzData() throw() {return _mzData;} //return a reference to mzData
    inline vector<int_t>& intData() throw() {return _intData;}//return a reference to intData
    inline int index() const throw() {return _index;}


    //setters
    inline void setSpectrum(const pwiz::msdata::SpectrumPtr &s) {_spectrum = s;}
    inline void setMzData(vector<mz_t>& _mzDat) {_mzData = _mzDat;}
    inline void setIntData(vector<int_t>& _intDat) {_intData = _intDat;}
    inline void setIndex(int _inde) {_index = _inde;}

    
    inline mzPeak(const pwiz::msdata::SpectrumPtr &s) : _spectrum(s), _index(0){}

    inline mzPeak(const vector<mz_t>& mz,
                         const vector<int_t>& ints,
                         const pwiz::msdata::SpectrumPtr& s):
        _mzData(mz), _intData(ints), _spectrum(s), _index(0) {

    }
    

    inline DataMode originMode() const{
        pwiz::data::CVParam& isCentroided = _spectrum->cvParam(pwiz::data::MS_centroid_spectrum);
        DataMode currentMode = ! isCentroided.empty() ? CENTROID : PROFILE;
        return currentMode;
    }


    inline float rt() const {
        pwiz::msdata::Scan* scan =  &_spectrum->scanList.scans[0];
        pwiz::msdata::CVParam& scanTimeParam = scan->cvParam(pwiz::msdata::MS_scan_start_time);
        return static_cast<float> (scanTimeParam.timeInSeconds());
    }


    //-----------------------------------------------------------------------------------------------------------
    //                             CENTROID FUNCTIONS

    PWIZ_API_DECL CentroidSPtr _computeCentroid() const{

        if (_mzData.empty() || _intData.empty()) {
            throw exception("Can not compute centroid: Empty data");
        }

        auto _centroid = std::make_shared<Centroid<mz_t, int_t> >();
        _centroid->rt = PwizHelper::rtOf(_spectrum);
        int nbDataPoints = _mzData.size();
        int apexPos = apexIndex();
        // Check that apex pos is a valid value

        if (apexPos < 0 || apexPos >= nbDataPoints) {
            throw exception("Invalid apex position");
            //return 0;
        }

        //set the background
        //const int_t& background = *min_element(_intData.begin(), _intData.end());
        const int_t& maxIntensity = _intData[apexPos];
        //_centroid->background = background;

        //printf("background:%f\n", background);

        _centroid->intensity = maxIntensity;
        int nbPointsLeft = apexPos;
        int nbPointsRight = nbDataPoints - (apexPos + 1);
        double halfY = maxIntensity * 0.5;

        if (_mzData.size() >= 3) {
            if (nbPointsLeft != 0 && nbPointsRight != 0 ) {
                //assume this is a symmetric peak
                vector<mz_t> apexMzs;
                vector<int_t> apexIntensities;

                for (int p = apexPos - 1; p <= apexPos + 1; p++) {
                    apexMzs.push_back(_mzData[p]);
                    apexIntensities.push_back(_intData[p]);
                }

                _centroid->mz = mzMath::gaussianCentroidApex<mz_t, int_t>(apexMzs, apexIntensities);

                //estimate fwhm
                try {
                    double fwhmX1 = mzPeak::interpolateXAtY(_mzData, _intData, apexPos, -1, halfY);
                    double fwhmX2 = mzPeak::interpolateXAtY(_mzData, _intData, apexPos, +1, halfY);
                    _centroid->lwhm = _centroid->mz - fwhmX1;
                    _centroid->rwhm = fwhmX2 - _centroid->mz;
                } catch (exception& ) {
                    double fwhmApprox = _mzData.back() - _mzData.front();
                    if (fwhmApprox == 0) {
                        throw exception("fwhm aproximation == 0\n");
                    }
                    _centroid->lwhm = fwhmApprox * 0.5;
                    _centroid->rwhm = fwhmApprox * 0.5;
                }

            } else {
                _centroid->mz = _mzData[apexPos];
                if (nbPointsLeft == 0){
                    //right
                    try {
                        double fwhmX2 = interpolateXAtY(_mzData, _intData, apexPos, +1, halfY);
                        _centroid->rwhm = fwhmX2 - _centroid->mz;
                        _centroid->lwhm = _centroid->rwhm;
                    } catch (exception& ){
                        double fwhmApprox = _mzData.back() - _mzData.front();
                        if (fwhmApprox == 0) {
                            throw exception("fwhm aproximation == 0\n");
                        }
                        _centroid->lwhm = fwhmApprox * 0.5;
                        _centroid->rwhm = fwhmApprox * 0.5;
                    }
                } else { // if (nbPointsRight == 0) {
                    //left
                    try {
                        double fwhmX1 = interpolateXAtY(_mzData, _intData, apexPos, -1, halfY);
                        _centroid->lwhm = _centroid->mz - fwhmX1;
                        _centroid->rwhm = _centroid->lwhm;
                    } catch (exception& ) {
                        double fwhmApprox = _mzData.back() - _mzData.front();
                        if (fwhmApprox == 0) {
                            throw exception("fwhm aproximation == 0\n");
                        }
                        _centroid->lwhm = fwhmApprox * 0.5;
                        _centroid->rwhm = fwhmApprox * 0.5;
                    }
                }
            }
        } else {
            //unable to compute anything
            _centroid->mz = _mzData[apexPos];
            if (_mzData.size() == 2) {
                double d = abs(_mzData[apexPos] - *min_element(_mzData.begin(), _mzData.end()));
                _centroid->lwhm = d;//MIN_SEMI_WIDTH;
                _centroid->rwhm = d;//MIN_SEMI_WIDTH;
            } else { //only one point*/
                _centroid->lwhm = MIN_SEMI_WIDTH;
                _centroid->rwhm = MIN_SEMI_WIDTH;
            }
        }
        if (_centroid->lwhm == 0 || _centroid->rwhm == 0) {
            _centroid->lwhm = MIN_SEMI_WIDTH;
            _centroid->rwhm = MIN_SEMI_WIDTH;
        }

        //if  (! _centroid || _centroid->mz < 0.0 || _centroid->mz > 3000.0)
        //    printf("ERROR mz centroid out of bound: %f\n", _centroid->mz);

        if (! mzMath::isFiniteNumber<mz_t>(_centroid->mz))
            _centroid->mz = _mzData[apexPos];

         //if (! mzMath::isFiniteNumber<mz_t>(_centroid->mz))
         //   printf("ERROR mz centroid is not a finite number: %f\n", _centroid->mz);

        return _centroid;
    }

    /**return is an implicit conversion*/
    inline static double interpolateXAtY(const vector<mz_t>& xData, const vector<int_t>& yData, int startIdx, int idxStep, double y)  {
        vector<pair<mz_t, int_t> >& twoPoints = getInterpolationPointsForY(xData, yData, startIdx, idxStep, y);
        pair<mz_t, int_t>& pointA = twoPoints[0];
        pair<mz_t, int_t>& pointB = twoPoints[1];

        if (pointA.second == y) {return pointA.first;}
        if (pointB.second == y) {return pointB.first;}
        return pointA.first + ( ( (y - pointA.second) * (pointB.first - pointA.first) ) / (pointB.second - pointA.second) );
    }


    inline static vector<pair<mz_t, int_t> > getInterpolationPointsForY(const vector<mz_t>& xData, const vector<int_t>& yData, int startIdx, int idxStep, double y) {

        if (idxStep < 0) {
            for (int i = startIdx; i + idxStep >= 0; i += idxStep) {
                if ( (y >= yData[i] && y <= yData[i + idxStep]) ||  (y >=yData[i + idxStep] && y <= yData[i])) {
                    vector<pair<mz_t, int_t> > r;
                    r.push_back(make_pair(xData[i + idxStep], yData[i + idxStep]));
                    r.push_back(make_pair(xData[i], yData[i]));
                    return r;
                }
            }
        } else {
            for (int i = startIdx;  i + idxStep < (int)xData.size(); i += idxStep) {
                if ( (y >= yData[i] && y <= yData[i + idxStep]) ||  (y >=yData[i + idxStep] && y <= yData[i])) {
                    vector<pair<mz_t, int_t> > r;
                    r.push_back(make_pair(xData[i], yData[i]));
                    r.push_back(make_pair(xData[i + idxStep], yData[i + idxStep]));
                    return r;
                }
            }
        }
        throw exception("hummm...that's embarassing, that was not supoosed to happend(google style)\n");
    }

};



}

#endif
