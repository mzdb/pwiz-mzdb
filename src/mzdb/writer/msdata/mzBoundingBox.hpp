
/*
BoundingBox class
use to store minmz, maxmz, minrt, maxrt
 */
#ifndef __BB__
#define __BB__

#include <algorithm>
#include <unordered_map>
#include "pwiz/data/msdata/MSData.hpp"

#include "types.h"
//#include "mzBlobHandler.hpp"
#include "../../utils/mzUtils.hpp"

namespace mzdb {
using namespace std;

template<class mz_t, class int_t>
class PWIZ_API_DECL mzBoundingBox {

protected:

    //both map can be null
    map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& _peaksByScanIDs;

    Ownership _ownership;

    float _mzmin, _mzmax, _rtmin, _rtmax; //use for descriptive filed no precision needed
    int _runSliceIdx;


public:

    inline float mzmin() const throw() {
        return _mzmin;
    }

    inline float mzmax() const throw() {
        return _mzmax;
    }

    inline float rtmax() const throw() {
        return _rtmax;
    }

    inline float rtmin() const throw() {
        return _rtmin;
    }

    inline int runSliceIdx() const throw() {
        return _runSliceIdx;
    }

    inline void setRunSliceIdx(int idx) throw() {
        _runSliceIdx = idx;
    }

    inline bool isEmpty() const throw () {
        return _peaksByScanIDs.empty();
    }

    /** */
    inline mzBoundingBox( map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& h,
                          Ownership ownership_):
        _peaksByScanIDs(h), _runSliceIdx(0),
        _rtmin(0), _rtmax(0), _mzmin(0), _mzmax(0), _ownership(ownership_) {

    }

    /** */
    mzBoundingBox(int idx,
                  float height,
                  map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& h,
                  Ownership ownership_): _peaksByScanIDs(h), _ownership(ownership_) {

        _mzmin = idx * height;
        _mzmax = _mzmin + height;
        _runSliceIdx = idx;

        if ( ! _peaksByScanIDs.empty() ) {
            this->setRtBoundaries();
        }
    }

    /** */
    inline mzBoundingBox(float mmin, float mmax,
                         float rmin, float rmax,
                         map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& h,
                         Ownership ownership):
        _mzmin(mmin), _mzmax(mmax),
        _rtmin(rmin), _rtmax(rmax),
        _peaksByScanIDs(h),
        _runSliceIdx(0), _ownership(ownership) {}

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
     * constructed with the flag TAKE_OWNERSHIP
     * Else it is assumed that pointer will be automatically
     * be destroyed
     */
    /*inline ~mzBoundingBox() {
        if (_ownership == TAKE_OWNERSHIP) {
            //delete centroid (belongs to spectrum)
            for (auto it = _peaksByScanIDs.begin(); it != _peaksByScanIDs.end(); ++it) {
                for (auto it_ = it->second.begin(); it_ != it->second.end(); ++it_) {
                    delete (*it_);
                }
            }
        }
    }*/

    /**
     * @brief computeMzBounds
     */
    void computeMzBounds() {
        if (! _peaksByScanIDs.empty())
           this->setMzBoundaries();
    }




    template<class mz_t, class int_t>
    inline static int getByteLength(map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& m, map<int, DataMode>& dataModes) {
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


    int getBytesVectorLength(map<int, DataMode>& dataModes) const {
        int sum = 0;
        if ( ! _peaksByScanID.empty())
            sum += mzBoundingBox::getByteLength<mz_t, int_t>(_peaksByScanIDs, dataModes);
        return sum;
    }


    /**
     * @brief asByteArray
     * @param v
     * @param dataModes
     */

    void asByteArray(vector<byte>& v, map<int, DataMode>& dataModes) const {
        for (auto it = this->_peaksByScanIDs.begin(); it != this->_peaksByScanIDs.end(); ++it) {
            const int& idx = it->first;
            DataMode& dm = dataModes[idx];
            if (dm == FITTED)
                insertFittedToBinaryVector<mz_t, int_t>(v, idx, _peaksByScanIDs);
            else
                insertCentroidToBinaryVector<mz_t, int_t>(v, idx, _peaksByScanIDs);

        }
    }

    /**
     *TODOOOOOOOOOOOOOOOOOOOO
     */
    template<typename mz_t, typename int_t>
    inline static void insertCentroidToBinaryVector(vector<byte>& v, int idx, map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& w)  {
        auto& peaks = w[idx];
        put<int>(idx, v);
        put<int>(peaks.size(), v);
        for (size_t j = 0; j < peaks.size(); ++j) {
            Centroid<mz_t, int_t>& p_ = *(peaks[j]);
            put<float>(p_.mz, v);
            put<float>(p_.intensity, v);
        }
    }

    /**
     *TODOOOOOOOOOOOOOOOOOOOO
     */
    template<typename mz_t, typename int_t>
    inline static void insertFittedToBinaryVector(vector<byte>& v, int idx, map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& w) {
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

    /**
     * @brief update
     * @param mh
     */
    inline void update(map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& h) throw(){
        for (auto it = h.begin(); it != h.end(); ++it) {
            const int& p = it->first;
            if (_peaksByScanIDs.find(p) == _peaksByScanIDs.end()) {
                _peaksByScanIDs[p];
            }
        }
    }

    /** */
    inline void setRtBoundaries() {
        _rtmin = 1e9, _rtmax = 0;
        for (auto it = this->_peaksByScanIDs.begin(); it != _peaksByScanIDs.end(); ++it) {
            if ( it->second.empty())
                continue;
            Centroid<mz_t, int_t>& p = *(it->second.front());
            const float& t = p.rt;
            _rtmin = min(_rtmin, t);
            _rtmax = max(_rtmax, t);
        }
    }

    /** */
    inline void setMzBoundaries()  {
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
    }


};

template<class mz_t, class int_t>
struct mzBoundingBoxPtrComp {

    bool operator()(const mzBoundingBox<mz_t, int_t>* a, const mzBoundingBox<mz_t, int_t>* b) {
        return a->mzmin() < b->mzmin();
    }
};


}
#endif
