#ifndef MZABSTRACTPEAKFINDER_H
#define MZABSTRACTPEAKFINDER_H

#include <vector>
#include <cmath>

#include "../../utils/mzUtils.hpp"
#include "peak.hpp"
#include "ceres_problems.hpp"
#include "../../utils/cwtlib/cwtlib"


__BEGIN_MZDB_NM

USE_STD_NM

namespace mzPeakFinderUtils {


#define TOF_RES_PPM 20.0
#define ORBI_RES_PPM 10.0

#define MAGIC 1.252


#define TOF_FWHM 0.023961661341853//0.015024


/**
 * @brief The OPTIONS enum
 */
enum OPTIMIZATION_OPTIONS {
    NO_OPTIMIZATION = 0x01,
    GAUSS_OPTIMIZATION = 0x02,
    PARABOLA_OPTIMIZATION = 0x04
};

enum USE_CWT {
    CWT_DISABLED=0,
    CWT_ENABLED
};

/**
 * @brief The FIT_OPTIONS enum
 */
enum FIT_OPTIONS {
    ONE_BY_ONE = 0x08,
    ALL = 0x10,
    COMPLEX = 0x20
};


#define wiff_default_minSNR 0
#define wiff_default_baseline 0
#define wiff_default_noise 0
#define wiff_default_fwhm  0


#define thermo_default_minSNR



/**
 * @brief The PeakPickerParams struct
 */

struct PeakPickerParams {
    double baseline, noise, minSNR, fwhm;
    unsigned char optimizationOpt;
    bool adaptiveBaselineAndNoise;

    PeakPickerParams(double minSNR, double fwhm, double baseline, double noise, unsigned char optimizationOpt):
        minSNR(minSNR), fwhm(fwhm), baseline(baseline), noise(noise), optimizationOpt(optimizationOpt), adaptiveBaselineAndNoise(false) {}

    PeakPickerParams(): minSNR(0), fwhm(0), baseline(0), noise(0), optimizationOpt(0x01), adaptiveBaselineAndNoise(false) {}

    bool isEmpty() { return !baseline && !noise; }

};



/**
 * @brief The PeakAsMaximaMinimaIndexes struct
 */
struct PeakAsMaximaMinimaIndexes {

    int leftMin, rightMin, apex;

    PeakAsMaximaMinimaIndexes(mz_uint cmax) : apex(cmax), leftMin(-1), rightMin(-1) {}

    PeakAsMaximaMinimaIndexes(mz_uint cmax, mz_uint lmin, mz_uint rmin):
        apex(cmax), leftMin(lmin), rightMin(rmin) {}

    PeakAsMaximaMinimaIndexes(): apex(-1), leftMin(-1), rightMin(-1) {}

    bool isMissingOnlyLeft() { return leftMin < 0 && rightMin >= 0; }

    bool isMissingOnlyRight() { return rightMin < 0 && leftMin >= 0; }

    bool isMissingBothLeftRight() { return rightMin < 0 && leftMin < 0; }
};



/**
 * @brief isPowerOf2
 * @param n
 * @return
 */
static inline bool isPowerOf2 (mz_uint n) {
    if (n == 0 || n == 1 || n == 2)
        return false;
    //((n & -n) == n);
    if (!((n - 1) & n))
        return true;
    return false;
}

/**
 * @brief nextPowerOf2
 * @param n
 * @return
 */
static mz_uint nextPowerOf2(mz_uint n) {
    mz_uint b = 1;
    while (b < n) {
      b = b << 1;
    }
    if (b == 1 || b == 2)
      b = 4;
    return b;
}

/**
 * @brief makePowerOf2
 * @param ints
 */
static void makePowerOf2(vector<double>& ints) {
    mz_uint n = ints.size();
    if ( ! isPowerOf2( n ) ) {
        mz_uint new_s = nextPowerOf2( n );
        while ( ints.size() != new_s ) {
            ints.push_back( 0.0 );
        }
    }
}





//TODO test if performance are better with iterators
template<class VEC>
static void findLocalMinima ( const VEC& intData, vector<mz_uint>& minIndexes) {
    for (size_t i = 0; i < intData.size(); ++i) {
        if ( i == 0) {
            if (intData[i + 1] > intData[i])
                minIndexes.push_back(i);
        } else if ( i == intData.size() - 1) {
            if ( intData[i - 1] > intData[i] )
                minIndexes.push_back(i);
        } else {
            if (intData[i - 1] > intData[i] && intData[i + 1] > intData[i])
                minIndexes.push_back(i);
        }
    }
}

/**
 *
 */
template<class VEC>
static void findLocalMaxima ( const VEC& intData, vector<mz_uint>& maxIndexes, float threshold) {
    for (size_t i = 0; i < intData.size(); ++i) {
        if ( i == 0) {
            if (intData[i + 1] < intData[i] && intData[i] > threshold)
                maxIndexes.push_back(i);
        } else if ( i == intData.size() - 1 && intData[i] > threshold) {
            if ( intData[i - 1] < intData[i] )
                maxIndexes.push_back(i);
        } else {
            if (intData[i - 1] < intData[i] && intData[i + 1] < intData[i] && intData[i] > threshold)
                maxIndexes.push_back(i);
        }
    }
}


/**
 *
 */
template<class VEC>
static void findLocalMinima( const vector<mz_uint>& maxIndexes, const VEC& data, vector<PeakAsMaximaMinimaIndexes>& peaks ) {

    for (auto it = maxIndexes.begin(); it != maxIndexes.end(); ++it ) {
        mz_uint maxIndex =  *it;

        //---leftval can not be less than 0, data never empty
        mz_uint leftVal = std::max<unsigned int>(maxIndex - 1, 0);

        int i = maxIndex - 2;
        while ( i >= 0 ){
            if ( data[i] > data[leftVal] ) {
                break;
            }
            leftVal = i;
            i--;
        }

        //---rightval can not be greater than the data size less 1
        mz_uint rightVal = std::min<unsigned int>(maxIndex + 1, data.size() - 1);

        i = maxIndex + 2;
        while( i < data.size() ) {
            if ( data[i] > data[rightVal] ) {
                break;
            }
            rightVal = i;
            i++;
        }


        PeakAsMaximaMinimaIndexes peakAsIndexes(maxIndex);
        if (leftVal != maxIndex)
            peakAsIndexes.leftMin = leftVal;
        if ( rightVal != maxIndex )
            peakAsIndexes.rightMin = rightVal;

        //printf("%d, %d, %d, %d\n", maxIndex, leftVal, rightVal, data.sie()  );
        peaks.push_back(peakAsIndexes);
    }
}



/**
 *
 */
template<class mz_t, class int_t>
static void optimizeCeres(const vector<double>& mzData,
                          const vector<double>& intData,
                          vector<mzPeak<mz_t, int_t>*>& peaks,
                          vector<Centroid<mz_t, int_t>*>& centroids,
                          byte op = NO_OPTIMIZATION) {
    if ( ! peaks.empty() ) {
        /* optimize all detected peak 's  width and intensity */
       if ( op & GAUSS_OPTIMIZATION ) {
           //printf("gauss optimization");
            vector<Centroid<mz_t, int_t>*> cs;
            for (size_t k = 0; k < peaks.size(); ++k) {
                Centroid<mz_t, int_t>* c = peaks[k]->_computeCentroid();
                if (! c)
                    printf("[findPeak Thermo] : null pointer\n");
                cs.push_back(c);
            }
            ProblemSolver<mz_t, int_t> problem(mzData, intData, cs);
            problem.solve(centroids);
            //no optimization requested, much faster with relatively good result
        } else if ( op & NO_OPTIMIZATION) {
            for (size_t k = 0; k < peaks.size(); ++k) {
                Centroid<mz_t, int_t>* c = peaks[k]->_computeCentroid();
                if (c)
                    centroids.push_back(c);
            }
        } else {
           throw exception("Unknown optimization option ! \n");
        }
    }

}


/**
 * @brief getBaselineAndNoise
 * @param ints
 * @return
 */
static const pair<double, double> getBaselineAndNoise( const vector<double>& ints )  {
    if ( ints.empty() )
        throw exception("[getBaselineAndNoise]::mzPeakFinderUtils: not critical but will fail");

    //--- copy input...expensive !
    vector<double> sortedInts(ints.begin(), ints.end());
    //--- sort intensities
    std::sort(sortedInts.begin(), sortedInts.end());
    //--- remove zero intensities ?
    sortedInts.erase(std::remove_if(sortedInts.begin(), sortedInts.end(), [](double& p) { return ! p; }), sortedInts.end());

    //---trim
    const mz_uint n = sortedInts.size();
    const mz_uint toBeTrimmed = ceil( 0.1 * n );

    const vector<double> trimmedInts(sortedInts.begin() + toBeTrimmed, sortedInts.end() - toBeTrimmed);


    mz_uint n2 = trimmedInts.size();
    double baseline=0.0, noise=0.0;

    std::for_each(trimmedInts.begin(), trimmedInts.end(), [&baseline](const double& d) {
        baseline += d;}
    );

    baseline /= n2;

    std::for_each(trimmedInts.begin(), trimmedInts.end(), [&baseline, &noise](const double& d) {
        noise += (d - baseline) * (d - baseline); }
    );

    noise /= n2;


    return make_pair(baseline, noise);
}


/**
 * @brief cwt
 * @param intensities
 * @param fwhm
 * @param result
 */
template<class type>
static void cwt(const vector<type>& intensities, double& fwhm, vector<double>& result) {

    if (intensities.empty()) {
        printf("Empty spectrum no chance !");
        exit(0);
    }

    //make a copy of size power of 2
    vector<double> ints(intensities.begin(), intensities.end());
    makePowerOf2(ints);
    //printf("Size of intensities:%d, %d\n", intensities.size(), ints.size());

    //double scale = MAGIC * fwhm;
    mz_uint N = ints.size();
    cwtlib::WTransform *wt;

    try {
        cwtlib::Signal s( N, &ints[0] );
        cwtlib::LinearRangeFunctor scales( fwhm, 3 * fwhm, 2 * fwhm); //perform only  at one scale ? ->to test
        wt = cwtlib::CWTalgorithm::cwtft(s, scales, cwtlib::MexicanHat(), "Youpi");
    } catch(exception& e) {
        printf("%s\n", e.what());
    }

    //fill result
    //result.reserve( intensities.size() );
    for (mz_uint i = 0, l=intensities.size(); i < l; ++i) {
        result.push_back(wt->re(0, i));
    }
    delete wt;
}



/**
 * force to be a vector in template parameter
 * should be private
 */
template<class mz_t, class int_t, class type>
static Ownership detectPeaks( const vector<type>& intData,
                               vector<mzPeak<mz_t, int_t>* > peaks,
                               const PeakPickerParams& p) {

    DCHECK( peaks.empty() );

    // ---perform  not detection if max_element < threshold ---
    // ---assume peaks vector is empty
    if ( (*std::max_element( intData.begin(), intData.end()) - p.baseline) / p.noise < p.minSNR ) {
        return TAKE_OWNERSHIP;
    }

    //---find all local maxima
    vector<mz_uint> maxIndexes;
    mzPeakFinderUtils::findLocalMaxima< vector<type> >( intData, maxIndexes, p.baseline );

    //---find all surrounding minima around this maxima
    vector<mzPeakFinderUtils::PeakAsMaximaMinimaIndexes> peakIndexes;
    try {
        mzPeakFinderUtils::findLocalMinima< vector<type> >( maxIndexes, intData, peakIndexes );
    } catch (exception& e) {
        LOG(FATAL) << e.what();
    }

    //---generate peak corresponding to all indexes
    for (auto it = peakIndexes.begin(); it != peakIndexes.end(); ++it ) {
        createPeakForIndex(*it, peaks);
    }


    return TAKE_OWNERSHIP;
}



/**
 * @brief detectPeaks
 * @return
 */
template<class mz_t, class int_t>
static Ownership detectPeaksCLASSIC(const vector<double>& intData,
                                    vector<mzPeak<mz_t, int_t>*>& peaks,
                                    const PeakPickerParams& p ) {
    return detectPeaks<mz_t, int_t, double>( intData, peaks, p );
}


/**
 * @brief detectPeaksCWT
 * @return
 */
/*template<class mz_t, class int_t>
static Ownership detectPeaksCWT(const vector<double>& intData,
                                vector<mzPeak<mz_t, int_t>*>& peaks,
                                const PeakPickerParams& p) {

    vector<double> cwtCoeffs;
    //convert intData in double vector
    //forced to copy or need to write more code partial specialization
    mzPeakFinderUtils::cwt(vector<double>( intData.begin(), intData.end() ), p.fwhm, cwtCoeffs);
    return detectPeaks<mz_t, int_t, double> ( cwtCoeffs, peaks, p );


}*/


}//end peakpicking nm



__END_MZDB_NM




#endif // MZABSTRACTPEAKFINDER_H
