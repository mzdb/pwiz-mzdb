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
 * @file data_points_collection.hpp
 * @brief Datastructure especially used for Thermo peak picking. A data points collection is constituted primarly by data points beteen to series of `0` in Thermo  raw files
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef DATAPOINTSCOLLECTION_HPP
#define DATAPOINTSCOLLECTION_HPP

#include <iterator>

#include "peak.hpp"
#include "boost/thread/thread.hpp"
#include "peak_finder_utils.hpp"


namespace mzdb {

/**
 *  DataPointsCollection class
 * =============================
 *
 * Datastructure especially used for Thermo peak picking. A data points collection
 * is constituted primarly by data points beteen to series of `0` in Thermo  raw files
 */
template<class mz_t, class int_t>
struct DataPointsCollection {

    typedef unique_ptr<mzPeak<mz_t, int_t> > PeakUPtr;

    /** associated pwiz spectrum */
    pwiz::msdata::SpectrumPtr spectrum;

    /** mz points constituting the dataPointsCollection */
    const vector<mz_t>& mzData;

    /** intensity points constituting the dataPointsCollection */
    const vector<int_t>& intData;

    /** peaks (unique_ptr) detected in this dataPointsCollection. Safely deleted when
     * datacollection is destructed*/
    vector<PeakUPtr> detectedPeaks;

private:

    /**
     * force to be a vector in template parameter
     * should be private
     */
    template<class type>
    vector<PeakUPtr>& _detectPeaks( const vector<type>& data, mzPeakFinderUtils::PeakPickerParams& params) {

        //---find all local maxima (meaning indexes of all points with a less intense point on the left and on the right)
        vector<mz_uint> maxIndexes;
        mzPeakFinderUtils::findLocalMaxima< vector<type> >( data, maxIndexes,  -1e9);
        if (maxIndexes.empty()) {
            return detectedPeaks;
        }

        //---find all surrounding minima around this maxima
        // for each value in maxIndexes, search in data the first point with a more intense point on the left and on the right
        // results are in peaksIndexes, each item contains .apex, .leftMin, .rightMin (all are indexes from data vector)
        vector<mzPeakFinderUtils::PeakAsMaximaMinimaIndexes> peaksIndexes;
        mzPeakFinderUtils::findLocalMinima< vector<type> >( maxIndexes, data, peaksIndexes );

        //---generate peak corresponding to all indexes
        for (auto it = peaksIndexes.begin(); it != peaksIndexes.end(); ++it)
            this->_createPeakForIndexes( *it, params);

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
            std::cout << "Empty stuff!"; //LOG(WARNING) 
        }

    }

    /**
      * @brief _createPeakForIndex
      * @param peakIndexes
      * @param params
      */
     void _createPeakForIndexes( mzPeakFinderUtils::PeakAsMaximaMinimaIndexes& peakIndexes, mzPeakFinderUtils::PeakPickerParams& params) {

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
            //LOG(WARNING) << "Do not know how to handle one peak";
            return;
        } else {
            //---usual case
            mz_uint v = std::min<unsigned int>( (unsigned int)(peakIndexes.rightMin ),  this->mzData.size()-1) + 1; //TODO check size() - 1 if it does not bug

            mz_uint v_2 = std::max<unsigned int>( (unsigned int)(peakIndexes.leftMin),  0 ); //TODO check size() - 1 if it does not bug

            if (v <= v_2) {
                return;
            }

            const vector<mz_t> mzp( this->mzData.begin() + v_2, this->mzData.begin() + v );
            const vector<int_t> intp( this->intData.begin() + v_2, this->intData.begin() + v );
            
            this->_addPeak(mzp, intp, params);

        }
    }
    
    template<class type>
    void _setDetectedPeaks(vector<std::shared_ptr<Centroid<mz_t, int_t> > >&givenCentroids, const vector<type>& adaptedIntData, mzPeakFinderUtils::PeakPickerParams& params) {

        // find in mzData the ids matching the given centroids (or the closest ones)
        // givenCentroids should contain only a few items, and all items should have a mz within mzData
        vector<mz_uint> maxIndexes;
        size_t j = 1; // start at item 2
        for(size_t i = 0; i < givenCentroids.size(); i++) {
            std::shared_ptr<Centroid<mz_t, int_t> > c = givenCentroids[i];
            while(j < mzData.size()) {
                if(c->mz >= mzData[j-1] && c->mz <= mzData[j]) {
                    // push the closest item
                    if(c->mz - mzData[j-1] > mzData[j] - c->mz) {
                        maxIndexes.push_back(j);
                    } else {
                        maxIndexes.push_back(j-1);
                    }
                    break;
                }
                j++;
            }
        }
        if (maxIndexes.empty()) {
            //LOG(WARNING) << "_setDetectedPeaks failed on centroid at mz " << givenCentroids[0]->mz;
            //for(size_t i = 0; i < mzData.size(); i++) LOG(WARNING) << "ABU profile data for centroid " << givenCentroids[0]->mz << ":\t" << mzData[i] << "\t" << intData[i];
            return;
        }

        //---find all surrounding minima around this maxima
        // for each value in maxIndexes, search in adaptedIntData the first point with a more intense point on the left and on the right
        // results are in peaksIndexes, each item contains .apex, .leftMin, .rightMin (all are indexes from adaptedIntData vector)
        vector<mzPeakFinderUtils::PeakAsMaximaMinimaIndexes> peaksIndexes;
        mzPeakFinderUtils::findLocalMinima< vector<type> >( maxIndexes, adaptedIntData, peaksIndexes );

        //---generate peak corresponding to all indexes
        for (auto it = peaksIndexes.begin(); it != peaksIndexes.end(); ++it)
            this->_createPeakForIndexes( *it, params);
        
    }
    
    void _peaksToFittedCentroids(vector<std::shared_ptr<Centroid<mz_t, int_t> > >&centroids) {
        for(auto it = this->detectedPeaks.begin(); it != this->detectedPeaks.end(); ++it) {
            try {
                PeakUPtr& peak = *it;
                auto c = peak->_computeFittedCentroid();
                centroids.push_back(c);
            } catch (exception& e) {
				std::cerr << e.what(); //LOG(ERROR)
            }
        }
    }

    void _peaksToCentroids(vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids) {
        std::for_each(this->detectedPeaks.begin(), this->detectedPeaks.end(), [&centroids](PeakUPtr& peak) {
            try {
                auto c = peak->_computeCentroid();
                centroids.push_back(c);
            } catch (exception& e) {
				std::cerr  << e.what(); //LOG(ERROR)
            }
        });
    }

public:

    /**
     * @brief isComputed
     * @return
     */
    inline bool isComputed() const{ return !detectedPeaks.empty(); }
    
    /**
     * @brief
     * Takes this->intData and extract the peaks from it:
     * example: if intData[0, 10, 50, 40, 90, 20, 50], then we get [(0, 3), (3, 5), (5, 6)] (as list of indexes)
     * Then create a mzPeak for each peak (ex: with mzData[0->3], intData[0->3] and spectrum reference)
     * All mzPeaks objects are added to detectedPeaks
     * Peaks in detectedPeaks contains exactly one local maxima (no more no less !)
     * @return detectedPeaks
     */
    vector<PeakUPtr>& detectPeaks(mzPeakFinderUtils::PeakPickerParams& params, mzPeakFinderUtils::USE_CWT opt) {
        if (opt == mzPeakFinderUtils::CWT_ENABLED)
            return _detectPeaksCWT(params);
        else if (opt == mzPeakFinderUtils::CWT_DISABLED)
            return _detectPeaksCLASSIC(params);
        else {
//            LOG(ERROR) <<"Wrong cwt use ...";
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
     * @param givenCentroids the list of precalculated centroids within the range of mzData (intensity shoulb be close to the highest value in intData)
     */
    void setDetectedPeaks(vector<std::shared_ptr<Centroid<mz_t, int_t> > > &givenCentroids, mzPeakFinderUtils::PeakPickerParams& params, mzPeakFinderUtils::USE_CWT opt) {
        if (opt == mzPeakFinderUtils::CWT_ENABLED) {
            vector<double> cwtCoeffs;
            mzPeakFinderUtils::cwt<int_t>( this->intData, params.fwhm, cwtCoeffs);
            _setDetectedPeaks<double>(givenCentroids, cwtCoeffs, params);
        } else {
            _setDetectedPeaks<int_t>(givenCentroids, intData, params);
        }
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
     * First, calls _peaksToCentroids:
     *  For each peak in detectedPeaks compute centroid and puts the result in an empty vector
     *  Computing centroid means generating a curve based on the peak's datapoints
     *  And then using mathematical functions to retrieve the apex and the left and right fwhm
     *  FIXME: peaks can be fitted, then optimized with ceres (that will recalculate fwhm !)
     */
    void optimize(
        vector<std::shared_ptr<Centroid<mz_t, int_t> > >&optimizedCentroids,
        unsigned char optimizationOpt,
        bool computeFWHM = true
    ) {
        if (this->detectedPeaks.empty() ) {
            //LOG(WARNING) << "no peaks detected, optimization is skipped";
            return;
        }
        
        // Convert detectedpeaks into inputCentroids
        vector<std::shared_ptr<Centroid<mz_t, int_t> > > inputCentroids;
        if(computeFWHM)
            this->_peaksToFittedCentroids(inputCentroids);
        else
            this->_peaksToCentroids(inputCentroids);

        //--- no optimization requested, much faster with relatively good result
        if ( optimizationOpt == mzPeakFinderUtils::NO_OPTIMIZATION) {

			for (size_t i = 0; i < inputCentroids.size(); ++i) {
				optimizedCentroids.push_back(inputCentroids[i]);
			}

            //optimizedCentroids = inputCentroids; // this code was erasing previous values in optimizedCentroids
        }
        //--- optimize all detected peak's width and intensity
        else if ( optimizationOpt == mzPeakFinderUtils::GAUSS_OPTIMIZATION ) {
            ProblemSolver<mz_t, int_t> problem(this->mzData, this->intData, inputCentroids);
            problem.solve(optimizedCentroids);
        } else {
           throw exception("Unknown optimization option ! \n");
        }
    }
    
    bool containsMz(mz_t mz) {
        if(mzData.size() == 0) return false;
        if(mzData[0] > mz) return false;
        if(mzData[mzData.size()] < mz) return false;
        return true;
    }

    /**
     * @brief DataPointsCollection constructor
     * @param mz_ Profile mz values
     * @param int_ Profile intensity values
     * @param s
     */
    DataPointsCollection(const vector<mz_t>& mz_, const vector<int_t>& int_, const pwiz::msdata::SpectrumPtr& s) :
        mzData(mz_), intData(int_), spectrum(s) {
    }
};

}
#endif // DATAPOINTSCOLLECTION_HPP
