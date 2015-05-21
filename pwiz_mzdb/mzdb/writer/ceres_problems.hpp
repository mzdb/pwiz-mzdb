#ifndef MZBUILDFITTINGPROBLEM_HPP
#define MZBUILDFITTINGPROBLEM_HPP

#include "ceres/ceres.h"
#include "peak.hpp"
#include "peak_models.hpp"

namespace mzdb {
using namespace std;

// this is better to use only one block parameter because the performance are better
class AutoDiffCostFunctionFactory{

public:

    template<typename functor,  typename mz_t, typename int_t> //typename nb_params,
    static ceres::CostFunction* buildCostFunction( vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids_b, const mz_t& x, const int_t& y)  {      // init guess of the parameters right sigmas

        int size = centroids_b.size();
        switch (size) {
        case 1: {
            return new ceres::AutoDiffCostFunction<functor, 1, 3>( new functor(x, y, centroids_b) );
            break;
        }
        case 2: {
            return new ceres::AutoDiffCostFunction<functor, 1, 6>( new functor(x, y, centroids_b) );
            break;
        }
        case 3: {
            return new ceres::AutoDiffCostFunction<functor, 1, 9>( new functor(x, y, centroids_b) );
            break;
        }
        case 4: {
            return new ceres::AutoDiffCostFunction<functor, 1, 12>( new functor(x, y, centroids_b) );
            break;
        }
        case 5: {
            return new ceres::AutoDiffCostFunction<functor, 1, 15>( new functor(x, y, centroids_b) );
            break;
        }
        case 6: {
            return new ceres::AutoDiffCostFunction<functor, 1, 18>( new functor(x, y, centroids_b) );
            break;
        }
        case 7: {
            return new ceres::AutoDiffCostFunction<functor, 1, 21>( new functor(x, y, centroids_b) );
            break;
        }
        case 8: {
            return new ceres::AutoDiffCostFunction<functor, 1, 24>( new functor(x, y, centroids_b) );
            break;
        }
        case 9: {
            return new ceres::AutoDiffCostFunction<functor, 1, 27>( new functor(x, y, centroids_b) );
            break;
        }
        case 10: {
            return new ceres::AutoDiffCostFunction<functor, 1, 30>( new functor(x, y, centroids_b) );
            break;
        }
        /*case 11: {
            return new ceres::AutoDiffCostFunction<functor, 1, 33>( new functor(x, y, centroids_b) );
            break;
        }
        case 12: {
            return new ceres::AutoDiffCostFunction<functor, 1, 36>( new functor(x, y, centroids_b) );
            break;
        }
        case 13: {
            return new ceres::AutoDiffCostFunction<functor, 1, 39>( new functor(x, y, centroids_b) );
            break;
        }
        case 14: {
            return new ceres::AutoDiffCostFunction<functor, 1, 42>( new functor(x, y, centroids_b) );
            break;
        }
        case 15: {
            return new ceres::AutoDiffCostFunction<functor, 1, 45>( new functor(x, y, centroids_b) );
            break;
        }
        case 16: {
            return new ceres::AutoDiffCostFunction<functor, 1, 48>( new functor(x, y, centroids_b) );
            break;
        }
        case 17: {
            return new ceres::AutoDiffCostFunction<functor, 1, 51>( new functor(x, y, centroids_b) );
            break;
        }
        case 18: {
            return new ceres::AutoDiffCostFunction<functor, 1, 54>( new functor(x, y, centroids_b) );
            break;
        }
        case 19: {
            return new ceres::AutoDiffCostFunction<functor, 1, 57>( new functor(x, y, centroids_b) );
            break;
        }
        case 20: {
            return new ceres::AutoDiffCostFunction<functor, 1, 60>( new functor(x, y, centroids_b) );
            break;
        }
        case 21: {
            return new ceres::AutoDiffCostFunction<functor, 1, 63>( new functor(x, y, centroids_b) );
            break;
        }
        case 22: {
            return new ceres::AutoDiffCostFunction<functor, 1, 66>( new functor(x, y, centroids_b) );
            break;
        }
        case 23: {
            return new ceres::AutoDiffCostFunction<functor, 1, 69>( new functor(x, y, centroids_b) );
            break;
        }
        case 24: {
            return new ceres::AutoDiffCostFunction<functor, 1, 72>( new functor(x, y, centroids_b) );
            break;
        }
        case 25: {
            return new ceres::AutoDiffCostFunction<functor, 1, 75>( new functor(x, y, centroids_b) );
            break;
        }
        case 26: {
            return new ceres::AutoDiffCostFunction<functor, 1, 78>( new functor(x, y, centroids_b) );
            break;
        }
        case 27: {
            return new ceres::AutoDiffCostFunction<functor, 1, 81>( new functor(x, y, centroids_b) );
            break;
        }
        case 28: {
            return new ceres::AutoDiffCostFunction<functor, 1, 84>( new functor(x, y, centroids_b) );
            break;
        }
        case 29: {
            return new ceres::AutoDiffCostFunction<functor, 1, 87>( new functor(x, y, centroids_b) );
            break;
        }
        case 30: {
            return new ceres::AutoDiffCostFunction<functor, 1, 90>( new functor(x, y, centroids_b) );
            break;
        }*/
        default: {
            printf("%d\n", size);
            throw exception("Max allowed 10 !");
        }

        }

    }

};



template<typename mz_t, typename int_t>
class ProblemSolver {
    vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids;
    const vector<mz_t>& xData;
    const vector<int_t>& yData;

public:

     inline ProblemSolver(const vector<mz_t>& x_,
                          const vector<int_t>& y_,
                          vector<std::shared_ptr<Centroid<mz_t, int_t> > >& centroids_): centroids(centroids_), xData(x_), yData(y_) {
    }


    void solve(vector<std::shared_ptr<Centroid<mz_t, int_t> > >& output, ceres::Solver::Options options = getDefaultOptions()) {

        ceres::Problem problem;
        /** to store initial parameters*/
        vector<double> data;
        //lwhms.reserve(centroids.size()); rwhms.reserve(centroids.size());
        //vector<double> as, bs, cs, aps, bps, cps;

        /** fill init parameters*/
        for (size_t i = 0; i < centroids.size(); ++i) {
            auto c = centroids[i];
            double intensity = c->intensity;
            double lwhm = c->lwhm;
            double rwhm = c->rwhm;

            data.push_back(intensity);
            data.push_back( (lwhm * 2.0) / SIGMA_FACTOR ); //enter sigma directly
            data.push_back( (rwhm * 2.0) / SIGMA_FACTOR );
        }

        /** add a cost function for each point */
        if (centroids.size() > 10 ) {
			for (size_t i = 0; i < centroids.size(); ++i) {
					output.push_back( centroids[i] );
            }
        } else {
			for (size_t i = 0; i < xData.size(); ++i) {
				ceres::CostFunction* costFunction = AutoDiffCostFunctionFactory::buildCostFunction<GaussianFittingCentroids<mz_t, int_t>, mz_t, int_t>(centroids, xData[i], yData[i]);
				problem.AddResidualBlock( costFunction, NULL, &data[0]);
			}

			/** solve the problem */
			ceres::Solver::Summary summary;
			ceres::Solve(options, &problem, &summary);


			/*if (summary.termination_type == ceres::NO_CONVERGENCE) {
				printf("That's sucks\n");
			}*/
			/** fill the result values*/
			for (size_t i = 0; i < centroids.size(); ++i) {
                auto c = centroids[i];

				double intensity = data[i*3];
				double lwhm = (data[i*3+1] * SIGMA_FACTOR) / 2.0;
				double rwhm = (data[i*3+2] * SIGMA_FACTOR) / 2.0;

				//boundConstraints
				//100ppm
                double maxhwhm = (c->mz / (c->mz * 100.0)) / 2.0;

                if ( isFiniteNumber<double>(lwhm) && lwhm > 0 && abs(lwhm - c->lwhm) < (c->lwhm * 0.5) && lwhm < maxhwhm )
					c->lwhm = lwhm;

                if ( isFiniteNumber<double>(rwhm) && rwhm > 0 && abs(rwhm - c->rwhm) < (c->rwhm * 0.5 ) && rwhm < maxhwhm)
					c->rwhm = rwhm;

                //if (centroids.size() > 1) {
                if (isFiniteNumber<double>(intensity) && intensity > 0 && abs(intensity - c->intensity) < (c->intensity * 0.5))
					c->intensity = intensity;
				//}*/
				output.push_back(c);
			}
        }
        //centroids.clear();
    }

    static ceres::Solver::Options getDefaultOptions() {
          ceres::Solver::Options options;
          options.max_num_iterations = 50;
          //options.num_threads = 2;//threads for the jacobian evaluation
          //options.function_tolerance = 1e-10;
          //options.minimizer_type = ceres::LINE_SEARCH;
          //options.line_search_direction_type = ceres::NONLINEAR_CONJUGATE_GRADIENT;
          //options.trust_region_strategy_type = ceres::DOGLEG;
          //options.num_linear_solver_threads = 2; // used for sparse_schur(suitesparse) linear solver
          //options.sparse_linear_algebra_library = ceres::CX_SPARSE;
          //options.linear_solver_type = ceres::DENSE_NORMAL_CHOLESKY;
          return options;
      }

   /** Test if a value is different from NaN and -INF and +INF*/
   template< typename T>
   inline bool static isFiniteNumber(T& x) {
      return (x <= DBL_MAX && x >= -DBL_MAX);
   }
};

/*template<class mz_t, class int_t>
class EigenLMSolver {

    vector<Centroid<mz_t, int_t>* > centroids;
    const vector<mz_t> xData;
    const vector<int_t> yData;

public:

    inline EigenLMSolver(const vector<mz_t>& x_,
                              const vector<int_t>& y_,
                              vector<Centroid<mz_t, int_t>* >& centroids_): centroids(centroids_), xData(x_), yData(y_) {
    }


   void solve(vector<Centroid<mz_t, int_t>* >& output) {


       const int n = centroids.size() * 3;
       Eigen::VectorXd data(n);

        for (size_t i = 0; i < centroids.size(); ++i) {
            Centroid<mz_t, int_t>* c = centroids[i];
            double intensity = c->intensity;
            double lwhm = c->lwhm;
            double rwhm = c->rwhm;
            data << intensity, (lwhm * 2.0) / SIGMA_FACTOR, (rwhm * 2.0) / SIGMA_FACTOR;
        }
        printf("after filling data\n");

        EigenGaussianOptimizer<mz_t, int_t> eigenGaussianOpt(xData, yData, centroids);
        //Eigen::NumericalDiff<EigenGaussianOptimizer<mz_t, int_t> > numDiff(eigenGaussianOpt);
        Eigen::LevenbergMarquardt<EigenGaussianOptimizer<mz_t, int_t> > lm(eigenGaussianOpt);

        printf("before solveNumericalDiff\n");
        int info = lm.minimize(data);
        printf("after solveNumericalDiff\n");

        for (size_t i = 0; i < centroids.size(); ++i) {
            Centroid<mz_t, int_t>* c = centroids[i];
            const double& intensity = data(i*3);
            const double& lwhm = (data(i * 3 + 1) * SIGMA_FACTOR) / 2.0;
            const double& rwhm = (data(i * 3 + 2) * SIGMA_FACTOR) / 2.0;

            //boundConstraints
            //100ppm
            double maxhwhm = (c->mz / (c->mz * 100.0)) / 2.0;

            if ( isFiniteNumber<double>(lwhm) && lwhm > 0 && abs(lwhm - c->lwhm) < (c->lwhm * 0.5) && lwhm < maxhwhm )
                c->lwhm = lwhm;

            if ( isFiniteNumber<double>(rwhm) && rwhm > 0 && abs(rwhm - c->rwhm) < (c->rwhm * 0.5 ) && rwhm < maxhwhm)
                c->rwhm = rwhm;

            if (isFiniteNumber<double>(intensity) && intensity > 0 && abs(intensity - c->intensity) < (c->intensity * 0.5))
                c->intensity = intensity;
            output.push_back(c);
        }
        //centroids.clear();
    }

   template< typename T>
   inline bool static isFiniteNumber(const T& x) {
      return (x <= DBL_MAX && x >= -DBL_MAX);
   }
};*/


} //end mzdb

#endif // MZBUILDFITTINGPROBLEM_HPP
