#include "../lib/sqlite3/include/sqlite3.h"

#include "params_collecter.h"

using namespace std;

namespace mzdb {

void mzParamsCollecter::insertCollectedCVTerms() {

    /***********************************************************************
    //controlled vocabulary
    ***********************************************************************/
    sqlite3_exec(m_mzdbFile.db, "INSERT INTO cv VALUES ('psi_ms', 'PSI Mass spectrometry', '3.60.0','http://psidev.cvs.sourceforge.net/viewvc/psidev/psi/psi-ms/mzML/controlledVocabulary/psi-ms.obo?revision=1.196');", 0, 0, 0);
    m_mzdbFile.stmt = 0;

    /***********************************************************************
    //cv_term
    ***********************************************************************/
    vector<pwiz::msdata::CVID> units;
    sqlite3_prepare_v2(m_mzdbFile.db, "INSERT INTO cv_term VALUES (?, ?, ?, ?);", -1, &(m_mzdbFile.stmt), 0);
    for (auto cvid = m_cvids.begin(); cvid != m_cvids.end(); ++cvid) {
        const pwiz::msdata::CVParam& cvparam = cvid->second;

        if ( find(units.begin(), units.end(), cvparam.units) == units.end())
            units.push_back(cvparam.units);

        pwiz::cv::CVTermInfo termInfo = pwiz::cv::cvTermInfo(cvparam.cvid);
        string& accession = termInfo.id;
        string& name = termInfo.name;
        string unitAccession = cvparam.unitsName();
        sqlite3_bind_text(m_mzdbFile.stmt, 1, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 3, unitAccession.c_str(), unitAccession.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 4, "psi_ms", 6, SQLITE_STATIC);
        //step then reset
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    /***********************************************************************
    //user_term
    ***********************************************************************/
    sqlite3_prepare_v2(m_mzdbFile.db, "INSERT INTO user_term VALUES (NULL, ?, ?, ?);", -1, &(m_mzdbFile.stmt), 0);
    for (auto pair = m_userParamsByName.begin(); pair != m_userParamsByName.end(); ++pair) {
        const pwiz::msdata::UserParam& userparam = pair->second;

        if ( find(units.begin(), units.end(), userparam.units) == units.end())
            units.push_back(userparam.units);

        const string& name = pair->first;
        const string& type = userparam.type;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, type.c_str(), type.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 3, "", 0, SQLITE_STATIC);
        //step then reset
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    /***********************************************************************
     *CV UNIT
     **********************************************************************/
    sqlite3_prepare_v2(m_mzdbFile.db, "INSERT INTO cv_unit VALUES (?, ?, ?);", -1, &(m_mzdbFile.stmt), 0);
    for (auto cvid = units.begin(); cvid != units.end(); cvid ++) {
        const pwiz::msdata::CVTermInfo& termInfo = pwiz::cv::cvTermInfo(*cvid);
        const string& accession = termInfo.id;
        const string& name = termInfo.name;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 3, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

}

}
