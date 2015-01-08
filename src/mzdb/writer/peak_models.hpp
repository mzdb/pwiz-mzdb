#ifndef MZPEAKOPTIMIZATION_H
#define MZPEAKOPTIMIZATION_H

#include "peak.hpp"

namespace mzdb {

/** Simple fitting without jacobian calculation */
template<class mz_t, class int_t>
class GaussianFittingCentroids {

private:
    const double x;
    const double y;
    const vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids;

public:
    GaussianFittingCentroids(double x_, double y_, vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids_):
        x(x_), y(y_), centroids(centroids_) {}

    template<typename T>
    bool operator() ( const T* const data, T* residual) const {
        T computedValue(0.0);
        const T y_(y);

        for (size_t i = 0; i < centroids.size(); ++i) {
            const T mz(centroids[i]->mz);
            const T x_(x);
            T sigma = (x_ <= mz) ? data[i * 3 + 1] : data[i * 3 +2];
            computedValue +=  data[i * 3] * exp(- pow( (x_ - mz) / ( T(2.0) * sigma), 2) );
        }
        residual[0] = y_ - computedValue;
        return true;
    }
};

}//end namespace
#endif // MZPEAKOPTIMIZATION_H
