#ifndef __MZSCAN_HPP__
#define __MZSCAN_HPP__

#include "pwiz/data/msdata/MSData.hpp"
#include "../../utils/mzUtils.hpp"

using namespace std;

namespace mzdb {


    
    struct PWIZ_API_DECL mzScan : public pwiz::msdata::Spectrum {
        int idMzDB;
        int msLevel;
        float rt;
        DataEncoding encoding;

        vector<double> mz;
        vector<double> intensities;
        vector<float> lwhm; //empty in case of centroided or profile mode
        vector<float> rwhm; //empty in case of centroided or profile mode

        double currentMzmin, currentMzmax;

        void addScanData(mzScan* s);
        void reduceTo(double& mzmin, double& mzmax);

        mzScan();
        mzScan(int id, int msLevel, float rt);


        struct minComp {
            mzScan* scan;

            minComp(mzScan* s) : scan(s) {
            }

            inline bool operator() (double k) {
                return (k > scan->currentMzmin);
            }
        };

        struct maxComp {
            mzScan* scan;

            maxComp(mzScan* s) : scan(s) {
            }

            inline bool operator() (double k) {
                return (k > scan->currentMzmax);
            }
        };

       
    };

}

#endif
