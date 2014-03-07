#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <vector>
#include <map>
#include <unordered_map>

namespace mzdb {


template<typename U, typename V>
struct mzSpectrum;

template<typename U, typename V>
struct Centroid;


/** objects */
typedef mzSpectrum<double, double> noLossSpec;

typedef mzSpectrum<double, float> highResSpec;

typedef mzSpectrum<float, float> lowResSpec;


typedef noLossSpec* noLossSpecPtr;

typedef highResSpec* highResSpecPtr;

typedef lowResSpec* lowResSpecPtr;


/** container */
typedef std::vector<noLossSpecPtr> no_loss_spec_vec;

typedef std::vector<highResSpecPtr> high_res_spec_vec;

typedef std::vector<lowResSpecPtr> low_res_spec_vec;


/** msLevel, Spectrum used as Buffer*/
typedef std::unordered_map<int, no_loss_spec_vec> no_loss_spec_vec_by_ms_level;

typedef std::unordered_map<int, high_res_spec_vec> high_res_spec_vec_by_ms_level;

typedef std::unordered_map<int, low_res_spec_vec> low_res_spec_vec_by_ms_level;

}//end namespace

#endif // TYPEDEFS_H
