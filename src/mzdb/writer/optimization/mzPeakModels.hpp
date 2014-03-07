#ifndef MZPEAKOPTIMIZATION_H
#define MZPEAKOPTIMIZATION_H

#include "../msdata/mzPeak.hpp"

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

/** Simple fitting without jacobian calculation */
/*
template<class mz_t, class int_t>
class GaussianOptimizationWithoutYmax{

private:
    const double x;
    const double y;
    const Centroid<mz_t, int_t>* centroid;

public:
    GaussianOptimizationWithoutYmax(double x_, double y_, Centroid<mz_t, int_t>* centroid_):
        x(x_), y(y_), centroid(centroid_) {}

    template<typename T>
    bool operator() ( const T* data, T* residual) const {
        const T mz(centroid->mz);
        const T intensity(centroid->intensity);
        const T x_(x);
        const T y_(y);
        T sigma = (x_ <= mz) ? data[1] : data[2];
        T dx(x_ - mz);
        residual[0] = y_ - (intensity * exp(- (dx * dx) / ( T(2.0) * sigma * sigma)));
        return true;
    }
};


template<class mz_t, class int_t>
class SimplifiedGaussianFittingCentroids {

private:
    const double x;
    const double y;
    const vector<Centroid<mz_t, int_t>* > centroids;

public:
    SimplifiedGaussianFittingCentroids(double x_, double y_, vector<Centroid<mz_t, int_t>* >& centroids_):
        x(x_), y(y_), centroids(centroids_) {}

    template<typename T>
    bool operator() ( const T* lwhms, const T* rwhms, T* residual) const {//const T* intensities,
        T computedValue(0.0);
        for (size_t i = 0; i < centroids.size(); ++i) {
            const T intensity(centroids[i]->intensity); //= intensities[i];
            const T mz(centroids[i]->mz);
            const T sigma = (T(x) <= mz) ? lwhms[i] : rwhms[i];
            const T sigma2 = sigma * sigma;
            computedValue += ( T(-1.0) / (T(2.0) * sigma2)) * (T(x) * T(x)) + (mz / sigma2) * T(x) - ((mz * mz ) / (T(2.0) * sigma2)) + log(intensity);
        }
        residual[0] = T(y) - computedValue;
        return true;
    }
};

template<class mz_t, class int_t>
class ParabolaFittingCentroids {

private:
    const double x;
    const double y;
    const vector<Centroid<mz_t, int_t>* > centroids;

public:
    ParabolaFittingCentroids(double x_, double y_, vector<Centroid<mz_t, int_t>* >& centroids_):
        x(x_), y(y_), centroids(centroids_) {}

    template<typename T>
    bool operator() (const T* as, const T* bs, const T* cs,
                     const T* aps, const T* bps, const T* cps,  T* residual) const {
        T computedValue(0.0);
        for (size_t i = 0; i < centroids.size(); ++i) {
            const bool left = x < centroids[i]->mz;
            const T a = (left) ? as[i] : aps[i];
            const T b = left ? bs[i] : bps[i];
            const T c = left ? cs[i]: cps[i];
            computedValue += a * (T(x) * T(x)) + b * T(x)  + c;
        }
        residual[0] = T(y) - computedValue;
        return true;
    }
};

*/




//GSL IMPL does ont works very well

/*struct  PenaltyFactors {

    PenaltyFactors() :pos(0), height(0), lWidth(0), rWidth(0), background(0) {}

    PenaltyFactors(const PenaltyFactors & p) : pos(p.pos), lWidth(p.lWidth), rWidth(p.rWidth), background(p.background) {}

    inline PenaltyFactors & operator=(const PenaltyFactors & p)
    {
        pos = p.pos;
        height = p.height,
        lWidth = p.lWidth;
        rWidth = p.rWidth;
        background = p.background;
        return *this;
    }

    ~PenaltyFactors(){}

    /// Penalty factor for the peak shape's position
    double pos;
    /// Penalty factor for the peak shape's left width parameter
    double lWidth;
    /// Penalty factor for the peak shape's right width parameter
    double rWidth;

    double height;

    double background;
};


template<typename mz_t=double, typename int_t=double>
struct Data {

    vector<mz_t> xData;
    vector<int_t> yData;
    vector<Centroid<mz_t, int_t>*> peaks;
    PenaltyFactors penalties;

    Data(const vector<mz_t>& x, const vector<int_t>& y, vector<Centroid<mz_t, int_t>*>& p) : xData(x), yData(y), peaks(p){}

};



namespace {

template<typename mz_t, typename int_t>
int residuals(const gsl_vector *x, void *params, gsl_vector *f) {


    vector<int_t>& yData = static_cast<Data<mz_t, int_t>*>(params)->yData;
    vector<mz_t>& xData = static_cast<Data<mz_t, int_t> *>(params)->xData;
    vector<Centroid<mz_t, int_t>*>& peaks = static_cast<Data<mz_t, int_t> *>(params)->peaks;
    //PenaltyFactors& penalties = static_cast<Data<mz_t, int_t>*>(params)->penalties;

    // iterate over all points of the signal
    for (size_t i = 0; i < xData.size(); i ++) {
        double xi = xData[i];
        double computed_signal = 0.0;
        double experimental_signal = yData[i];

        // iterate over all peaks
        for (size_t j = 0; j < peaks.size(); j ++) {
            // Store the current parameters for this peak
            //double background = gsl_vector_get(x, 5 * j);
            double intensity = gsl_vector_get(x, 5*j + 1);
            double mz = gsl_vector_get(x, 5*j + 2);
            double demifwhm = (xi <= mz) ? gsl_vector_get(x,  5*j + 3) : gsl_vector_get(x, 5*j + 4); //the left of right width
            //double demifwhm2 = demifwhm * demifwhm;
            //double nx = xi - mz;
            double sigma = (demifwhm * 2.0) / SIGMA_FACTOR;
            double sigma2 = sigma * sigma;
            //BEST FOR THE MOMENT
            computed_signal += (- 1.0 / 2.0 * sigma2) * (xi * xi) + (mz / sigma2) * xi - (mz * mz ) / (2.0 * sigma2) + log(intensity);

            //LORENTZ
            //computed_signal += intensity / ( 1.0 + pow(demifwhm * nx, 2));//intensity * (demifwhm2 / ( nx * nx + demifwhm2));//intensity / (1.0 + pow(demifwhm * (current_position - mz), 2));

            //POLY
            /*double a = gsl_vector_get(x, 3 * current_peak);
            double b = gsl_vector_get(x, 3 * current_peak + 1);
            double c = gsl_vector_get(x, 3 * current_peak + 2);
            computed_signal += a * (current_position * current_position) + b * current_position + c;*/


            //double nx = xi - mz;
            //we assume here that it follows a gaussian shape
            //computed_signal += intensity * exp ( - (nx * nx) / ( 2 * sigma * sigma) );

        /*}
        //set the point
        gsl_vector_set(f, i, computed_signal - experimental_signal);
    }*/
    //calculate the penalties
    /*double penalty = 0.;
    double penalty_pos = penalties.pos;
    double penalty_lwidth = penalties.lWidth;
    double penalty_rwidth = penalties.rWidth;
    double penalty_intensity = penalties.height;
    double penalty_background = penalties.background;

    // iterate over all peaks again to compute the penalties
    for (size_t current_peak = 0; current_peak < peaks.size(); current_peak++) {
        double old_background = peaks[current_peak]->_background;
        double old_position = peaks[current_peak]->_mz;
        double old_height = peaks[current_peak]->_intensity;
        double old_width_l = peaks[current_peak]->_lwhm;
        double old_width_r = peaks[current_peak]->_rwhm;

        double p_background = gsl_vector_get(x, 5 * current_peak);
        double p_height = gsl_vector_get(x, 5 * current_peak + 1);
        double p_position = gsl_vector_get(x, 5 * current_peak + 2);
        double p_width_l = gsl_vector_get(x, 5 * current_peak + 3);
        double p_width_r = gsl_vector_get(x, 5 * current_peak + 4);

        if (p_background < 0) {
            penalty += 100000 * penalty_background * pow(fabs(p_background - old_background), 2);
        }

        if (p_height < 1)
            penalty += 100000 * penalty_intensity * pow(fabs(p_height - old_height), 2);

        if (p_width_l < 0){
            penalty += penalty_lwidth * peaks.size() * 10000 * pow(fabs(p_width_l - old_width_l), 2);
        }//else if (p_width_l < 1.5)
        //    penalty += 10000 * pow(fabs(p_width_l - old_width_l), 2);

        if (p_width_r < 0){
            penalty += penalty_rwidth * peaks.size() * 10000 * pow(fabs(p_width_r - old_width_r), 2);
        }//else if (p_width_r < 1.5)
          //  penalty += 10000 * pow(fabs(p_width_r - old_width_r), 2);


        if (fabs(old_position - p_position) > 0.1){
            penalty += 10000 * penalty_pos * pow(fabs(old_position - p_position), 2);
        }
    }

    gsl_vector_set(f, xData.size(),  penalty);

    return GSL_SUCCESS;
}


template<typename mz_t, typename int_t>
int jacobian(const gsl_vector * x, void * params, gsl_matrix * J) {
    // For the conventions on x and params c.f. the commentary in residual()
    //
    // The matrix J is supposed to contain the result when we return from this function.
    // Note: GSL expects the Jacobian as follows:
    // - each row corresponds to one data point
    // - each column corresponds to one parameter
    // std::vector<DoubleReal>& signal = static_cast<OptimizePick::Data*> (params) ->signal;
    vector<mz_t> & positions = static_cast<Data<mz_t, int_t>*>(params)->xData;
    vector<Centroid<mz_t, int_t>*>& peaks = static_cast<Data<mz_t, int_t>*>(params)->peaks;
    //PenaltyFactors & penalties = static_cast<Data<mz_t, int_t> *>(params)->penalties;
    // iterate over all points of the signal
    for (size_t i = 0; i < positions.size(); i++) {

        double xi = positions[i];

        // iterate over all peaks
        for (size_t j = 0; j < peaks.size(); j++) {
            // Store the current parameters for this peak
            //the best
            //double background = gsl_vector_get(x, 5 * j);
            double intensity = gsl_vector_get(x, 5*j + 1);
            double mz = gsl_vector_get(x, 5 * j + 2);
            double demifwhm = (xi <= mz) ? gsl_vector_get(x,  5*j + 3) : gsl_vector_get(x, 5 * j + 4); //the left of right width
            double sigma = (demifwhm * 2.0) / SIGMA_FACTOR;
            double nx = xi - mz;
            double nx2 = nx * nx;
            //double sigma2 = sigma * sigma;

            /*double exp_ = exp( - nx2 / (2 * sigma2) );
            double f = intensity * exp_ * ( nx / sigma2 );
            double dfdd = f * (nx / sigma);*/

            /*gsl_matrix_set(J, i, 5 * j, 0.0);     //partial derivative against a
            gsl_matrix_set(J, i, 5 * j + 1, exp_); //partial derivative against b
            gsl_matrix_set(J, i, 5 * j + 2, f); //partial derivative against c
            if (xi <=  mz) {
                gsl_matrix_set(J, i, 5 * j + 3, dfdd); //partial derivative against d
                gsl_matrix_set(J, i, 5 * j + 4, 0.0);
            } else {
                gsl_matrix_set(J, i, 5 * j + 3, 0.0); //partial derivative against d
                gsl_matrix_set(J, i, 5 * j + 4, dfdd);
            }




            //GAUSS_APPROX FIT WORKS THE BEST
            double dfdy = 1.0 / intensity;
            double dfdl = (xi < mz)  ? nx2 / pow(sigma,3) : 0;
            double dfdr = (xi >= mz)  ? nx2 / pow(sigma,3) : 0;
            double dfdx0 = nx / (sigma * sigma);

            /*LORENTZ
            double diff = current_position - mz;
            double denom_inv = 1.0 / (1.0 + pow(demifwhm * diff, 2));
            double ddl_left  = (current_position <= mz)
                    ? -2 * intensity * pow(diff, 2) * demifwhm * pow(denom_inv, 2) :
                      0;

            double ddl_right = (current_position  > mz)
                    ? -2 * intensity * pow(diff, 2) * demifwhm * pow(denom_inv, 2) :
                      0;

            double ddx0          = -2 * intensity * pow(demifwhm, 2) * diff * pow(denom_inv, 2);*/


            /*double a = gsl_vector_get(x, 3 * current_peak);
            double b = gsl_vector_get(x, 3 * current_peak + 1);
            double c = gsl_vector_get(x, 3 * current_peak + 2);

            gsl_matrix_set(J, i, 5 * j, 0.0);
            gsl_matrix_set(J, i, 5 * j + 1, dfdy);
            gsl_matrix_set(J, i, 5 * j + 2, dfdx0);
            gsl_matrix_set(J, i, 5 * j + 3, dfdl);
            gsl_matrix_set(J, i, 5 * j + 4, dfdr);



        }
    }
    //compute the penalties
    /*for (size_t current_peak = 0; current_peak < peaks.size(); current_peak++) {
        double p_background = gsl_vector_get(x, 5 * current_peak);
        double p_width_left = gsl_vector_get(x, 5 * current_peak + 3);
        double p_width_right = gsl_vector_get(x, 5 * current_peak + 4);
        double p_position = gsl_vector_get(x, 5 * current_peak + 2);
        double p_height = gsl_vector_get(x, 5 * current_peak + 1);

        double old_background = peaks[current_peak]->_background;
        double old_width_left = peaks[current_peak]->_lwhm;
        double old_width_right = peaks[current_peak]->_rwhm;
        double old_position = peaks[current_peak]->_mz;
        double old_height = peaks[current_peak]->_intensity;

        double penalty_b = 0.0, penalty_h = 0., penalty_l = 0., penalty_r = 0., penalty_p = 0.;

        if (p_background < 0) {
            penalty_b += 100000 * 2 * penalty_b * (fabs(p_background - old_background));
        }

        if (p_height < 1)
            penalty_h += 100000 * 2 * penalties.height * (fabs(p_height) - fabs(old_height));

        if (p_width_left < 0 )
                penalty_l += peaks.size() * 2 * penalties.lWidth * 10000 * (fabs(p_width_left - old_width_left));
        //else if (p_width_left < 1.5)
        //        penalty_l += 2 * penalties.lWidth * 10000 * pow(fabs(p_width_left - old_width_left), 2);

        if (p_width_right < 0 )
            penalty_r += peaks.size() * 2 * penalties.rWidth * 10000 * (fabs(p_width_right - old_width_right));
        //else if (p_width_right < 1.5 )
        //        penalty_r += 2 * penalties.rWidth * 10000 * pow(fabs(p_width_right - old_width_right), 2);

        if (fabs(old_position - p_position) > 0.1)

            penalty_p += 10000 * penalties.pos * 2 * fabs(old_position - p_position);

        gsl_matrix_set(J, positions.size(), 5 * current_peak, 100 * penalty_b);
        gsl_matrix_set(J, positions.size(), 5 * current_peak + 1, 100 * penalty_h);
        gsl_matrix_set(J, positions.size(), 5 * current_peak + 3, 100 * penalty_l);
        gsl_matrix_set(J, positions.size(), 5 * current_peak + 4, 100 * penalty_r);
        gsl_matrix_set(J, positions.size(), 5 * current_peak + 2, 100 * penalty_p);
       }

    return GSL_SUCCESS;
}

template<typename mz_t, typename int_t>
int evaluate(const gsl_vector * x, void * params, gsl_vector * f, gsl_matrix * J) {
    residuals<mz_t, int_t>(x, params, f);
    jacobian<mz_t, int_t>(x, params, J);
    return GSL_SUCCESS;
}

}

template<typename mz_t = double, typename int_t = double>
class mzPeakShapeOptimizer {

protected:

    /// Maximum number of iterations during optimization
    unsigned int _maxIteration;

    /// Maximum absolute and relative error used in the optimization.
    double _errorAbs;
    double _errorRel;

public:

    mzPeakShapeOptimizer(const int max_iteration, const double eps_abs, const double eps_rel):_maxIteration(max_iteration), _errorAbs(eps_abs), _errorRel(eps_rel){}

    void optimize(Data<mz_t, int_t>& data) {

        vector<Centroid<mz_t, int_t>*>& peaks = data.peaks;

        if ( peaks.empty() ) {//|| peaks.size() != data->detectedPeaks.size()) {
            //printf("\nempty peaks\n");
            return;
        }
        //we store four parameters per peaks
        gsl_vector* start_value  = gsl_vector_alloc( 5 * peaks.size() );


        for (size_t i =0; i < peaks.size(); ++i) {
            Centroid<mz_t, int_t>& currentPeak = *(peaks[i]);
            double a = currentPeak.background;
            double b = currentPeak.intensity;
            double c = currentPeak.mz; //may consider that mz is alredy fully optimized
            double dl = currentPeak.lwhm;
            double dr = currentPeak.rwhm;

            gsl_vector_set(start_value, 5 * i, 0.0);
            gsl_vector_set(start_value, 5 * i + 1, b);
            gsl_vector_set(start_value, 5 * i + 2, c);
            gsl_vector_set(start_value, 5 * i + 3, dl);
            gsl_vector_set(start_value, 5 * i + 4, dr);
        }

        // The gsl algorithms require us to provide function pointers for the evaluation of
        // the target function.
        gsl_multifit_function_fdf fit_function;
        fit_function.f = residuals<mz_t, int_t>;
        fit_function.df = jacobian<mz_t, int_t>;
        fit_function.fdf = evaluate<mz_t, int_t>;
        fit_function.n  = std::max(data.xData.size() + 1, 5 * peaks.size());
        fit_function.p  = 5 * peaks.size();
        fit_function.params = &data;


        const gsl_multifit_fdfsolver_type * type = gsl_multifit_fdfsolver_lmsder;

        gsl_multifit_fdfsolver* fit = gsl_multifit_fdfsolver_alloc(type, std::max(data.xData.size() + 1, 5 * peaks.size()), 5 * peaks.size());

        gsl_multifit_fdfsolver_set(fit, &fit_function, start_value);

        // initial norm
        //std::cout << "Before optimization" << std::endl;

        // Iteration
        unsigned int iteration = 0;
        int status;

        do {
            iteration++;
            status = gsl_multifit_fdfsolver_iterate(fit);

            if (boost::math::isnan(gsl_blas_dnrm2(fit->dx)))
                break;

            // We use the gsl function gsl_multifit_test_delta to decide if we can finish the iteration.
            // We only finish if all new parameters deviates only by a small amount from the parameters of the last iteration
            status = gsl_multifit_test_delta(fit->dx, fit->x, _errorAbs, _errorRel);
            if (status != GSL_CONTINUE)
                break;

        } while (status == GSL_CONTINUE && iteration < _maxIteration);
        //get optimized values

        for (size_t current_peak = 0; current_peak < peaks.size(); current_peak++) {
            // Store the current parameters for this peak
            Centroid<mz_t, int_t>& c = *(peaks[current_peak]);
            /*double a = gsl_vector_get(fit->x, 3 * current_peak);
            double b = gsl_vector_get(fit->x, 3 * current_peak + 1);
            double c = gsl_vector_get(fit->x, 3 * current_peak + 1);
            double b2 = b * b;
            double d1 = b2 - 4 * a * c;
            centroid->_intensity = ( -d1 / (4 * a));
            centroid->_mz = -b / (2 * a);
            double d3 = b2 - 4 * a * (c + d1 / (8 * a));
            double x1 = ( -b - sqrt(d3)) / (2 * a);
            double x2 = ( -b + sqrt(d3)) / (2 * a);
            centroid->_lwhm = (centroid->_mz - x1) * 2;
            centroid->_rwhm = centroid->_lwhm;
            c.background = gsl_vector_get(fit->x, 5 * current_peak);
            c.intensity = gsl_vector_get(fit->x, 5 * current_peak + 1);
            c.mz = gsl_vector_get(fit->x, 5 * current_peak + 2);
            c.lwhm = gsl_vector_get(fit->x, 5 * current_peak + 3);
            c.rwhm = gsl_vector_get(fit->x, 5 * current_peak + 4);
            //printf("%f, %f, %f, %f, %f\n", c.background, c.intensity, c.mz, c.lwhm, c.rwhm);
        }

        gsl_multifit_fdfsolver_free(fit);
        gsl_vector_free(start_value);
    }




};
/*
using namespace dlib;


template<typename MatrixType>
double residual(const pair<input_vector, double>& data, const MatrixType& params, vector<Centroid<mz_t, int_t>* >& peaks ) {
    double x = data.first(0);
    double computedSignal = 0d;
    for (size_t i = 0; i < peaks.size(); ++i) {
        //const double& background = params(i * 5);
        const double& intensity = params(i * 5 + 1);
        const double& mz = params(i * 5 + 2);
        const double& lwhm = params(i * 5 + 3);
        const double& rwhm = params(i * 5 + 4);
        const double sigma = x < mz ? (lwhm * 2) / SIGMA_FACTOR : (rwhm * 2)/ SIGMA_FACTOR;
        const double sigma2 = sigma * sigma;
        computedSignal += (- 1.0 / 2.0 * sigma2) * (x * x) + (mz / sigma2) * x - (mz * mz ) / (2.0 * sigma2) + log(intensity);
    }
    return data.second - computedSignal;
}

template<typename mz_t, typename int_t>
class DlibOptimizer {
    vector<Centroid<mz_t, int_t>* > _centroids;

    DlibOptimizer(vector<Centroid<mz_t, int_t>* >& centroids) : _centroids(centroid) {}



};*/



}//end namespace
#endif // MZPEAKOPTIMIZATION_H
