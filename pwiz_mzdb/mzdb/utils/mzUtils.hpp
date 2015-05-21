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


///A spectrum can be stored in different ways:
///
/// -`Profile`: the spectrum has been acquired in profile and the user want to keep
/// it in profile (all data points)
///
/// -`Fitted`: the spectrum has been acquired in profile and the user want to perform a
/// peak picking on it. The `Fitted` mode is an intermediary mode between profile and
/// centroid. Each mass peak is modelized to keep most of the information: its width at half
/// maximum and m/z value plus intensity value at the apex. This mode is a good trade-off between
/// file data size and peak informations.
///
/// -`Centroid`: the spectrum could already be acquired in centroid mode. In case, it is in profile,
/// a peak picking method is performed usually preferring `vendor` algorithm
enum DataMode {
    PROFILE = 1,
    CENTROID = 12,
    FITTED = 20
};

/// a mass peak can be encoded in three different ways:
///
/// -if this a low resolution spectrum (i.e. a MS2 spectrum for exmaple), it is preferred
/// to store it with m/z and intensity values encoded in 32 bits in order to save space: enum value of
/// 8 (4 bytes + 4 bytes)
///
/// -if this a high resolution spectrum (i.e. MS1 spectrum most of the time) a high precision in
/// m/z dimension is required: it will be stored using a 64 bits (double in moder desktop architecture):
/// enum value of 12 (4 bytes +  8 bytes)
///
/// -For some reasons, you may want to encode both m/z and intensities in 64 bits: enum value of 16
/// (8 bytes + 8 bytes)
enum PeakEncoding {
    LOW_RES_PEAK = 8,
    HIGH_RES_PEAK = 12,
    NO_LOSS_PEAK = 16
};


/// A DataEncoding is the combination of the DataMode @see DataMode, the PeakEncoding @see PeakEncoding
/// and a string which indicates if data are compressed or not. This strucure is used both in reading
/// and writing tasks; so its usage can vary slightly through cases.
struct DataEncoding {

    /// sqlite DB row id of `data_encoding` table
    int id;

    ///mode @see DataMode
    DataMode mode;

    ///peakEncoding @see PeakEncoding
    PeakEncoding peakEncoding;

    ///compression
    string compression;

    /**
     * DataEncoding of one or several spectrum(a)
     * @param id_
     * @param mode_ @see DataMode
     * @param pe_ @see PeakEncoding
     * @param compression default to `none`, no compression applied
     */
    DataEncoding(int id_, DataMode mode_, PeakEncoding pe_) : id(id), mode(mode_), peakEncoding(pe_), compression("none") {}

    ///Default constructor
    DataEncoding(){}

    ///Create `SQL` for inserting a row in `data_encoding` table with current member values
    string buildSQL() {
        string mzPrec, intPrec;
        if (peakEncoding == NO_LOSS_PEAK) {
            mzPrec = "64"; intPrec = "64";
        } else if (peakEncoding = HIGH_RES_PEAK) {
            mzPrec = "64"; intPrec = "32";
        } else if (peakEncoding == LOW_RES_PEAK) {
            mzPrec = "32"; intPrec = "32";
        }

        string mode_str;
        if (mode == PROFILE)
            mode_str = "profile";
        else if (mode == FITTED)
            mode_str = "fitted";
        else if (mode == CENTROID)
            mode_str = "centroid";

        return "INSERT INTO data_encoding VALUES (NULL, '" + mode_str + "', '"+ compression + "', 'little_endian', "+ mzPrec + ", " + intPrec + ");";
    }
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

///Utility function to find if a value already exists in the map
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


/// regroups functions taking in parameters pwiz::msdata::SpectrumPtr
/// gather some metadata on it
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
    return si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
}

} // end namespace PWIZ HELPER


/**
 * Get simple activation code as string for pwiz ``Activation`` object.
 * -HCD, CID, ETD
 *
 * @param a pwiz activation object contained in precursor object
 * @return string representing activation to be inserted in the database
 */
static inline string getActivationCode(const pwiz::msdata::Activation& a) {
    if (a.empty())
        return string(EMPTY_STR);
    if (a.hasCVParam(pwiz::msdata::MS_CID) && ! a.hasCVParam(pwiz::msdata::MS_ETD))
        return string(CID_STR);
    else if (a.hasCVParam(pwiz::msdata::MS_ETD)) //electron_transfer_dissociation))
        return string(ETD_STR);
    else if (a.hasCVParam(pwiz::msdata::MS_HCD)) //MS_high_energy_collision_induced_dissociation))
        return string(HCD_STR);
     else
        return string(UNKNOWN_STR);
}


/**
 * return the effective dataMode given a pwiz spectrum
 * Most of the time it corresponds to the wantedMode except for
 * this kind of cases:
 * wanted mode: `profile` and the spectrum was acquired in centroid.
 *
 * @param s:pwiz::msdata::SpectrumPtr
 * @param wantedMode: DataMode dataMode wanted by the user for this msLevel
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

/// some useful functions to get first approximation of centroid
namespace mzMath {


#define SIGMA_FACTOR 2.354820045

/// getting ``ymax`` with in a gaussian model
inline static double yMax( double xZero, double sigmaSquared, double x, double y) {
    double nx = x - xZero;
    return y / exp( - ( nx*nx ) /( 2 * sigmaSquared) );
}

/// compute y value given gaussian model parameters
inline static double y( double x, double xZero, double yMax, double sigmaSquared ) {
    double nx = x - xZero;
    return yMax * exp( - ( nx * nx ) / ( 2 * sigmaSquared) );
}

///compute sigma given gaussian parameters
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

///compute width of peak in Da from gaussian parameters
inline static double width( double sigma, double relativeHeight) {
    if (! (sigma > 0) )
        throw std::exception("sigma must be > 0");
    if (! (relativeHeight <= 1) )
        throw std::exception("relative must  < 1");
    return sigma * ( 2 * sqrt(- 2 * log(relativeHeight) ) );
}

///compute the FWHM (Full Width at HALF Maximum)
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

/// MaxQuant formula (parabola) to compute m/z centroid from mass peak data points
/// @param xData m/z values
/// @param yData intensities values
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

/// Extension of the MaxQuant formula (parabola) to compute intensity maximum from mass peak data points
/// (Does not perform very well)
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

/// Extension of the MaxQuant formula (parabola) to compute sigma maximum from mass peak data points
/// (not rigorous since gaussian is a parabola only around the apex)
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


///utility function to print a nice progression bar
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
