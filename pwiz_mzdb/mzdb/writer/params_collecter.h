#ifndef MZPARAMSCOLLECTER_H
#define MZPARAMSCOLLECTER_H

#include "unordered_map"

#include "pwiz/data/msdata/MSData.hpp"

#include "../utils/mzDBFile.h"

using namespace std;

namespace mzdb {

/**
 * The mzParamsCollecter class
 * ===========================
 *
 * Allow the collect of user param and cv params of pwiz ParamContainer objects
 *
 * @see DataContainer
 */
class mzParamsCollecter {
private:

    /// map cvid assuming they are unique to the cvParam object itself
    unordered_map<string, pwiz::msdata::CVParam> m_cvids; //assume they are unique

    /// map user param name unique to the UserParam object itself
    unordered_map<string, pwiz::msdata::UserParam> m_userParamsByName; // the same

    /// reference to the mzDBFile which provides sqlite3 stmt and db structures
    MzDBFile& m_mzdbFile;

public:

    /**
     * Ctor
     *
     * @param mzdbFile
     */
    mzParamsCollecter(MzDBFile& mzdbFile): m_mzdbFile(mzdbFile) {}

    /**
     * fetch new cvParam (that is not already in the map) from a ParamContainer
     *
     * @param pc
     */
    inline void  updateCVMap(const pwiz::msdata::ParamContainer& pc) {
        for (auto cvparam = pc.cvParams.begin(); cvparam != pc.cvParams.end(); ++cvparam) {
            m_cvids[(*cvparam).name()] = *cvparam;
        }
    }

    /**
     * fetch new userParam (that is not already in the map) from a ParamContainer
     *
     * @param pc
     */
    inline void updateUserMap(const pwiz::msdata::ParamContainer& pc) {
        for (auto userparam = pc.userParams.begin(); userparam != pc.userParams.end(); ++userparam) {
            m_userParamsByName[(*userparam).name] = *userparam;
        }
    }

    /**
     * Insert all cvParams and userParams object in cv_params and user_params table respectively
     * in the sqlite database.
     */
    void insertCollectedCVTerms();

};

} // end namespace mzdb

#endif // MZPARAMSCOLLECTER_H
