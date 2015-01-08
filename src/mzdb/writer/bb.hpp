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

/*
BoundingBox class
use to store minmz, maxmz, minrt, maxrt
 */

#ifndef __BB__
#define __BB__

#include <algorithm>
#include <unordered_map>
#include "pwiz/data/msdata/MSData.hpp"

#include "../../utils/mzUtils.hpp"
#include "peak.hpp"

namespace mzdb {
using namespace std;

template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
class PWIZ_API_DECL mzBoundingBox {

    typedef std::shared_ptr<Centroid<h_mz_t, h_int_t> > HighResCentroidSPtr;
    typedef std::shared_ptr<Centroid<l_mz_t, l_int_t> > LowResCentroidSPtr;

protected:

    //both map can be null
    unique_ptr<map<int, vector<HighResCentroidSPtr> > > _highResPeaksByScanIDs;
    unique_ptr<map<int, vector<LowResCentroidSPtr> > > _lowResPeaksByScanIDs;

    float _mzmin, _mzmax, _rtmin, _rtmax; //use for descriptive field no precision needed
    int _runSliceIdx;


public:

    inline float mzmin() const throw() {return _mzmin;}

    inline float mzmax() const throw() {return _mzmax;}

    inline float rtmax() const throw() {return _rtmax;}

    inline float rtmin() const throw() {return _rtmin;}

    inline int runSliceIdx() const throw() {return _runSliceIdx;}

    inline void setRunSliceIdx(int idx) throw() {_runSliceIdx = idx;}

    inline bool isEmpty() const throw () {
        return _highResPeaksByScanIDs->empty() && _lowResPeaksByScanIDs->empty();
    }

    /** */
    inline mzBoundingBox( unique_ptr<map<int, vector<HighResCentroidSPtr> > >&& h,
                          unique_ptr<map<int, vector<LowResCentroidSPtr> > >&& l):
        _highResPeaksByScanIDs(move(h)), _lowResPeaksByScanIDs(move(l)), _runSliceIdx(0),
        _rtmin(0), _rtmax(0), _mzmin(0), _mzmax(0) {

    }

    /** */
    mzBoundingBox(int idx,
                  float height,
                  unique_ptr<map<int, vector<HighResCentroidSPtr> > >& h,
                  unique_ptr<map<int, vector<LowResCentroidSPtr> > >& l):
        _highResPeaksByScanIDs(move(h)), _lowResPeaksByScanIDs(move(l)) {

        _mzmin = idx * height;
        _mzmax = _mzmin + height;
        _runSliceIdx = idx;

        if ( ! _highResPeaksByScanIDs->empty() || ! _lowResPeaksByScanIDs->empty()) {
            this->setRtBoundaries();
        }
    }

    /** */
    /*inline mzBoundingBox(float mmin, float mmax,
                         float rmin, float rmax,
                         map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& h,
                         Ownership ownership):
        _mzmin(mmin), _mzmax(mmax),
        _rtmin(rmin), _rtmax(rmax),
        _peaksByScanIDs(h),
        _runSliceIdx(0), _ownership(ownership) {}
    */

    /** move constructor */
    /*inline mzBoundingBox(mzBoundingBox&& o):
        _mzmin(o._mzmin), _mzmax(o._mzmax),
        _rtmin(o._rtmin), _rtmax(o._rtmax),
        _runSliceIdx(o._runSliceIdx), _ownership(o._ownership) {

        _peaksByScanIDs = std::move(o._peaksByScanIDs);
        o._mzmin = nullptr;
        o._mzmax = nullptr;
        o._rtmin = nullptr;
        o._rtmax = nullptr;
        o._runSliceIdx = nullptr;
        o._ownership = nullptr;
    }*/


    /**
     * @brief ~mzBoundingBox
     * Destruct map pointer if the bounding box has been
     * Else it is assumed that pointer will be automatically
     * be destroyed
     */


    /**
     * @brief computeMzBounds
     */
    /*void computeMzBounds() {
        if (! _peaksByScanIDs.empty())
           this->setMzBoundaries();
    }*/

    template<class mz_t, class int_t>
    inline static int getByteLength(map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& m,
                                    map<int, DataMode>& dataModes) {
        int sum = 0;
        for(auto it = m.begin(); it != m.end(); ++it ) {
            const int& idx = it->first;
            auto& peaks = it->second;
            int sizeStruct = sizeof(mz_t) + sizeof(int_t);
            if (dataModes[idx] == FITTED)
                sizeStruct += 2 * sizeof(float);
            size_t N = peaks.size();
            sum +=  N * sizeStruct + 2 * sizeof(int);
        }
        return sum;
    }

    /** */
    int getBytesVectorLength(map<int, DataMode>& dataModes) const {
        int sum = 0;
        if (! _highResPeaksByScanIDs->empty())
            sum += mzBoundingBox::getByteLength<h_mz_t, h_int_t>(_highResPeaksByScanIDs, dataModes);
        if (! _lowResPeaksByScanIDs->empty())
            sum += mzBoundingBox::getByteLength<l_mz_t, l_int_t>(_lowResPeaksByScanIDs, dataModes);
        return sum;
    }

    /** */
    void asByteArray(vector<byte>& v, map<int, DataMode>& dataModes) const {
        vector<pair<int, int> > o;
        this->iterationOrder(o);
        for (size_t i = 0; i < o.size(); ++i) {
            const auto& p = o[i];
            const int& idx = p.second;
            auto& dm = dataModes[idx];
            if (p.first == 1) {
                // high res mode
                if (dm == FITTED)
                    insertFittedToBinaryVector<h_mz_t, h_int_t>(v, idx, *_highResPeaksByScanIDs);
                else
                    insertCentroidToBinaryVector<h_mz_t, h_int_t>(v, idx, *_highResPeaksByScanIDs);
            } else {
                // low res mode
                if (dm == FITTED)
                    insertFittedToBinaryVector<l_mz_t, l_int_t>(v, idx, *_lowResPeaksByScanIDs);
                else
                    insertCentroidToBinaryVector<l_mz_t, l_int_t>(v, idx, *_lowResPeaksByScanIDs);
            }
        }
    }

    /** */
    template<typename mz_t, typename int_t>
    inline static void insertCentroidToBinaryVector(vector<byte>& v, int idx,
                                                    map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& w)  {
        auto& peaks = w[idx];
        put<int>(idx, v);
        put<int>(peaks.size(), v);
        for (size_t j = 0; j < peaks.size(); ++j) {
            Centroid<mz_t, int_t>& p_ = *(peaks[j]);
            put<mz_t>(p_.mz, v);
            put<int_t>(p_.intensity, v);
        }
    }

    /** */
    template<typename mz_t, typename int_t>
    inline static void insertFittedToBinaryVector(vector<byte>& v, int idx,
                                                  map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& w) {
        auto& peaks = w[idx];
        put<int>(idx, v);
        put<int>(peaks.size(), v);
        for (size_t j = 0; j < peaks.size(); ++j) {
            Centroid<mz_t, int_t>& p_ = *(peaks[j]);
            put<mz_t>(p_.mz, v);
            put<int_t>(p_.intensity, v);
            put<float>(p_.lwhm, v);
            put<float>(p_.rwhm, v);
        }
    }

    /** */
    inline void update(map<int, vector<HighResCentroidSPtr> >& h,
                       map<int, vector<LowResCentroidSPtr> >& l) throw(){
        for (auto it = h.begin(); it != h.end(); ++it) {
            const int& p = it->first;
            if (_highResPeaksByScanIDs->find(p) == _highResPeaksByScanIDs->end()) {
                (*_highResPeaksByScanIDs)[p]; //call the default ctor
            }
        }
        for (auto it = l.begin(); it != l.end(); ++it) {
            const int& p = it->first;
            if (_lowResPeaksByScanIDs->find(p) == _lowResPeaksByScanIDs->end()) {
                (*_lowResPeaksByScanIDs)[p]; //call the default ctor
            }
        }
    }

    /** */
    inline void setRtBoundaries() {
        _rtmin = 1e9;
        for (auto it = _highResPeaksByScanIDs->begin(); it != _highResPeaksByScanIDs->end(); ++it) {
            if ( it->second.empty())
                continue;
            Centroid<h_mz_t, h_int_t>& p = *(it->second.front());
            _rtmin = p.rt;
            break;
        }
        for (auto it = _lowResPeaksByScanIDs->begin(); it != _lowResPeaksByScanIDs->end(); ++it) {
            if ( it->second.empty())
                continue;
            Centroid<l_mz_t, l_int_t>& p = *(it->second.front());
            _rtmin = std::min(p.rt, _rtmin);
            break;
        }

        _rtmax = 0;
        for (auto it = _highResPeaksByScanIDs->rbegin(); it != _highResPeaksByScanIDs->rend(); ++it) {
            if ( it->second.empty())
                continue;
            Centroid<h_mz_t, h_int_t>& p = *(it->second.front());
            _rtmax = p.rt;
            break;
        }
        for (auto it = _lowResPeaksByScanIDs->rbegin(); it != _lowResPeaksByScanIDs->rend(); ++it) {
            if ( it->second.empty())
                continue;
            Centroid<l_mz_t, l_int_t>& p = *(it->second.front());
            _rtmax = std::max(p.rt, _rtmin);
            break;
        }
    }

    /** not used even if we can get slightly better performance in R*Tree request */
    /*inline void setMzBoundaries()  {
        _mzmin = 1e9, _mzmax = 0;
        for (auto it = _peaksByScanIDs.begin(); it != _peaksByScanIDs.end(); ++it) {
            vector<Centroid<mz_t, int_t>*>&  vec = it->second;
            if ( vec.empty())
                continue;
            Centroid<mz_t, int_t>* p1 = vec.front();
            Centroid<mz_t, int_t>* p2 = vec.back();
            _mzmin = min( _mzmin, (float) p1->mz );
            _mzmax = max( _mzmax, (float) p2->mz );
        }
    }*/

    struct PairSorter {
        bool operator() (const pair<int, int>&a, const pair<int, int>& b) {
            return (a.second < b.second);
        }
    };

    /** Iteration order */
    inline void iterationOrder(vector<pair<int, int> >& v) const {

        for (auto it = _highResPeaksByScanIDs->begin(); it != _highResPeaksByScanIDs->end(); ++it)
            v.push_back(make_pair(1, it->first));
        for (auto it = _lowResPeaksByScanIDs->begin(); it != _lowResPeaksByScanIDs->end(); ++it)
            v.push_back(make_pair(2, it->first));
        std::sort(v.begin(), v.end(),mzBoundingBox::PairSorter());
    }

    // static version, assume both highrespeaks and lowrespeaks are not empty
    inline static void iterationOrder(map<int, vector<HighResCentroidSPtr> >& highResPeaks,
                                      map<int, vector<LowResCentroidSPtr> >& lowResPeaks,
                                      vector<pair<int, int> >& v) {
        for (auto it = highResPeaks.begin(); it != highResPeaks.end(); ++it)
            v.push_back(make_pair(1, it->first));
        for (auto it = lowResPeaks.begin(); it != lowResPeaks.end(); ++it)
            v.push_back(make_pair(2, it->first));
        std::sort(v.begin(), v.end(), mzBoundingBox::PairSorter());

    }
};

template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
struct mzBoundingBoxPtrComp {

    bool operator()(const mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>* a, const mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>* b) {
        return a->mzmin() < b->mzmin();
    }
};


}
#endif
