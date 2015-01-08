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

//template< class MetadataExtractorPolicy >
class mzSpectrumInserter {


private:

    /* can have different way to collect cv and user params */
    mzParamsCollecter& m_paramsCollecter;

    MzDBFile& m_mzdbFile;

    /* raw file format currently treated */
    pwiz::msdata::CVID m_rawFileFormat;

    pwiz::vendor_api::ABI::WiffFilePtr m_wiffFile;

    map<int, DataMode>& m_dataModeByMsLevel;

    map<DataMode, int> m_dataModePosition;


public:

    /* constructor, object will hold a reference to the unique paramsCollecter */
    mzSpectrumInserter(MzDBFile& mzdb,  mzParamsCollecter& pc,
                                   pwiz::msdata::CVID rawFileFormat,
                                   map<int, DataMode>& dataModeByMsLevel) :
        m_mzdbFile(mzdb),
        m_paramsCollecter(pc),
        m_rawFileFormat(rawFileFormat),
        m_dataModeByMsLevel(dataModeByMsLevel) {

        m_dataModePosition[PROFILE] = 1;
        m_dataModePosition[FITTED] = 2 ;
        m_dataModePosition[CENTROID] = 3;


        if (m_rawFileFormat == pwiz::msdata::MS_ABI_WIFF_format)
            m_wiffFile = pwiz::vendor_api::ABI::WiffFile::create(m_mzdbFile.name);
    }

    template<typename mz_t, typename int_t, typename h_mz_t, typename h_int_t>
    void insertScan(std::shared_ptr<mzSpectrum<mz_t, int_t> >& spectrum,
                           int idxInCycle,                                                                    //needed by ABI low level API
                           int bbFirstScanId,                                                               // id of the first scan of the bounding box
                           std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> >& parentSpectrum,
                           pwiz::msdata::MSDataPtr msdata,
                           ISerializer::xml_string_writer& serializer) {

        // should never happen
        if (! spectrum) {
            LOG(ERROR) << "ERROR...spectrum pointer is null\n";
            return;
        }

        const auto& spec = spectrum->spectrum;

        // update spectra cv params
        m_paramsCollecter.updateCVMap(*spec);
        m_paramsCollecter.updateUserMap(*spec);

        // update for cvparams of scans
        // TODO: crash if scans vector does not contain any scan !
        const pwiz::msdata::Scan& scan = spec->scanList.scans[0];
        m_paramsCollecter.updateCVMap(scan);
        m_paramsCollecter.updateUserMap(scan);

        // case thermo file
        double highResPrecMz = 0.0;
        if (m_rawFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
            highResPrecMz = scan.userParam(THERMO_TRAILER).valueAs<double>();
        }

        const int& msLevel = spectrum->msLevel();
        const char* sql = "INSERT INTO spectrum  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(m_mzdbFile.db, sql, -1, &(m_mzdbFile.stmt), 0);

        sqlite3_bind_int(m_mzdbFile.stmt, 1, spectrum->id);

        //initial_id
        sqlite3_bind_int(m_mzdbFile.stmt, 2, spectrum->id);
        const string& id = spec->id;
        //title
        sqlite3_bind_text(m_mzdbFile.stmt, 3, id.c_str(), id.length(), SQLITE_TRANSIENT);
        //cycle
        sqlite3_bind_int(m_mzdbFile.stmt, 4, spectrum->cycle);
        //time
        sqlite3_bind_double(m_mzdbFile.stmt, 5, PwizHelper::rtOf(spec));
        //msLevel
        sqlite3_bind_int(m_mzdbFile.stmt, 6, msLevel);

        //activation_type
        if (msLevel > 1) {
            const string activationCode = mzdb::getActivationCode(spec->precursors.front().activation);
            sqlite3_bind_text(m_mzdbFile.stmt, 7, activationCode.c_str(), activationCode.length(), SQLITE_TRANSIENT);
        } else {
            // in the model shoud not be null -> use an empty string
            sqlite3_bind_text(m_mzdbFile.stmt, 7, "", 0, SQLITE_STATIC);
        }

        //tic
        sqlite3_bind_double(m_mzdbFile.stmt, 8, spec->cvParam(pwiz::msdata::MS_total_ion_current).valueAs<float>());
        //base peak mz
        sqlite3_bind_double(m_mzdbFile.stmt, 9, spec->cvParam(pwiz::msdata::MS_base_peak_m_z).valueAs<double>());
        //base peak intensity
        sqlite3_bind_double(m_mzdbFile.stmt, 10, spec->cvParam(pwiz::msdata::MS_base_peak_intensity).valueAs<float>());

        //precursor mz precursor charge
        if (msLevel > 1) {
            //clear the selected ions
            // just add some metadata, PIP and nearest prec mz
//            auto& selectedIons = spec->precursors.front().selectedIons;
//            pwiz::msdata::SelectedIon si = spectrum->refinedPrecursorMz(parentSpectrum, _emptyPrecCount);
//            bool siEmpty = si.empty();
//            if (! siEmpty)
//                selectedIons.push_back(si);

            //seems to be buggy
            //spectrum->refinedPrecursorMzPwiz(parentSpectrum, selectedIons);
            if (highResPrecMz)
                sqlite3_bind_double(m_mzdbFile.stmt, 11, spectrum->precursorMz());
//            else {
//                if (! siEmpty)
//                    sqlite3_bind_double(m_mzdbFile.stmt, 11, si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>());
                else
                    sqlite3_bind_double(m_mzdbFile.stmt, 11, spectrum->precursorMz());

            //}
            //---charge
            /*if (m_rawFileFormat == pwiz::msdata::MS_ABI_WIFF_format) {

                int charge = 0;
                double mz = 0.0, intensity = 0.0;
                auto abiSpec = m_wiffFile->getSpectrum(1, 1, idxInCycle, spectrum->cycle);
                if (abiSpec->getHasPrecursorInfo()) {
                    abiSpec->getPrecursorInfo(mz, intensity, charge);
                } else
                    printf("has prec info return false");

                sqlite3_bind_double(m_mzdbFile.stmt, 12, charge);

            } else*/
            sqlite3_bind_double(m_mzdbFile.stmt, 12, PwizHelper::precursorChargeOf(spec));

        } else {
            sqlite3_bind_null(m_mzdbFile.stmt, 11);
            sqlite3_bind_null(m_mzdbFile.stmt, 12);
        }

        //datapointscount
        sqlite3_bind_int(m_mzdbFile.stmt, 13, spectrum->nbPeaks());

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
        sqlite3_bind_text(m_mzdbFile.stmt, 14, r.c_str(), r.length(), SQLITE_TRANSIENT);
        // ---Here we use directly the pwiz api for writing xml chunks
        //scan list
        ostringstream os_2;
        pwiz::minimxml::XMLWriter writer_2(os_2);
        pwiz::msdata::IO::write(writer_2, spec->scanList, *msdata);
        string r_2 = os_2.str();
        sqlite3_bind_text(m_mzdbFile.stmt, 15, r_2.c_str(), r_2.length(), SQLITE_TRANSIENT);

        //precursor list
        if (msLevel > 1) {
            ostringstream os_3;
            pwiz::minimxml::XMLWriter writer_3(os_3);
            for (auto p = spec->precursors.begin(); p != spec->precursors.end(); ++p) {
                pwiz::msdata::IO::write(writer_3, *p);
            }
            string r_3 =  os_3.str();
            sqlite3_bind_text(m_mzdbFile.stmt, 16, r_3.c_str(), r_3.length(), SQLITE_TRANSIENT);

            //product list
            ostringstream os_4;
            pwiz::minimxml::XMLWriter writer_4(os_4);
            for (auto p = spec->products.begin(); p != spec->products.end(); ++p) {
                pwiz::msdata::IO::write(writer_4, *p);
            }
            string r_4 = os_4.str();
            sqlite3_bind_text(m_mzdbFile.stmt, 17, r_4.c_str(), r_4.length(), SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(m_mzdbFile.stmt, 16);
            sqlite3_bind_null(m_mzdbFile.stmt, 17);
        }

        //shared param tree id
        sqlite3_bind_null(m_mzdbFile.stmt, 18);
        //instrument config id
        sqlite3_bind_int(m_mzdbFile.stmt, 19, instrumentConfigurationIndex( msdata, spec->scanList.scans[0].instrumentConfigurationPtr));
        //source file id
        sqlite3_bind_int(m_mzdbFile.stmt, 20, sourceFileIndex(msdata, spec->sourceFilePtr));
        //run id
        sqlite3_bind_int(m_mzdbFile.stmt, 21, 1); //run id default to 1
        //data proc id
        sqlite3_bind_int(m_mzdbFile.stmt, 22, dataProcessingIndex(msdata, spec->dataProcessingPtr));
        //data enc id
        //TODO: fix this we like before
        sqlite3_bind_int(m_mzdbFile.stmt, 23, m_dataModePosition[spectrum->getEffectiveMode(m_dataModeByMsLevel[msLevel])]);
        //bb first spec id
        sqlite3_bind_int(m_mzdbFile.stmt, 24, bbFirstScanId);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_finalize(m_mzdbFile.stmt);

        //safe reinit
        m_mzdbFile.stmt = 0;
    }

    //insert several scans
    template<class h_mz_t, class h_int_t,
                  class l_mz_t, class l_int_t >
    void insertScans(unique_ptr<mzSpectraContainer<h_mz_t, h_int_t, l_mz_t, l_int_t> >& cycleObject,
                            pwiz::msdata::MSDataPtr msdata,
                            ISerializer::xml_string_writer& serializer,
                            int bbFirstScanID=0) {

        //to handle swath cycle
        int i = 1;
        auto& parentSpectrum = cycleObject->parentSpectrum;
        if (parentSpectrum && parentSpectrum->isInserted == false && bbFirstScanID) {
            this->insertScan<h_mz_t, h_int_t>(parentSpectrum, i, bbFirstScanID, parentSpectrum, msdata, serializer);
            ++i;
        }
        auto& spectra = cycleObject->spectra;
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {
            if (it->first != nullptr) {
                this->insertScan<h_mz_t, h_int_t, h_mz_t, h_int_t>
                    (it->first, i, cycleObject->getBeginId(),
                     cycleObject->parentSpectrum, msdata, serializer);
            } else {
                this->insertScan<l_mz_t, l_int_t, h_mz_t, h_int_t>
                    (it->second, i, cycleObject->getBeginId(),
                     cycleObject->parentSpectrum, msdata, serializer);
            }
            ++i;
        }
    }

//----------------------------------------------
    int instrumentConfigurationIndex(pwiz::msdata::MSDataPtr msdata,
                                                    const pwiz::msdata::InstrumentConfigurationPtr & ic) const {
        const vector<pwiz::msdata::InstrumentConfigurationPtr>& insconfs = msdata->instrumentConfigurationPtrs;
        int pos = std::find(insconfs.begin(), insconfs.end(), ic) - insconfs.begin();
        return pos + 1;
    }


    int dataProcessingIndex(pwiz::msdata::MSDataPtr msdata,
                                        const pwiz::msdata::DataProcessingPtr & dp) const {
        if (!dp)
            return 1; //by default pointer to scanProcess;
        const vector<pwiz::msdata::DataProcessingPtr>& dataProcessings = msdata->allDataProcessingPtrs();
        int pos = std::find(dataProcessings.begin(), dataProcessings.end(), dp) - dataProcessings.begin();
        return pos + 1;
    }

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
