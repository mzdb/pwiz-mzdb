#ifndef MZSPECTRUMINSERTER_H
#define MZSPECTRUMINSERTER_H

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/data/msdata/IO.hpp"
#include "pwiz_aux/msrc/utility/vendor_api/ABI/WiffFile.hpp"

#include "../lib/sqlite3/include/sqlite3.h"

#include "../utils/mzDBFile.h"
#include "params_collecter.h"
#include "spectrum.hpp"
#include "../utils/mzISerializer.h"


namespace mzdb {

/**
 * The mzSpectrumInserter class
 * =============================
 *
 * Insert spectrum into mzDB file
 */
class mzSpectrumInserter {

private:

    /* can have different way to collect cv and user params */
    /// object that collect cv params and use params from spectrum objects
    /// and spectrum child objects
    mzParamsCollecter& mParamsCollecter;

    /// mzDBFile provding sqlite objects statement and database
    MzDBFile& mMzdbFile;

    /// raw file format currently treated
    pwiz::msdata::CVID mRawFileFormat;

    /// pointer to the raw wiff file
    /// null if not a wiff file
    pwiz::vendor_api::ABI::WiffFilePtr mWiffFile;

    /// map mslevel dataMode
    /// @see DataMode
    map<int, DataMode>& mDataModeByMsLevel;

    /// map db id, DataEncoding, useful to retrieve db data encoding id
    /// @see DataEncoding
    map<int, DataEncoding>& mDataEncodingByID;

    /**
     * @brief findDataEncodingID
     * Helping function to retrieve good data encoding id for a given
     * DataMode
     *
     * @param mode
     * @param inHighRes
     * @param noLoss
     * @return
     */
    inline int findDataEncodingID(DataMode mode, bool inHighRes, bool noLoss=false) {
        PeakEncoding pe = inHighRes ? HIGH_RES_PEAK : LOW_RES_PEAK;
        if (inHighRes && noLoss)
            pe = NO_LOSS_PEAK;

        for (auto it = mDataEncodingByID.begin(); it != mDataEncodingByID.end(); ++it) {
            DataEncoding de = it->second;
            if (de.mode == mode && pe == de.peakEncoding)
                return it->first;
        }
        return 1;
        //throw exception("Can not determine the good data encoding ID");
    }

public:

    /**
     * @brief mzSpectrumInserter
     *
     * @param mzdb
     * @param pc
     * @param rawFileFormat
     * @param dataModeByMsLevel
     * @param dataEncodingByID
     */
    mzSpectrumInserter(MzDBFile& mzdb,
                       mzParamsCollecter& pc,
                       pwiz::msdata::CVID rawFileFormat,
                       map<int, DataMode>& dataModeByMsLevel,
                       map<int, DataEncoding>& dataEncodingByID) :

        mMzdbFile(mzdb),
        mParamsCollecter(pc),
        mRawFileFormat(rawFileFormat),
        mDataModeByMsLevel(dataModeByMsLevel),
        mDataEncodingByID(dataEncodingByID) {

        if (mRawFileFormat == pwiz::msdata::MS_ABI_WIFF_format)
            mWiffFile = pwiz::vendor_api::ABI::WiffFile::create(mMzdbFile.name);
    } // end ctor

    /**
     * @brief insertScan
     * Insert spectrum object into the mzDB file
     */
    template<typename mz_t, typename int_t, typename h_mz_t, typename h_int_t>
    void insertScan(std::shared_ptr<mzSpectrum<mz_t, int_t> >& spectrum,
                    int idxInCycle,  //needed by ABI low level API
                    int bbFirstScanId, // id of the first scan of the bounding box
                    std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> >& parentSpectrum,
                    pwiz::msdata::MSDataPtr msdata,
                    ISerializer::xml_string_writer& serializer) {

        // should never happen
        if (! spectrum || spectrum == nullptr) {
            LOG(ERROR) << "ERROR...spectrum pointer is null";
            return;
        }

        const auto& spec = spectrum->spectrum;

        if (! spec || spec == nullptr) {
            LOG(ERROR) << "null pwiz spectrum. Recovering has failed";
            return;
        }

        // update spectra cv params
        mParamsCollecter.updateCVMap(*spec);
        mParamsCollecter.updateUserMap(*spec);

        // fast version: crash if scans vector does not contain any scan !
        //const pwiz::msdata::Scan& scan = spec->scanList.scans[0];
        pwiz::msdata::Scan scan;
        try {
            scan = spec->scanList.scans.at(0);
        } catch (exception&) {
            LOG(ERROR) << "Empty scan vector, spectrum id:" << spectrum->id << ". Skipping it...";
            return;
        }

        // update for cvparams of scans
        mParamsCollecter.updateCVMap(scan);
        mParamsCollecter.updateUserMap(scan);

        // mslevel
        const int& msLevel = spectrum->msLevel();

        // Prepare stmt
        const char* sql = "INSERT INTO tmp_spectrum  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(mMzdbFile.db, sql, -1, &(mMzdbFile.stmt), 0);

        //ID could be autoincrement
        sqlite3_bind_int(mMzdbFile.stmt, 1, spectrum->id);

        //initial_id
        sqlite3_bind_int(mMzdbFile.stmt, 2, spectrum->id);
        const string& id = spec->id;

        //title
        sqlite3_bind_text(mMzdbFile.stmt, 3, id.c_str(), id.length(), SQLITE_STATIC);

        //cycle
        sqlite3_bind_int(mMzdbFile.stmt, 4, spectrum->cycle);

        //time
        sqlite3_bind_double(mMzdbFile.stmt, 5, PwizHelper::rtOf(spec));

        //msLevel
        sqlite3_bind_int(mMzdbFile.stmt, 6, msLevel);

        //activation_type
        string activationCode;
        if (msLevel > 1) {
            activationCode = mzdb::getActivationCode(spec->precursors.front().activation);
            sqlite3_bind_text(mMzdbFile.stmt, 7, activationCode.c_str(), activationCode.length(), SQLITE_STATIC);
        } else {
            // in the sql model shoud not be null -> use an empty string
            sqlite3_bind_text(mMzdbFile.stmt, 7, "", 0, SQLITE_STATIC);
        }

        //tic
        try {
            sqlite3_bind_double(mMzdbFile.stmt, 8, spec->cvParam(pwiz::msdata::MS_total_ion_current).valueAs<float>());
        } catch (boost::bad_lexical_cast&) {
            LOG(ERROR) << "Wrong cv value: MS_total_ion_current";
            sqlite3_bind_double(mMzdbFile.stmt, 8, 0.0);
        }

        //base peak mz
        try {
            sqlite3_bind_double(mMzdbFile.stmt, 9, spec->cvParam(pwiz::msdata::MS_base_peak_m_z).valueAs<double>());
        } catch (boost::bad_lexical_cast&) {
            LOG(ERROR) << "Wrong cv value MS_base_peak_mz: " << spec->cvParam(pwiz::msdata::MS_base_peak_m_z);
            sqlite3_bind_double(mMzdbFile.stmt, 9, 0.0);
        }

        //base peak intensity
        try {
            sqlite3_bind_double(mMzdbFile.stmt, 10, spec->cvParam(pwiz::msdata::MS_base_peak_intensity).valueAs<float>());
        } catch (boost::bad_lexical_cast&) {
            LOG(ERROR) << "Wrong cv value: MS_base_peak_intensity";
            sqlite3_bind_double(mMzdbFile.stmt, 10, 0.0);
        }

        //precursor mz precursor charge
        if (msLevel > 1) {
            /*
            //clear the selected ions
            // just add some metadata, PIP and nearest prec mz
            auto& selectedIons = spec->precursors.front().selectedIons;
            pwiz::msdata::SelectedIon si = spectrum->refinedPrecursorMz(parentSpectrum, _emptyPrecCount);
            bool siEmpty = si.empty();
            if (! siEmpty)
                selectedIons.push_back(si);

            seems to be buggy
            spectrum->refinedPrecursorMzPwiz(parentSpectrum, selectedIons);
            if (highResPrecMz)
                sqlite3_bind_double(mMzdbFile.stmt, 11, spectrum->precursorMz());
            else {
                if (! siEmpty)
                    sqlite3_bind_double(m_mzdbFile.stmt, 11, si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>());
                else
                    sqlite3_bind_double(mMzdbFile.stmt, 11, spectrum->precursorMz());

            //}
            //---charge
            if (m_rawFileFormat == pwiz::msdata::MS_ABI_WIFF_format) {

                int charge = 0;
                double mz = 0.0, intensity = 0.0;
                auto abiSpec = m_wiffFile->getSpectrum(1, 1, idxInCycle, spectrum->cycle);
                if (abiSpec->getHasPrecursorInfo()) {
                    abiSpec->getPrecursorInfo(mz, intensity, charge);
                } else
                    printf("has prec info return false");

                sqlite3_bind_double(m_mzdbFile.stmt, 12, charge);

            } else
            */

            double precMz = 0.0;

            // case thermo file
            if (mRawFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
                precMz = scan.userParam(THERMO_TRAILER).valueAs<double>();
                if (! precMz)
                    precMz = PwizHelper::precursorMzOf(spec);
            } else {
                precMz = PwizHelper::precursorMzOf(spec);
            }
            sqlite3_bind_double(mMzdbFile.stmt, 11, precMz);
            sqlite3_bind_double(mMzdbFile.stmt, 12, PwizHelper::precursorChargeOf(spec));

        } else {
            sqlite3_bind_null(mMzdbFile.stmt, 11);
            sqlite3_bind_null(mMzdbFile.stmt, 12);
        }

        //datapointscount
        sqlite3_bind_int(mMzdbFile.stmt, 13, spectrum->nbPeaks());

        //spectrum paramtree
        //TODO: put a new user param to evaluate the resolution of a spectrum
        bool isInHighRes =  spectrum->isInHighRes;
        pwiz::msdata::UserParam p(IN_HIGH_RES_STR); p.type = XML_BOOLEAN;
        if (isInHighRes) {
            p.value = TRUE_STR;
        } else {
            p.value = FALSE_STR;
        }
        spec->userParams.push_back(p);
        string& r = ISerializer::serialize(*spec, serializer);
        sqlite3_bind_text(mMzdbFile.stmt, 14, r.c_str(), r.length(), SQLITE_STATIC);

        //---Here we use directly the pwiz api for writing xml chunks

        //scan list
        ostringstream os_2;
        pwiz::minimxml::XMLWriter writer_2(os_2);
        pwiz::msdata::IO::write(writer_2, spec->scanList, *msdata);
        string r_2 = os_2.str();
        sqlite3_bind_text(mMzdbFile.stmt, 15, r_2.c_str(), r_2.length(), SQLITE_STATIC);

        //precursor list
        string r_3, r_4;

        if (msLevel > 1) {
            ostringstream os_3;
            pwiz::minimxml::XMLWriter writer_3(os_3);
            for (auto p = spec->precursors.begin(); p != spec->precursors.end(); ++p) {
                pwiz::msdata::IO::write(writer_3, *p);
            }
            r_3 =  os_3.str();
            sqlite3_bind_text(mMzdbFile.stmt, 16, r_3.c_str(), r_3.length(), SQLITE_STATIC);

            //product list
            ostringstream os_4;
            pwiz::minimxml::XMLWriter writer_4(os_4);
            for (auto p = spec->products.begin(); p != spec->products.end(); ++p) {
                pwiz::msdata::IO::write(writer_4, *p);
            }
            r_4 = os_4.str();
            if (r_4.empty()) sqlite3_bind_null(mMzdbFile.stmt, 17);
            else
                sqlite3_bind_text(mMzdbFile.stmt, 17, r_4.c_str(), r_4.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(mMzdbFile.stmt, 16);
            sqlite3_bind_null(mMzdbFile.stmt, 17);
        }

        //shared param tree id
        if (mMzdbFile.sharedParamTreeID)
            sqlite3_bind_int(mMzdbFile.stmt, 18, mMzdbFile.sharedParamTreeID);
        else
            sqlite3_bind_null(mMzdbFile.stmt, 18);

        //instrument config id
        sqlite3_bind_int(mMzdbFile.stmt, 19, 1); //instrumentConfigurationIndex(msdata, scan.instrumentConfigurationPtr));

        //source file id
        //previous source file was leading to an error when FOREIGN KEY constraint enable
        //force it to 1 as one sourcefile is mandatory for mzML validation
        sqlite3_bind_int(mMzdbFile.stmt, 20, 1); //sourceFileIndex(msdata, spec->sourceFilePtr));

        //run id
        //run id default to 1
        sqlite3_bind_int(mMzdbFile.stmt, 21, 1);

        //data proc id
        sqlite3_bind_int(mMzdbFile.stmt, 22, 1); //dataProcessingIndex(msdata, spec->dataProcessingPtr));

        //data enc id
        DataMode effectiveMode = spectrum->getEffectiveMode(mDataModeByMsLevel[msLevel]);
        sqlite3_bind_int(mMzdbFile.stmt, 23, this->findDataEncodingID(effectiveMode, isInHighRes, mMzdbFile.noLoss));

        //bb first spec id
        sqlite3_bind_int(mMzdbFile.stmt, 24, bbFirstScanId);

        //finalize statement
        sqlite3_step(mMzdbFile.stmt);
        sqlite3_finalize(mMzdbFile.stmt);
        mMzdbFile.stmt = 0;
    }

    //insert several scans
    /**
     * @brief insertScans
     * Insert all scans of a cycle (represented as mzSpectraContainer)
     */
    template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t >
    void insertScans(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject,
                     pwiz::msdata::MSDataPtr msdata,
                     ISerializer::xml_string_writer& serializer,
                     int bbFirstScanIDMS1=0,
                     int bbFirstScanIDMS2=0) {

        //to handle swath cycle
        int i = 1;
        auto& parentSpectrum = cycleObject->parentSpectrum;
        if (parentSpectrum && parentSpectrum->isInserted == false && bbFirstScanIDMS1) {
            this->insertScan<h_mz_t, h_int_t>(parentSpectrum, i, bbFirstScanIDMS1, parentSpectrum, msdata, serializer);
            ++i;
        }
        auto& spectra = cycleObject->spectra;
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {
            int bbfirstid = bbFirstScanIDMS2 ? bbFirstScanIDMS2 : cycleObject->getBeginId();

            if (it->first != nullptr) {
                this->insertScan<h_mz_t, h_int_t, h_mz_t, h_int_t>(it->first,
                                                                   i,
                                                                   bbfirstid, //cycleObject->getBeginId(),
                                                                   parentSpectrum,  //cycleObject->parentSpectrum,
                                                                   msdata,
                                                                   serializer);
            } else {
                this->insertScan<l_mz_t, l_int_t, h_mz_t, h_int_t>(it->second,
                                                                   i,
                                                                   bbfirstid, //cycleObject->getBeginId(),
                                                                   parentSpectrum,  //cycleObject->parentSpectrum,
                                                                   msdata,
                                                                   serializer);
            }
            ++i;
        }
    } //end insertScan


    //    int instrumentConfigurationIndex(pwiz::msdata::MSDataPtr msdata,
    //                                     const pwiz::msdata::InstrumentConfigurationPtr & ic) const {
    //        const vector<pwiz::msdata::InstrumentConfigurationPtr>& insconfs = msdata->instrumentConfigurationPtrs;
    //        int pos = std::find(insconfs.begin(), insconfs.end(), ic) - insconfs.begin();
    //        return pos;
    //    }


    //    int dataProcessingIndex(pwiz::msdata::MSDataPtr msdata,
    //                                        const pwiz::msdata::DataProcessingPtr & dp) const {
    //        if (!dp)
    //            return 1; //by default pointer to scanProcess;
    //        const vector<pwiz::msdata::DataProcessingPtr>& dataProcessings = msdata->allDataProcessingPtrs();
    //        int pos = std::find(dataProcessings.begin(), dataProcessings.end(), dp) - dataProcessings.begin();
    //        return pos + 1;
    //    }

    /**
     * @brief sourceFileIndex
     * Warning this is not working
     * @param msdata
     * @param sf
     * @return
     */
    int sourceFileIndex(pwiz::msdata::MSDataPtr msdata, const pwiz::msdata::SourceFilePtr & sf) const {
        const vector<pwiz::msdata::ScanSettingsPtr>& scanSettings = msdata->scanSettingsPtrs;
        int sourceFileCounter = 1;

        for (auto it = scanSettings.begin(); it != scanSettings.end(); ++it) {
            for (auto it_ = (*it)->sourceFilePtrs.begin(); it_ != (*it)->sourceFilePtrs.end(); ++it) {
                if (*it_ == sf) {
                    return sourceFileCounter;
                }
                sourceFileCounter++;
            }
        }
        return 1; //by default one sourcefile exist ;
    }

}; //end class


} // end namespace mzdb

#endif // MZSPECTRUMINSERTER_H
