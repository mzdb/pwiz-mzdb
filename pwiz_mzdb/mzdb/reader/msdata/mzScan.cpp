#include "mzScan.hpp"


namespace mzdb {
	using namespace std;



mzScan::mzScan(int id, int msLevel, float rt):
    pwiz::msdata::Spectrum(), idMzDB(id), msLevel(msLevel), rt(rt) {}

mzScan::mzScan() : pwiz::msdata::Spectrum() {}


 void mzScan::addScanData(mzScan* s) {
            if (!s) {
                printf("Invalid pointer\n");
				return;
			}
            mz.insert(mz.end(), s->mz.begin(), s->mz.end());
            intensities.insert(intensities.end(), s->intensities.begin(), s->intensities.end());
            if (! s->lwhm.empty() && ! s->rwhm.empty()) {
                lwhm.insert(lwhm.end(), s->lwhm.begin(), s->lwhm.end());
                rwhm.insert(rwhm.end(), s->rwhm.begin(), s->rwhm.end());
            }
        }

void mzScan::reduceTo(double& mzmin, double& mzmax) {
            
            currentMzmin = mzmin;
            currentMzmax = mzmax;

            vector<double>::iterator minit = find_if(mz.begin(), mz.end(), minComp(this));
            int minIndex = minit - mz.begin();
            vector<double>::iterator maxit = find_if(mz.begin(), mz.end(), maxComp(this));

            int maxIndex = maxit - mz.begin();
			mz.erase(mz.begin(), minit); 
			mz.erase(maxit, mz.end());
			intensities.erase(intensities.begin(), intensities.begin() + minIndex); 
			intensities.erase(intensities.begin() + maxIndex, intensities.end());
            
			//s->mz.insert(s->mz.end(), minit, maxit);
            //s->intensities.insert(s->intensities.end(), intensities.begin() + minIndex, intensities.begin() + maxIndex);

            if (!lwhm.empty() && !rwhm.empty()) {
                lwhm.erase(lwhm.begin(), lwhm.begin() + minIndex); 
				lwhm.erase(lwhm.begin() + maxIndex, lwhm.end());
				rwhm.erase(rwhm.begin(), rwhm.begin() + minIndex); 
				rwhm.erase(rwhm.begin() + maxIndex, rwhm.end());
            }
        }

}
