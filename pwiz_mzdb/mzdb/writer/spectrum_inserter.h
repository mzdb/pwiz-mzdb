/*
 * Copyright 2014 CNRS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file spectrum_inserter.h
 * @brief Insert spectrum into mzDB file
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

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

#include <unordered_map>


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

    /// vector of DataEncoding items, used items will be inserted to the mzDB file
    /// @see DataEncoding
    vector<DataEncoding> mDataEncodings;

    // map of original modes per msLevel (what mode are in the input file)
    unordered_map<int, DataMode> originalModesByMsLevel;
    // map of effective modes per msLevel (what mode are really used)
    unordered_map<int, DataMode> effectiveModesByMsLevel;
    // map to know the number of spectra per ms level
    unordered_map<int, int> nbSpectraByMsLevel;

    // variables for a potential scan offset
    int scanOffset;
    bool isScanOffsetComputed;

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
        
        // loop on each possible data encoding
        for (auto d = mDataEncodings.begin(); d != mDataEncodings.end(); ++d) {
            DataEncoding de = (*d);
            // one and only one must match
            if(d->mode == mode && pe == d->peakEncoding) {
                // if it hasnt been inserted in the mzDB file yet, add it !
                if(!d->hasBeenInserted) {
                    sqlite3_exec(mMzdbFile.db, d->buildSQL().c_str(), 0, 0, 0);
                    // the result of sqlite3_exec is not tested because it says that it failed even when it succeeded :S
                    d->setHasBeenInserted(true);
                }
                return d->id;
            }
        }
        LOG(ERROR) << "Data encoding not found for findDataEncodingID(" << modeToString(mode) << ", " << inHighRes << ", " << noLoss << ")";
        return 1;
    }

    /**
     * @brief insertFakeScan
     * Insert empty spectrum object into the mzDB file
     * Because it is possible that the first cycle does not have a MS1 scan !
     *
     * @param spectrum
     * @param bbFirstScanId
     */
    template<typename mz_t, typename int_t>
    void insertFakeScan(std::shared_ptr<mzSpectrum<mz_t, int_t> >& spectrum, int bbFirstScanId) {

        const auto& spec = spectrum->spectrum;

        int msLevel = 1;
        // Prepare stmt
        const char* sql = "INSERT INTO tmp_spectrum  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(mMzdbFile.db, sql, -1, &(mMzdbFile.stmt), 0);
        sqlite3_bind_int(mMzdbFile.stmt, 1, spectrum->id); //ID could be autoincrement [PK] [NOT NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 2, spectrum->id); //initial_id [NOT NULL]
        sqlite3_bind_text(mMzdbFile.stmt, 3, "", 0, SQLITE_STATIC); //title [NOT NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 4, spectrum->cycle); //cycle [NOT NULL]
        sqlite3_bind_double(mMzdbFile.stmt, 5, 0); //time [NOT NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 6, msLevel); //msLevel [NOT NULL]
        sqlite3_bind_text(mMzdbFile.stmt, 7, "", 0, SQLITE_STATIC); //activation_type [NOT NULL]
        sqlite3_bind_double(mMzdbFile.stmt, 8, 0.0); //tic [NOT NULL]
        sqlite3_bind_double(mMzdbFile.stmt, 9, 0.0); //base peak mz [NOT NULL]
        sqlite3_bind_double(mMzdbFile.stmt, 10, 0.0); //base peak intensity [NOT NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 11); //precursor mz [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 12); //precursor charge [NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 13, 0); //datapointscount [NOT NULL]
        sqlite3_bind_text(mMzdbFile.stmt, 14, "", 0, SQLITE_STATIC); //spectrum paramtree [NOT NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 15); //scan list [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 16); //precursor list [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 17); //product list [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 18); //shared param tree id [FK] [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 19); //instrument config id [FK] [NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 20); //source file id [FK] [NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 21, 1); //run id default to 1 [FK] [NOT NULL]
        sqlite3_bind_null(mMzdbFile.stmt, 22); //data proc id [FK] [NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 23, 1); //data enc id [FK] [NOT NULL]
        sqlite3_bind_int(mMzdbFile.stmt, 24, bbFirstScanId); //bb first spec id [FK] [NOT NULL]

        //finalize statement
        sqlite3_step(mMzdbFile.stmt);
        sqlite3_finalize(mMzdbFile.stmt);
        mMzdbFile.stmt = 0;
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
                       vector<DataEncoding> dataEncodings):

        mMzdbFile(mzdb),
        mParamsCollecter(pc),
        mRawFileFormat(rawFileFormat),
        mDataEncodings(dataEncodings) {

        scanOffset = 0;
        isScanOffsetComputed = false;
        
        if (mRawFileFormat == pwiz::msdata::MS_ABI_WIFF_format)
            mWiffFile = pwiz::vendor_api::ABI::WiffFile::create(mMzdbFile.name);
    } // end ctor

    /**
     * @brief insertScan
     * Insert spectrum object into the mzDB file
     * this->insertScan<h_mz_t, h_int_t>(parentSpectrum, i, bbFirstScanIDMS1, parentSpectrum, msdata, serializer);
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
            if(idxInCycle == 1) {
                LOG(ERROR) << "null pwiz spectrum. Adding a fake spectrum for cycle " << idxInCycle;
                insertFakeScan(spectrum, bbFirstScanId);
            } else {
                LOG(ERROR) << "null pwiz spectrum. Recovering has failed";
            }
            return;
        }

        // update spectra cv params
        mParamsCollecter.updateCVMap(*spec);
        mParamsCollecter.updateUserMap(*spec);

        // fast version: crash if scans vector does not contain any scan !
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

        // prepare title and scan offset (because it is possible than initial_id = 1 when scan_id=3426)
        const string& title = spec->id;
        if(!isScanOffsetComputed) {
            int scanNumber = PwizHelper::extractScanNumber(title);
            if(scanNumber > 0) {
                scanOffset = scanNumber - spectrum->id; // default is 0 (neutral offset)
                LOG(WARNING) << "First scan number is " << scanOffset+1 << ", previous spectra could not be read";
            }
            isScanOffsetComputed = true;
        }
        
        //ID could be autoincrement
        sqlite3_bind_int(mMzdbFile.stmt, 1, spectrum->id);

        //initial_id
        sqlite3_bind_int(mMzdbFile.stmt, 2, spectrum->id + scanOffset);

        //title
        sqlite3_bind_text(mMzdbFile.stmt, 3, title.c_str(), title.length(), SQLITE_STATIC);

        //cycle
        sqlite3_bind_int(mMzdbFile.stmt, 4, spectrum->cycle);

        //time
        float rt = PwizHelper::rtOf(spec);
        if(rt == 0) LOG(ERROR) << "Can't find RT for spectrum " << title;
        sqlite3_bind_double(mMzdbFile.stmt, 5, rt);

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
        DataMode effectiveMode = spectrum->getEffectiveMode();
        sqlite3_bind_int(mMzdbFile.stmt, 23, this->findDataEncodingID(effectiveMode, isInHighRes, mMzdbFile.noLoss));

        //bb first spec id
        sqlite3_bind_int(mMzdbFile.stmt, 24, bbFirstScanId);

        //finalize statement
        sqlite3_step(mMzdbFile.stmt);
        sqlite3_finalize(mMzdbFile.stmt);
        mMzdbFile.stmt = 0;
        
        originalModesByMsLevel.insert (std::make_pair<int, DataMode>(msLevel, spectrum->getOriginalMode()));
        effectiveModesByMsLevel.insert (std::make_pair<int, DataMode>(msLevel, effectiveMode));
        auto it = nbSpectraByMsLevel.find(msLevel);
        if(it == nbSpectraByMsLevel.end()) {
            nbSpectraByMsLevel.insert (std::make_pair<int, int>(msLevel, 1));
        } else {
            it->second = it->second + 1;
        }
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
                     map<int, int>* bbFirstSpectrumID=nullptr) {

        //to handle swath cycle
        int i = 1;
        auto& parentSpectrum = cycleObject->parentSpectrum;
        if (parentSpectrum && parentSpectrum->isInserted == false && bbFirstScanIDMS1) {
            this->insertScan<h_mz_t, h_int_t>(parentSpectrum, i, bbFirstScanIDMS1, parentSpectrum, msdata, serializer);
            ++i;
        }
        auto& spectra = cycleObject->spectra;
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {

            if (it->first != nullptr) {
                int bbfirstid = (bbFirstSpectrumID != nullptr) ? (*bbFirstSpectrumID)[(it->first)->id] : cycleObject->getBeginId();
                this->insertScan<h_mz_t, h_int_t, h_mz_t, h_int_t>(it->first,
                                                                   i,
                                                                   bbfirstid, //cycleObject->getBeginId(),
                                                                   parentSpectrum,  //cycleObject->parentSpectrum,
                                                                   msdata,
                                                                   serializer);
            } else {
                int bbfirstid = (bbFirstSpectrumID != nullptr) ? (*bbFirstSpectrumID)[(it->first)->id] : cycleObject->getBeginId();
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
    
    void printGlobalInformation() {
        // at the end of the consuming (only one instance of it), write a description of what have been seen and done
        if(nbSpectraByMsLevel.size() > 0) {
            int msLevel = 1;
            std::stringstream summary;
            summary << "\n\n************ Summary of the conversion ************\n\n";
            summary << "Created file: " << mMzdbFile.outputFilename << "\n";
            summary << "MS\tInput   \tOutput  \tNb spectra\n";
            // make sure to print all levels in the right order. It used to be 'while(true)' but just in case...
            // this code will have to be updated when mass spectrometers will be able to fragment a peptide more than 100 times !! (it should be fine for a while)
            while(msLevel < 100) {
                auto originalMode = originalModesByMsLevel.find(msLevel);
                if(originalMode != originalModesByMsLevel.end()) {
                    auto effectiveMode = effectiveModesByMsLevel.find(msLevel);
                    if(effectiveMode != originalModesByMsLevel.end()) {
                        auto nbSpectra = nbSpectraByMsLevel.find(msLevel);
                        if(nbSpectra != nbSpectraByMsLevel.end()) {
                            // exploded printing in order to add the right amount of space character
                            // otherwise, it does not print well in the output
                            summary << "MS" << msLevel << "\t";
                            if(originalMode->second == PROFILE) {
                                summary << "PROFILE ";
                            } else {
                                summary << "CENTROID";
                            }
                            summary << "\t";
                            if(effectiveMode->second == PROFILE) {
                                summary << "PROFILE ";
                            } else if(effectiveMode->second == FITTED) {
                                summary << "FITTED  ";
                            } else {
                                summary << "CENTROID";
                            }
                            summary << "\t" << nbSpectra->second << "\n";
                        }
                    }
                    msLevel++;
                } else {
                    break;
                }
            }
            summary << "\n\n";
            LOG(INFO) << summary.str();
        }
    }

}; //end class


} // end namespace mzdb

#endif // MZSPECTRUMINSERTER_H
