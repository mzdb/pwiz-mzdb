#ifndef MZPARAMSCOLLECTER_H
#define MZPARAMSCOLLECTER_H

#include "unordered_map"

#include "pwiz/data/msdata/MSData.hpp"

#include "../utils/mzDBFile.h"

using namespace std;

namespace mzdb {

class mzParamsCollecter {
private:
    unordered_map<string, pwiz::msdata::CVParam> m_cvids; //assume they are unique
    unordered_map<string, pwiz::msdata::UserParam> m_userParamsByName; // the same
    MzDBFile& m_mzdbFile;

public:

    mzParamsCollecter(MzDBFile& mzdbFile): m_mzdbFile(mzdbFile) {}

    inline void  updateCVMap(const pwiz::msdata::ParamContainer& pc) {
       for (auto cvparam = pc.cvParams.begin(); cvparam != pc.cvParams.end(); ++cvparam) {
           m_cvids[(*cvparam).name()] = *cvparam;
       }
    }

    inline void updateUserMap(const pwiz::msdata::ParamContainer& pc) {
       for (auto userparam = pc.userParams.begin(); userparam != pc.userParams.end(); ++userparam) {
           m_userParamsByName[(*userparam).name] = *userparam;
       }

    }

    void insertCollectedCVTerms();

};

} // end namespace mzdb

#endif // MZPARAMSCOLLECTER_H
