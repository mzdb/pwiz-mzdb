#ifndef DATAPOINTSCOLLECTION_HPP
#define DATAPOINTSCOLLECTION_HPP

#include <iterator>


#include "types.h"
#include "mzPeak.hpp"
#include "boost/thread/thread.hpp"
#include "../peak_picking/mzPeakFinderUtils.hpp"

#define INTENSITY_CUT_OFF 10000



namespace mzdb {

template<class mz_t, class int_t>
struct DataPointsCollection {

    pwiz::msdata::SpectrumPtr spectrum;
    const vector<mz_t>& mzData; //taken as references
    const vector<int_t>& intData; //taken as references
    vector<unique_ptr<mzPeak<mz_t, int_t> > > detectedPeaks;

private:
    /**
     * force to be a vector in template parameter
     * should be private
     */
    template<class type>
    vector<unique_ptr<mzPeak<mz_t, int_t> > >& _detectPeaks( const vector<type>& data, mzPeakFinderUtils::PeakPickerParams& params) {

        // --- no detection performed if max_element < threshold ---
        //LOG(DEBUG) << "baseline:" << params.baseline <<", noise :" << params.noise <<", minSNR: "<< params.minSNR;
        if ( ( *std::max_element(data.begin(), data.end()) - params.baseline)  / params.noise < params.minSNR) {
            return this->detectedPeaks;
        }

        //---find all local maxima
        vector<mz_uint> maxIndexes;
        mzPeakFinderUtils::findLocalMaxima< vector<type> >( data, maxIndexes, params.baseline );

        //---find all surrounding minima around this maxima
        vector<mzPeakFinderUtils::PeakAsMaximaMinimaIndexes> peaksIndexes;
        //try {
        mzPeakFinderUtils::findLocalMinima< vector<type> >( maxIndexes, data, peaksIndexes );
        /*} catch (exception& e) {
            LOG(ERROR) << e.what() << endl;
        }*/
        //LOG(INFO) << "maxIndexes size: "<< maxIndexes.size() << ", nbPeaksMaxMinIndexes: "<< peakIndexes.size();

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
                //std::unique_ptr<mzPeak<mz_t, int_t> > p(mzs, ints, this->spectrum);
                this->detectedPeaks.push_back( unique_ptr<mzPeak<mz_t, int_t> >(new mzPeak<mz_t, int_t>(mzs, ints, this->spectrum)));
            }
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
            return;
        } else {
            //---usual case
            mz_uint v = std::min<unsigned int>( (unsigned int)(peakIndexes.rightMin ),  this->mzData.size() + 1); //TODO check size() - 1 if it does not bug

            mz_uint v_2 = std::max<unsigned int>( (unsigned int)(peakIndexes.leftMin),  0 ); //TODO check size() - 1 if it does not bug

            if (v <= v_2)
                return;

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
        std::for_each(this->detectedPeaks.begin(), this->detectedPeaks.end(), [&centroids](unique_ptr<mzPeak<mz_t, int_t> >& peak) {
            /*
            auto* c = peak->_computeCentroid();
            if (c)
                centroids.push_back(std::shared_ptr<Centroid<mz_t, int_t> >(c));
            */
            centroids.push_back(peak->_computeCentroid());
        });
    }

public:

    /**
     * @brief isComputed
     * @return
     */
    inline bool isComputed() const{ return !detectedPeaks.empty(); }

    vector<unique_ptr<mzPeak<mz_t, int_t> > >& detectPeaks(mzPeakFinderUtils::PeakPickerParams& params, mzPeakFinderUtils::USE_CWT opt) {
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
        auto it = std::remove_if(peaks.begin(), peaks.end(), [](unique_ptr<mzPeak<mz_t, int_t> >& peak){
                peak.snr < snr;
            });
        std::for_each(it, this->detectedPeaks.end(), [](unique_ptr<mzPeak<mz_t, int_t> >& peak){
            delete peak;
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
            return;
        }

        //--- optimize all detected peak 's  width and intensity
       if ( optimizationOpt & mzPeakFinderUtils::GAUSS_OPTIMIZATION ) {
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

    /**
     * @brief ~DataPointsCollection
     */
    /*inline ~DataPointsCollection() {
        for (size_t  i = 0; i < detectedPeaks.size(); ++i)
            delete detectedPeaks[i];
    }*/

};

}
#endif // DATAPOINTSCOLLECTION_HPP
