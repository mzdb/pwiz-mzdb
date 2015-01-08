#ifndef MZBOUNDINGBOXCOMPUTER_HPP
#define MZBOUNDINGBOXCOMPUTER_HPP

#include <algorithm>

#include "peak.hpp"
#include "spectrum.hpp"
#include "bb.hpp"

using namespace std;

namespace mzdb {

namespace BBComputer {

//---------------------------- BOUNDING BOX INSERTIONS ---------------------
/**
 * build Map key scanID value vector of centroids
 * @param pt
 * @param vt
 * @param dataModes
 */
template<typename mz_t, typename int_t>
static void buildCentroidsByScanID(map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > >& centroidsByScanId,
                                   const vector<std::shared_ptr<mzSpectrum<mz_t, int_t> > >& spectra,
                                   map<int, DataMode>& dataModes) {

    for_each(spectra.begin(), spectra.end(), [&centroidsByScanId, &dataModes]( std::shared_ptr<mzSpectrum<mz_t, int_t> > spectrum) {
        const int& id = spectrum->id;
        //move semantics to avoid copy, leads to bug ?
        centroidsByScanId[id] = std::move(spectrum->peaks);
        dataModes[id] = spectrum->effectiveMode;
    });
}


/**
 * Caculation of the runSlice index for each peak it belongs to
 * @param v
 * @param x
 * @param scanIdx
 * @param bbheight
 */
template<class mz_t, class int_t>
static void groupByMzIndex(vector<std::shared_ptr<Centroid<mz_t, int_t> > >& v, //correspond to the entire centroid of one spectrum,
                           map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<mz_t, int_t> > > > > >& x, //run Slice
                           int scanIdx, double& bbheight) {

    typedef std::shared_ptr<Centroid<mz_t, int_t> > CentroidSPtr;

    for_each(v.begin(), v.end(), [&x, &scanIdx, &bbheight](CentroidSPtr& peak) {
        const int idx = peak->mz * bbheight;
        //not found
        if (x.find(idx) == x.end()) {
            auto m = unique_ptr<map<int, vector<CentroidSPtr> > >(new map<int, vector<CentroidSPtr> >);
            x[idx] = move(m);
        }
        (*(x[idx]))[scanIdx].push_back(peak);
    });
}


/**
 * compute BBs given a MapHolder
 * @param bbheight
 * @param bbs: bbs output
 * @param hrs: map<int, map<int, vector<mzPeak> > > runSliceidx / scanidx, peaks(hr) contained in this scan
 * @see MapHolder
 */
template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
static void computeBoundingBox(double& bbheight,
                               map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > >& highResPeaksByScanIDs,
                               map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > >& lowResPeaksByScanIDs,
                               vector<unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > >& bbs, // output
                               map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<h_mz_t, h_int_t> > > > > >& hrs,
                               map<int, unique_ptr<map<int, vector<std::shared_ptr<Centroid<l_mz_t, l_int_t> > > > > >& lrs ) {

    typedef std::shared_ptr<Centroid<h_mz_t, h_int_t> > HighResCentroidSPtr;
    typedef std::shared_ptr<Centroid<l_mz_t, l_int_t> > LowResCentroidSPtr;
    typedef unique_ptr<mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t> > mzBoundingBoxUPtr;
    //group all peaks by their runSliceId

    vector<pair<int, int> > v;
    mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>::iterationOrder(highResPeaksByScanIDs, lowResPeaksByScanIDs, v);
    for (size_t i = 0; i < v.size(); ++i) {
        pair<int, int>& p = v[i];
        int scanIdx = p.second;
        if (p.first == 1) {
            groupByMzIndex<h_mz_t, h_int_t>( highResPeaksByScanIDs[scanIdx], hrs, scanIdx, bbheight );
        } else {
            groupByMzIndex<l_mz_t, l_int_t>( lowResPeaksByScanIDs[scanIdx], lrs, scanIdx, bbheight );
        }
    }

    vector<int> treatedRunSliceIdx;

    //retrieve all bounding box belonging to the same runSlice idx
    for (auto it = hrs.begin(); it != hrs.end(); ++it) {
        const int& runSliceIdx = it->first;
        auto lp = unique_ptr<map<int, vector<LowResCentroidSPtr> > >(new map<int, vector<LowResCentroidSPtr> >);
        if (lrs.find(runSliceIdx) != lrs.end()) {
            for (auto iit = lrs[runSliceIdx]->begin(); iit != lrs[runSliceIdx]->end(); ++iit) {
                (*lp)[iit->first] = std::move(iit->second);
            }
            treatedRunSliceIdx.push_back(runSliceIdx);
        }
        auto bb = mzBoundingBoxUPtr(
                    new mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(runSliceIdx, 1.0/bbheight, it->second, lp));
        bb->update(highResPeaksByScanIDs, lowResPeaksByScanIDs);
        if (! bb->isEmpty()) {
            //bb->computeMzBounds();
            bbs.push_back(move(bb));
        }
    }

    for (auto it = lrs.begin(); it != lrs.end(); ++it) {
        const int& runSliceIdx = it->first;
        if (find(treatedRunSliceIdx.begin(), treatedRunSliceIdx.end(), runSliceIdx) == treatedRunSliceIdx.end()) {
            auto hp = unique_ptr<map<int, vector<HighResCentroidSPtr> > >(new map<int, vector<HighResCentroidSPtr> >);
            auto bb = mzBoundingBoxUPtr(
                        new mzBoundingBox<h_mz_t, h_int_t, l_mz_t, l_int_t>(runSliceIdx, 1.0/bbheight, hp, it->second));
            bb->update(highResPeaksByScanIDs, lowResPeaksByScanIDs);
            if (! bb->isEmpty()) {
                //bb->computeMzBounds();
                bbs.push_back(move(bb));
            }
        }
    }
    //seems to be unuseful
    //sort selon mz begin implementation of operator <
    //sort(bbs.begin(), bbs.end(), mzBoundingBoxPtrComp<h_mz_t, h_int_t>());
}
}

}


#endif // MZBOUNDINGBOXCOMPUTER_HPP
