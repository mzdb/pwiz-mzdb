#ifndef FITTEDTOPROFILE_H
#define FITTEDTOPROFILE_H

#include <math.h>

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/data/msdata/ChromatogramListBase.hpp"

#include "../reader/msdata/mzScan.hpp"
#include "../utils/MzDBFile.h"
#include "BinnedSpectrum.h"



namespace mzdb {


struct mzEmptyChromatogram: public pwiz::msdata::ChromatogramListBase {
    pwiz::msdata::ChromatogramIdentity ci;
    mzEmptyChromatogram(){}
    size_t size() const {return 0;}
    bool empty() const { return true;}
    const pwiz::msdata::ChromatogramIdentity& chromatogramIdentity(size_t index) const {
        return ci;
    }
    pwiz::msdata::ChromatogramPtr chromatogram(size_t index, bool getBinaryData = false) const {
        pwiz::msdata::ChromatogramPtr c(new pwiz::msdata::Chromatogram);
        return c;
    }
};


void mzDBMSDataTomzMLMSData(MzDBFile *f, pwiz::msdata::MSDataPtr, pwiz::msdata::MSDataPtr);

void clearScanData(mzScan*);

void integrate(const vector<pair<double, double> >&, double&);
}

#endif // FITTEDTOPROFILE_H
