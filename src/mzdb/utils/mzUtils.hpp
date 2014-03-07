#ifndef MZUTILS_HPP
#define MZUTILS_HPP

#include <iostream>

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/analysis/peakdetect/PeakFamilyDetectorFT.hpp"
#include "pwiz/analysis/spectrum_processing/PrecursorRecalculatorDefault.hpp"
//#include <emmintrin.h> //sse2
#include <math.h>

namespace mzdb {

using namespace std;


#define __BEGIN_MZDB_NM namespace mzdb {

#define __END_MZDB_NM }

#define USE_STD_NM using namespace std;


#define xml_float "xsd:float"

#define xml_string "xsd:string"

#define xml_double "xsd:double"

#define xml_int "xsd:int"

#define param_name_str "name"

#define param_value_str "value"

#define param_type_str "type"

#define ms_ns_str "MS"

#define cv_ref_str "cvRef"

#define access_nb_str "accession"

#define params_str "params"

#define cv_params_str "cvParams"

#define cv_param_str "cvParam"

#define user_params_str "userParams"

#define user_param_str "userParam"

#define user_texts_str "userTexts"

#define user_text_str "userText"

#define true_str "true"

#define false_str "false"

#define empty_str "empty"

#define cid_str "CID"

#define etd_str "ETD"

#define hcd_str "HCD"

#define unknown_str "unknown"



/*functions*/
#define ppm2mz(mz, ppm) mz * ppm / 1e6

#define mz2ppm(mz, ppm) mz * 1e6 / ppm;



/**useful typedef */
typedef unsigned char byte;
typedef unsigned int mz_uint;




#define DELETE_IF_HAS_OWNERSHIP(ownership, vec)  \
        if ( ownership == TAKE_OWNERSHIP) { \
            for (auto it= vec.begin(); it != vec.end(); ++it) { \
                delete *it; \
            } \
        }





enum Ownership {
    TAKE_OWNERSHIP,
    DO_NOT_TAKE_OWNERSHIP
};

enum ConstructorFile {
    RAW = 1,
    WIFF = 2
};

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


struct DataEncoding {
    int id;
    DataMode mode;
    PeakEncoding peakEncoding;

    DataEncoding(int id_, DataMode mode_, PeakEncoding pe_) : id(id), mode(mode_), peakEncoding(pe_) {}
    DataEncoding(){}
};



template <typename T>
inline static void put(T data, vector<byte>& v) {
    //memcpy(&v[wpos], (byte*)&data, sizeof(T));
    //++wpos;
    unsigned int wpos_ = v.size();
    unsigned int s = sizeof (data);
    if (v.size() < (wpos_ + s))
       v.resize(wpos_ + s);
    memcpy(&v[wpos_], (byte*) & data, s);
}

template<typename T>
inline static T get(unsigned int index, vector<byte> &buffer) {
    //if (index + sizeof (T) <= buffer.size())
    return *((T*) & buffer[index]);
    //return 0;
}

template<typename T> inline static bool isInMapKeys(int value, map<int, T>& m) {
    typename map<int, T>::iterator it;
    it = m.find(value);
    if (it == m.end())
        return false; //not found
    return true; //found
}


inline static float rtOf(const pwiz::msdata::SpectrumPtr& s) {
    pwiz::msdata::Scan* scan = !s->scanList.empty() ? &s->scanList.scans[0] : 0;
    if (scan == 0) {
        printf("\nCan not find RT of one spectrum !\n");
        return 0;
    }
    pwiz::msdata::CVParam scanTimeParam = scan ? scan->cvParam(pwiz::msdata::MS_scan_start_time) : pwiz::msdata::CVParam();
    return static_cast<float> (scanTimeParam.timeInSeconds());
}

/*
inline static int msLevelOf(const pwiz::msdata::SpectrumPtr &s) {
    return s->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
}


inline static double precursorMzOf(const pwiz::msdata::SpectrumPtr &s) {
    const pwiz::msdata::SelectedIon& si = s->precursors.front().selectedIons.front();
    return si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
}

*/
inline static int precursorChargeOf(const pwiz::msdata::SpectrumPtr &s) {
    const pwiz::msdata::SelectedIon& si = s->precursors.front().selectedIons.front();
    return si.cvParam(pwiz::msdata::MS_charge_state).valueAs<int>();
}



static inline string getActivationCode(const pwiz::msdata::Activation& a) {
    if (a.empty())
        return empty_str;
    if (a.hasCVParam(pwiz::msdata::MS_collision_induced_dissociation) && ! a.hasCVParam(pwiz::msdata::MS_electron_transfer_dissociation) )
        return cid_str;
    else if (a.hasCVParam(pwiz::msdata::MS_electron_transfer_dissociation))
        return etd_str;
    else if (a.hasCVParam(pwiz::msdata::MS_high_energy_collision_induced_dissociation))
        return hcd_str;
    else
        return unknown_str;
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
       //printf("%f\n", width);
       throw std::exception("Width must >= 0");
   }
   if ( relativeHeight < 0 || relativeHeight > 1 ) {
       //printf("%f\n", relativeHeight);
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

template<class mz_t, class int_t>
static double gaussianCentroidApex(const std::vector<mz_t>& xData, const std::vector<int_t>& yData) {
     int nb_values = xData.size();
     if (nb_values != 3)
         throw std::exception("gaussian centroid apex");

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
    /*
     __m128d x_m1 = *(__m128d*)(&xData[0]);
     __m128d y_m1 = *(__m128d)(&yData[0]);
     __m128d x_0 = *(__m128d*)(&xData[1]);
     __m128d y_0 = *(__m128d)(&yData[1]);
     __m128d x_p1 = *(__m128d*)(&xData[2]);
     __m128d y_p1 = *(__m128d)(&yData[2]);

     __m128d diff_log_y_0_p1  = _mm_log*/


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
     //if (alpha /D < 0)
     //    printf("aie aie aie\n");
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
    cout << "\r" "[" << bar << "] ";
    cout << percent << "%     " << flush;
}

}//end namespace

#endif // MZUTILS_HPP
