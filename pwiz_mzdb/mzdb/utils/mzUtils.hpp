#ifndef MZUTILS_HPP
#define MZUTILS_HPP

#include <iostream>
#include <math.h>
#include <set>


#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/analysis/peakdetect/PeakFamilyDetectorFT.hpp"
#include "pwiz/analysis/spectrum_processing/PrecursorRecalculatorDefault.hpp"

using namespace std;

namespace mzdb {



#define __BEGIN_MZDB_NM namespace mzdb {

#define __END_MZDB_NM }

#define USE_STD_NM using namespace std;

//
#define MS1_BB_MZ_WIDTH_STR "ms1_bb_mz_width"

#define MSN_BB_MZ_WIDTH_STR "msn_bb_mz_width"

#define MS1_BB_TIME_WIDTH_STR "ms1_bb_time_width"

#define MSN_BB_TIME_WIDTH_STR "msn_bb_time_width"

#define IS_LOSSLESS_STR "is_lossless"

#define ORIGIN_FILE_FORMAT_STR "origin_file_format"

//xml attributes
#define XML_FLOAT "xsd:float"

#define XML_STRING "xsd:string"

#define XML_DOUBLE "xsd:double"

#define XML_INT "xsd:int"

#define XML_BOOLEAN "xsd:boolean"

#define PARAM_NAME_STR "name"

#define PARAM_VALUE_STR "value"

#define PARAM_TYPE_STR "type"

#define MS_NS_STR "MS"

#define CV_REF_STR "cvRef"

#define ACCESS_NB_STR "accession"

#define PARAMS_STR "params"

#define CV_PARAMS_STR "cvParams"

#define CV_PARAM_STR "cvParam"

#define USER_PARAMS_STR "userParams"

#define USER_PARAM_STR "userParam"

#define USER_TEXTS_STR "userTexts"

#define USER_TEXT_STR "userText"

#define TRUE_STR "true"

#define FALSE_STR "false"

#define EMPTY_STR "empty"

#define CID_STR "CID"

#define ETD_STR "ETD"

#define HCD_STR "HCD"

#define UNKNOWN_STR "unknown"

#define IN_HIGH_RES_STR "in_high_res"

#define THERMO_TRAILER "[Thermo Trailer Extra]Monoisotopic M/Z:"

#define THERMO_DATA_PROC "pwiz_mzdb_thermo_conversion"

#define ABI_DATA_PROC "pwiz_mzdb_abi_conversion"

#define ABI_SWATH_DATA_PROC "pwiz_mzdb_abi_swath_conversion"

#define XML_DATA_PROC "pwiz_mzdb_xml_conversion"


#define _64_BIT_MZ "64_bit_float_mz"

#define _64_BIT_INTENSITY "64_bit_float_intensity"

#define _32_BIT_MZ "32_bit_float_mz"

#define _32_BIT_INTENSITY "32_bit_float_intensity"

#define PROFILE_STR "profile"


/*functions*/
#define PPM2MZ(mz, ppm) mz * ppm / 1e6

#define MZ2PPM(mz, ppm) mz * 1e6 / ppm;


/**useful typedef */
typedef unsigned char byte;
typedef unsigned int mz_uint;


#define DELETE_IF_HAS_OWNERSHIP(ownership, vec)  \
    if ( ownership == TAKE_OWNERSHIP) { \
    for (auto it= vec.begin(); it != vec.end(); ++it) { \
    delete *it; \
} \
}


/// Simulating ownership of naked pointer
enum Ownership {
    TAKE_OWNERSHIP,
    DO_NOT_TAKE_OWNERSHIP
};


/// Enumeration which value equals to the number of bytes
/// in the encoding mode
enum DataMode {
    PROFILE = 1,
    CENTROID = 12,
    FITTED = 20
};


enum PeakEncoding {
    LOW_RES_PEAK = 8,
    HIGH_RES_PEAK = 12,
    NO_LOSS_PEAK = 16
};


/// Wrapper of PeakEncoding and DataMode
struct DataEncoding {
    int id;
    DataMode mode;
    PeakEncoding peakEncoding;

    DataEncoding(int id_, DataMode mode_, PeakEncoding pe_) : id(id), mode(mode_), peakEncoding(pe_) {}
    DataEncoding(){}
};


/// put data into a vector of bytes
/// To be more effective the vector should have some reserved space
template <typename T>
inline static void put(T data, vector<byte>& v) {
    unsigned int wpos_ = v.size();
    unsigned int s = sizeof (data);
    if (v.size() < (wpos_ + s))
        v.resize(wpos_ + s);
    memcpy(&v[wpos_], (byte*) & data, s);
}

/// getting data from the bytebuffer
template<typename T>
inline static T get(unsigned int index, vector<byte> &buffer) {
    return *((T*) & buffer[index]);
}


template<typename T>
inline static bool isInMapKeys(int value, map<int, T>& m) {
    typename map<int, T>::iterator it;
    it = m.find(value);
    if (it == m.end())
        return false; //not found
    return true; //found
}

/// more generic version than the previous one
template <typename T> static bool isInMapKeys(typename T::key_type value, T& m) {
    typename T::iterator it;
    it = m.find(value);
    if (it == m.end())
        return false;
    return true;
}


namespace PwizHelper {

inline static float rtOf(const pwiz::msdata::SpectrumPtr& s) {
    pwiz::msdata::Scan* scan = !s->scanList.empty() ? &s->scanList.scans[0] : 0;
    if (! scan) {
        printf("\nCan not find RT of one spectrum !\n");
        return 0;
    }
    pwiz::msdata::CVParam scanTimeParam = scan ? scan->cvParam(pwiz::msdata::MS_scan_start_time) : pwiz::msdata::CVParam();
    return static_cast<float> (scanTimeParam.timeInSeconds());
}

inline static int precursorChargeOf(const pwiz::msdata::SpectrumPtr &s) {
    const pwiz::msdata::SelectedIon& si = s->precursors.front().selectedIons.front();
    return si.cvParam(pwiz::msdata::MS_charge_state).valueAs<int>();
}

inline static int precursorMzOf(const pwiz::msdata::SpectrumPtr &s) {
    const pwiz::msdata::SelectedIon& si = s->precursors.front().selectedIons.front();
    return si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<int>();
}

} // end namespace PWIZ HELPER


static inline string getActivationCode(const pwiz::msdata::Activation& a) {
    if (a.empty())
        return EMPTY_STR;
    if (a.hasCVParam(pwiz::msdata::MS_CID) && ! a.hasCVParam(pwiz::msdata::MS_ETD))
        return CID_STR;
    else if (a.hasCVParam(pwiz::msdata::MS_ETD)) //electron_transfer_dissociation))
        return ETD_STR;
    else if (a.hasCVParam(pwiz::msdata::MS_HCD)) //MS_high_energy_collision_induced_dissociation))
        return HCD_STR;
     else
        return UNKNOWN_STR;
}


/**
 * return the dataMode given a pwiz spectrum
 * @brief getDataMode
 * @param ptr
 * @return
 */
static DataMode getDataMode( const pwiz::msdata::SpectrumPtr s, DataMode wantedMode)  {

    const pwiz::msdata::CVParam& isCentroided = s->cvParam(pwiz::msdata::MS_centroid_spectrum);
    DataMode currentMode = !isCentroided.empty() ? CENTROID : PROFILE;
    DataMode effectiveMode;
    if (wantedMode == PROFILE && currentMode == PROFILE) {
        effectiveMode = PROFILE;
    } else if ((wantedMode == CENTROID && currentMode == PROFILE) ||
                 (wantedMode == FITTED && currentMode == PROFILE)) {
        effectiveMode = wantedMode;
    } else {
        // current is CENTROID nothing to do
        effectiveMode = CENTROID;
    }
    return effectiveMode;
}

namespace mzMath {


#define SIGMA_FACTOR 2.354820045


inline static double yMax( double xZero, double sigmaSquared, double x, double y) {
    double nx = x - xZero;
    return y / exp( - ( nx*nx ) /( 2 * sigmaSquared) );
}

inline static double y( double x, double xZero, double yMax, double sigmaSquared ) {
    double nx = x - xZero;
    return yMax * exp( - ( nx * nx ) / ( 2 * sigmaSquared) );
}

inline static double sigma( double width, double relativeHeight ) {
    /* actually if width is null there is no problem */
    if ( width < 0 ) {
        throw std::exception("Width must >= 0");
    }
    if ( relativeHeight < 0 || relativeHeight > 1 ) {
        throw std::exception("relativeHeight must be between 0 and 1");
    }

    return width /  sqrt(- 2 * log(relativeHeight) ) ;
}

inline static double width( double sigma, double relativeHeight) {
    if (! (sigma > 0) )
        throw std::exception("sigma must be > 0");
    if (! (relativeHeight <= 1) )
        throw std::exception("relative must  < 1");
    return sigma * ( 2 * sqrt(- 2 * log(relativeHeight) ) );
}

inline static double fwhm( double sigma) {
    return sigma * SIGMA_FACTOR;
}

/**the wizard math formula*/
inline static double mzToDeltaMz(double mz) {
    return 2E-07 * pow(mz, 1.5);
}

template< typename T>
inline bool static isFiniteNumber(T& x) {
   return (x <= DBL_MAX && x >= -DBL_MAX);
}

template<class mz_t, class int_t>
static double gaussianCentroidApex(const std::vector<mz_t>& xData, const std::vector<int_t>& yData) {
    int nb_values = xData.size();
    if (nb_values != 3)
        throw std::exception("[gaussian centroid apex] failed");

    double x_m1 = xData[0];
    double y_m1 = yData[0];
    double x_0 = xData[1];
    double y_0 = yData[1];
    double x_p1 = xData[2];
    double y_p1 = yData[2];

    double diff_log_y_0_p1 = log(y_0) - log(y_p1);
    double diff_log_y_p1_m1 = log(y_p1) - log(y_m1);
    double diff_log_y_m1_0 = log(y_m1) - log(y_0);


    double x = 0.5 * (diff_log_y_0_p1 * pow(x_m1, 2.0) + diff_log_y_p1_m1 * pow(x_0, 2.0) + diff_log_y_m1_0 * pow(x_p1, 2.0)) /
            (diff_log_y_0_p1 * x_m1 + diff_log_y_p1_m1 * x_0 + diff_log_y_m1_0 * x_p1);

    return x;
}

template<typename mz_t, typename int_t>
static double gaussianCentroidIntensityMax(const std::vector<mz_t>& xData, const std::vector<int_t>& yData) {
    double x1 = xData[0];
    double y1 = yData[0];
    double x2 = xData[1];
    double y2 = yData[1];
    double x3 = xData[2];
    double y3 = yData[2];

    double D = (x1 - x2) * (x1 - x3) * (x2 - x3);
    double alpha = x3 * (log(y2) - log(y1)) + x2 * (log(y1) - log(y3)) + x1 * (log(y3) - log(y2));
    double gamma = log(y1)* x2 * x3 * (x2 - x3) + log(y2)*x3*x1*(x3-x1) + log(y3) * x1 * x2 * (x1- x2);
    double beta = pow(x3, 2) * (log(y1) - log(y2)) + pow(x2, 2) * (log(y3) - log(y1)) + pow(x1, 2) * (log(y2) - log(y3));
    return exp( (gamma/D) - pow( beta/D , 2) / (4 * alpha/D) );
}

template<typename mz_t, typename int_t>
static double gaussianSigma(const std::vector<mz_t>& xData, const std::vector<int_t>& yData) {
    double x1 = xData[0];
    double y1 = yData[0];
    double x2 = xData[1];
    double y2 = yData[1];
    double x3 = xData[2];
    double y3 = yData[2];

    double D = (x1 - x2) * (x1 - x3) * (x2 - x3);
    double alpha = x3 * (log(y2) - log(y1)) + x2 * (log(y1) - log(y3)) + x1 * (log(y3) - log(y2));
    return sqrt( - 1. / ( 2 * (alpha/D)) );
}

}//end mzmath


inline static void printProgBar(int percent) {
    string bar;

    for (int i = 0; i < 50; i++) {
        if (i < (percent / 2)) {
            bar.replace(i, 1, "=");
        } else if (i == (percent / 2)) {
            bar.replace(i, 1, ">");
        } else {
            bar.replace(i, 1, " ");
        }
    }
    std::cout << "\r" "[" << bar << "] ";
    std::cout << percent << "%     " << std::flush;
}

}//end namespace

#endif // MZUTILS_HPP
