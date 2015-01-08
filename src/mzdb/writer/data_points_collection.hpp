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

#ifndef DATAPOINTSCOLLECTION_HPP
#define DATAPOINTSCOLLECTION_HPP

#include <iterator>

#include "peak.hpp"
#include "boost/thread/thread.hpp"
#include "peak_finder_utils.hpp"


namespace mzdb {

template<class mz_t, class int_t>
struct DataPointsCollection {

    typedef unique_ptr<mzPeak<mz_t, int_t> > PeakUPtr;

    pwiz::msdata::SpectrumPtr spectrum;
    const vector<mz_t>& mzData;                                 //taken as references
    const vector<int_t>& intData;                               //taken as references
    vector<PeakUPtr> detectedPeaks;

private:
    /**
     * force to be a vector in template parameter
     * should be private
     */
    template<class type>
    vector<PeakUPtr>& _detectPeaks( const vector<type>& data, mzPeakFinderUtils::PeakPickerParams& params) {

        // --- no detection performed if max_element < threshold ---
        //LOG(INFO) << "baseline:" << params.baseline <<", noise :" << params.noise <<", minSNR: "<< params.minSNR;
        //if ( ( *std::max_element(data.begin(), data.end()) - params.baseline)  / params.noise < params.minSNR) {
        //    return this->detectedPeaks;
        //}

        //---find all local maxima
        vector<mz_uint> maxIndexes;
        mzPeakFinderUtils::findLocalMaxima< vector<type> >( data, maxIndexes,  -1e9);
        //printf("maxIndexes:%d\n", maxIndexes.size());
        //---find all surrounding minima around this maxima
        vector<mzPeakFinderUtils::PeakAsMaximaMinimaIndexes> peaksIndexes;
        mzPeakFinderUtils::findLocalMinima< vector<type> >( maxIndexes, data, peaksIndexes );

        //---generate peak corresponding to all indexes
        for (auto it = peaksIndexes.begin(); it != peaksIndexes.end(); ++it)
            this->_createPeakForIndexes( *it, params);

        //printf("peaks size:%d\n", detectedPeaks.size());
        return detectedPeaks;
    }


    /**
     * @brief _addPeak
     * @param params
     */
    void _addPeak( const vector<mz_t>&mzs, const vector<int_t>&ints, mzPeakFinderUtils::PeakPickerParams& params) {
        if ( ! mzs.empty() && ! ints.empty()) {
            if ( (*std::max_element(ints.begin(), ints.end()) -  params.baseline) / params.noise > params.minSNR) {
                PeakUPtr p(new mzPeak<mz_t, int_t>(mzs, ints, this->spectrum));
                this->detectedPeaks.push_back( std::move(p));
            }
        } else {
            printf("Empty stuff!");
        }

    }

    /**
      * @brief _createPeakForIndex
      * @param peakIndexes
      * @param params
      */
     void _createPeakForIndexes( mzPeakFinderUtils::PeakAsMaximaMinimaIndexes& peakIndexes, mzPeakFinderUtils::PeakPickerParams& params ) {


        if (peakIndexes.isMissingOnlyLeft()) {
            //---missing left part of the peak, building apex and rightMin
            mz_uint v = std::min<unsigned int>( (unsigned int)(peakIndexes.rightMin ),  mzData.size() + 1); //TODO check size() - 1 if it does not bug

            if ( v <= (unsigned int)peakIndexes.apex)
                return;

            const vector<mz_t> mzp( mzData.begin() + peakIndexes.apex, mzData.begin() + v );
            const vector<int_t> intp( intData.begin() + peakIndexes.apex, intData.begin() + v );

            this->_addPeak(mzp, intp, params);


        } else if ( peakIndexes.isMissingOnlyRight() ) {
            //---missing right part of the peak, building leftMin and apex
            mz_uint v = std::max<unsigned int>( (unsigned int)(peakIndexes.leftMin),  0 ); //TODO check size() - 1 if it does not bug

            mz_uint maxIndexPlusOne = std::min<unsigned int>( (unsigned int)(peakIndexes.apex + 1), mzData.size());

            if (maxIndexPlusOne <= v)
                return;

            const vector<mz_t> mzp( mzData.begin() + v, mzData.begin() + maxIndexPlusOne );
            const vector<int_t> intp( intData.begin() + v, intData.begin() + maxIndexPlusOne );

            this->_addPeak(mzp, intp, params);


        } else if (peakIndexes.isMissingBothLeftRight() ) {
            //---in that case dont know what to do
            // does not build a peak from these peak indexes
            printf("Do know how to handle one peak\n");
            return;
        } else {
            //---usual case
            mz_uint v = std::min<unsigned int>( (unsigned int)(peakIndexes.rightMin ),  this->mzData.size() + 1); //TODO check size() - 1 if it does not bug

            mz_uint v_2 = std::max<unsigned int>( (unsigned int)(peakIndexes.leftMin),  0 ); //TODO check size() - 1 if it does not bug

            if (v <= v_2) {
                return;
            }

            const vector<mz_t> mzp( this->mzData.begin() + v_2, this->mzData.begin() + v );
            const vector<int_t> intp( this->intData.begin() + v_2, this->intData.begin() + v );

            this->_addPeak(mzp, intp, params);

        }
    }

    /**
     * @brief _peaksToCentroids
     * @param centroids
     */
    void _peaksToCentroids(vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids) {
        std::for_each(this->detectedPeaks.begin(), this->detectedPeaks.end(), [&centroids](PeakUPtr& peak) {
            auto c = peak->_computeCentroid();
            centroids.push_back(c);
        });
    }

public:

    /**
     * @brief isComputed
     * @return
     */
    inline bool isComputed() const{ return !detectedPeaks.empty(); }

    vector<PeakUPtr>& detectPeaks(mzPeakFinderUtils::PeakPickerParams& params, mzPeakFinderUtils::USE_CWT opt) {
        if (opt == mzPeakFinderUtils::CWT_ENABLED)
            return _detectPeaksCWT(params);
        else if (opt == mzPeakFinderUtils::CWT_DISABLED)
            return _detectPeaksCLASSIC(params);
        else {
            LOG(ERROR) <<"Wrong cwt use ...";
            exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief detectPeaks
     * @return
     */
    vector<unique_ptr<mzPeak<mz_t, int_t> > >& _detectPeaksCLASSIC(mzPeakFinderUtils::PeakPickerParams& params) {
        return _detectPeaks<int_t>( this->intData, params );
    }


    /**
     * @brief detectPeaksCWT
     * @return
     */
    vector<unique_ptr<mzPeak<mz_t, int_t> > >& _detectPeaksCWT(mzPeakFinderUtils::PeakPickerParams& params) {
        vector<double> cwtCoeffs;
        //convert intData in double vector
        //forced to copy or need to write more code partial specialization
        mzPeakFinderUtils::cwt<int_t>( this->intData, params.fwhm, cwtCoeffs);
        return _detectPeaks<double> ( cwtCoeffs, params );
    }

    /**
     * @brief filterOnSNR
     * @param snr
     */
    void filterOnSNR( double snr ) {
        auto it = std::remove_if(detectedPeaks.begin(), detectedPeaks.end(), [](unique_ptr<mzPeak<mz_t, int_t> >& peak){
                peak.snr < snr;
            });
        std::for_each(it, this->detectedPeaks.end(), [](unique_ptr<mzPeak<mz_t, int_t> >& peak){
            auto* p = peak.release();
            delete p;
            peak = 0;
        });
         this->detectedPeaks.erase(it, this->detectedPeaks.end());
    }

    /**
     * @brief optimize
     * @param params
     */
    void optimize( vector<std::shared_ptr<Centroid<mz_t, int_t> > >&centroids, unsigned char optimizationOpt) {
        if ( this->detectedPeaks.empty() ) {
            printf("[optimize] : No peak detected !\n");
            return;
        }

        //--- optimize all detected peak 's  width and intensity
       if ( optimizationOpt & mzPeakFinderUtils::GAUSS_OPTIMIZATION ) {
           printf("Opt gauss\n");
            vector<std::shared_ptr<Centroid<mz_t, int_t> > > cs;

            this->_peaksToCentroids(cs);
            ProblemSolver<mz_t, int_t> problem(this->mzData, this->intData, cs);
            problem.solve(centroids);

        //--- no optimization requested, much faster with relatively good result
        } else if ( optimizationOpt == mzPeakFinderUtils::NO_OPTIMIZATION) {
           this->_peaksToCentroids(centroids);

        } else {
           throw exception("Unknown optimization option ! \n");
        }

    }


    /**
     * @brief DataPointsCollection constructor
     * @param mz_
     * @param int_
     * @param s
     */
    DataPointsCollection(const vector<mz_t>& mz_, const vector<int_t>& int_, const pwiz::msdata::SpectrumPtr& s) :
        mzData(mz_), intData(int_), spectrum(s){
    }
};

}
#endif // DATAPOINTSCOLLECTION_HPP
