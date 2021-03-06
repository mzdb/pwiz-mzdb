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
 * @file mzdb_writer.cpp
 * @brief Main class to performing the conversion
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#include "wchar.h"
#include "string.h"

#include "boost/thread/thread.hpp"
#include "boost/algorithm/string.hpp"

#include "mzdb_writer.hpp"
#include "version.h"

using namespace std;
using namespace pwiz::msdata;

namespace mzdb {

void mzDBWriter::buildDataEncodingRowByID() {
    DataEncoding deProfileNoLoss(1, PROFILE, NO_LOSS_PEAK);
    m_dataEncodings.push_back(deProfileNoLoss);
    DataEncoding deProfileHighResPeak(2, PROFILE, HIGH_RES_PEAK);
    m_dataEncodings.push_back(deProfileHighResPeak);
    DataEncoding deProfileLowResPeak(3, PROFILE, LOW_RES_PEAK);
    m_dataEncodings.push_back(deProfileLowResPeak);
    DataEncoding deFittedHighResPeak(4, FITTED, HIGH_RES_PEAK);
    m_dataEncodings.push_back(deFittedHighResPeak);
    DataEncoding deFittedLowResPeak(5, FITTED, LOW_RES_PEAK);
    m_dataEncodings.push_back(deFittedLowResPeak);
    DataEncoding deCentroidHighResPeak(6, CENTROID, HIGH_RES_PEAK);
    m_dataEncodings.push_back(deCentroidHighResPeak);
    DataEncoding deCentroidLowResPeak(7, CENTROID, LOW_RES_PEAK);
    m_dataEncodings.push_back(deCentroidLowResPeak);
    // required for no loss mode !
    DataEncoding deFittedNoLoss(8, FITTED, NO_LOSS_PEAK);
    m_dataEncodings.push_back(deFittedNoLoss);
    DataEncoding deCentroidNoLoss(9, CENTROID, NO_LOSS_PEAK);
    m_dataEncodings.push_back(deCentroidNoLoss);
}


void mzDBWriter::determineSpectrumListType(pwiz::msdata::SpectrumListPtr inner) {
    if (m_PreferPeakPickingVendor) {
        pwiz::msdata::detail::SpectrumList_Thermo* thermo = dynamic_cast<pwiz::msdata::detail::SpectrumList_Thermo*>(inner.get());
        if (thermo) {
            m_Mode = 1;
        }

        pwiz::msdata::detail::SpectrumList_Bruker* bruker = dynamic_cast<pwiz::msdata::detail::SpectrumList_Bruker*>(inner.get());
        if (bruker) {
            m_Mode = 2;
        }

        pwiz::msdata::detail::SpectrumList_ABI* abi = dynamic_cast<pwiz::msdata::detail::SpectrumList_ABI*>(inner.get());
        if (abi) {
            m_Mode = 3;
        }

        pwiz::msdata::detail::SpectrumList_Agilent* agilent = dynamic_cast<pwiz::msdata::detail::SpectrumList_Agilent*>(inner.get());
        if (agilent) {
            m_Mode = 4;
        }

        pwiz::msdata::detail::SpectrumList_ABI_T2D* abi_t2d = dynamic_cast<pwiz::msdata::detail::SpectrumList_ABI_T2D*>(inner.get());
        if (abi_t2d) {
            m_Mode = 5;
        }
    }
}


///Setup pragmas and tables
///@brief mzDBWriter::createTables
void mzDBWriter::createTables() {

    // foreign keys are not used because of the use of a temporary table
    // for spectrum.
    LOG(INFO) << "Settings SQLite pragmas...";
    int r = sqlite3_exec(m_mzdbFile.db,
                         //"PRAGMA mmap_size=268435456;"
                         "PRAGMA encoding = 'UTF-8';"
                         "PRAGMA page_size=4096;"
                         "PRAGMA synchronous=OFF;"
                         "PRAGMA journal_mode=OFF;"
                         "PRAGMA temp_store=3;"
                         "PRAGMA cache_size=2000;"
                         //"PRAGMA foreign_keys=ON;"
                         "PRAGMA automatic_index=OFF;"
                         "PRAGMA locking_mode=EXCLUSIVE;"
                         "PRAGMA ignore_check_constraints=ON;", 0, 0, 0);
    if (r != SQLITE_OK) {
//        LOG(ERROR) << "SQLITE_RETURN: " << r;
//        LOG(WARNING) << "Error setting database PRAGMAS: slow performance...\n";
    }
    // Setting the SQLite _database page size to the same size speeds up
    //_database on systems where the cluster size is the same. The default
    //cluster size for a Windows NTFS system seems to be 4096 bytes.It has
    //to be set before the first CREATE TABLE statement. Once the _database
    //exists, the page size is fixed and can never change.


    LOG(INFO) << "Create database tables...";

    r = sqlite3_exec(m_mzdbFile.db,
                     "CREATE TABLE data_processing (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "name TEXT NOT NULL);"

                     "CREATE TABLE scan_settings (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "param_tree,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id) );"

                     "CREATE TABLE data_encoding (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, mode TEXT(10) NOT NULL,\n"
                     "compression TEXT, byte_order TEXT(13) NOT NULL, \n"
                     "mz_precision INTEGER NOT NULL, \n"
                     "intensity_precision INTEGER NOT NULL, \n"
                     "param_tree TEXT);"

                     "CREATE TABLE software (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, \n"
                     "name TEXT NOT NULL, \n"
                     "version TEXT NOT NULL,\n"
                     "param_tree TEXT NOT NULL, \n"
                     "shared_param_tree_id INTEGER, \n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id) );"

                     "CREATE TABLE processing_method (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, \n"
                     "number INTEGER NOT NULL, \n"
                     "param_tree TEXT NOT NULL, \n"
                     "shared_param_tree_id INTEGER, \n"
                     "data_processing_id INTEGER NOT NULL, \n"
                     "software_id INTEGER NOT NULL, \n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id), \n"
                     "FOREIGN KEY (data_processing_id) REFERENCES data_processing (id), \n"
                     "FOREIGN KEY (software_id) REFERENCES software (id) );"

                     "CREATE TABLE sample (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, \n"
                     "name TEXT NOT NULL, \n"
                     "param_tree TEXT, \n"
                     "shared_param_tree_id INTEGER, \n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id));"

                     "CREATE TABLE source_file (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "name TEXT NOT NULL,\n"
                     "location TEXT NOT NULL,\n"
                     "param_tree TEXT NOT NULL,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id));"

                     "CREATE TABLE source_file_scan_settings_map (\n"
                     "scan_settings_id INTEGER NOT NULL,\n"
                     "source_file_id INTEGER NOT NULL,\n"
                     "PRIMARY KEY (scan_settings_id, source_file_id));"

                     "CREATE TABLE cv (\n"
                     "id TEXT(10) NOT NULL,\n"
                     "full_name TEXT NOT NULL,\n"
                     "version TEXT(10),\n"
                     "uri TEXT NOT NULL,\n"
                     "PRIMARY KEY (id));"

                     "CREATE TABLE param_tree_schema (\n"
                     "name TEXT NOT NULL,\n"
                     "type TEXT(10) NOT NULL,\n"
                     "schema TEXT NOT NULL,PRIMARY KEY (name));"

                     "CREATE TABLE table_param_tree_schema (\n"
                     "table_name TEXT NOT NULL,\n"
                     "schema_name TEXT NOT NULL,\n"
                     "PRIMARY KEY (table_name),\n"
                     "FOREIGN KEY (schema_name) REFERENCES param_tree_schema (name));"

                     "CREATE TABLE shared_param_tree (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "data TEXT NOT NULL,\n"
                     "schema_name TEXT NOT NULL,\n"
                     "FOREIGN KEY (schema_name) REFERENCES param_tree_schema (name));"

                     "CREATE TABLE instrument_configuration (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "name TEXT NOT NULL,\n"
                     "param_tree TEXT, \n"
                     "component_list TEXT NOT NULL,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "software_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id),\n"
                     "FOREIGN KEY (software_id) REFERENCES software (id));"

                     "CREATE TABLE mzdb (\n"
                     "version TEXT(10) NOT NULL,\n"
                     "creation_timestamp TEXT NOT NULL,\n"
                     "file_content TEXT NOT NULL,\n"
                     "contacts TEXT NOT NULL,\n"
                     "param_tree TEXT NOT NULL,\n"
                     "PRIMARY KEY (version));"

                     "CREATE TABLE run (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, \n"
                     "name TEXT NOT NULL,\n"
                     "start_timestamp TEXT,\n"
                     "param_tree TEXT,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "sample_id INTEGER NOT NULL,\n"
                     "default_instrument_config_id INTEGER NOT NULL,\n"
                     "default_source_file_id INTEGER,\n"
                     "default_scan_processing_id INTEGER NOT NULL,\n"
                     "default_chrom_processing_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id),\n"
                     "FOREIGN KEY (sample_id) REFERENCES sample (id),\n"
                     "FOREIGN KEY (default_instrument_config_id) REFERENCES instrument_configuration (id),\n"
                     "FOREIGN KEY (default_source_file_id) REFERENCES source_file (id),\n"
                     "FOREIGN KEY (default_scan_processing_id) REFERENCES data_processing (id),\n"
                     "FOREIGN KEY (default_chrom_processing_id) REFERENCES data_processing (id));"

                     "CREATE TABLE chromatogram (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "name TEXT NOT NULL,\n"
                     "activation_type TEXT(10) NOT NULL,\n"
                     "data_points BLOB NOT NULL,\n"
                     "param_tree TEXT NOT NULL, \n"
                     "precursor TEXT,\n"
                     "product TEXT,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "run_id INTEGER NOT NULL,\n"
                     "data_processing_id INTEGER,\n"
                     "data_encoding_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id),\n"
                     "FOREIGN KEY (run_id) REFERENCES run (id),\n"
                     "FOREIGN KEY (data_processing_id) REFERENCES data_processing (id),\n"
                     "FOREIGN KEY (data_encoding_id) REFERENCES data_encoding (id));"

                     "CREATE TABLE run_slice ( \n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "ms_level INTEGER NOT NULL,\n"
                     "number INTEGER NOT NULL,\n"
                     "begin_mz REAL NOT NULL,\n"
                     "end_mz REAL NOT NULL,\n"
                     "param_tree TEXT,\n"
                     "run_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (run_id) REFERENCES run (id) );"

                     "CREATE TEMPORARY TABLE tmp_spectrum (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "initial_id INTEGER NOT NULL,\n"
                     "title TEXT NOT NULL,\n"
                     "cycle INTEGER NOT NULL,\n"
                     "time REAL NOT NULL,\n"
                     "ms_level INTEGER NOT NULL,\n"
                     "activation_type TEXT(10) NOT NULL,\n"
                     "tic REAL NOT NULL,\n"
                     "base_peak_mz REAL NOT NULL, \n"
                     "base_peak_intensity REAL NOT NULL,\n"
                     "main_precursor_mz REAL,\n"
                     "main_precursor_charge INTEGER,\n"
                     "data_points_count INTEGER NOT NULL,\n"
                     "param_tree TEXT NOT NULL,\n"
                     "scan_list TEXT,\n"
                     "precursor_list TEXT,\n"
                     "product_list TEXT,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "instrument_configuration_id INTEGER,\n"
                     "source_file_id INTEGER,\n"
                     "run_id INTEGER NOT NULL,\n"
                     "data_processing_id INTEGER,\n"
                     "data_encoding_id INTEGER NOT NULL,\n"
                     "bb_first_spectrum_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id),\n"
                     "FOREIGN KEY (instrument_configuration_id) REFERENCES instrument_configuration (id),\n"
                     "FOREIGN KEY (source_file_id) REFERENCES source_file (id),\n"
                     "FOREIGN KEY (run_id) REFERENCES run (id),\n"
                     "FOREIGN KEY (data_processing_id) REFERENCES data_processing (id),\n"
                     "FOREIGN KEY (data_encoding_id) REFERENCES data_encoding (id),\n"
                     "FOREIGN KEY (bb_first_spectrum_id) REFERENCES tmp_spectrum (id));"

                     "CREATE TABLE bounding_box (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "data BLOB NOT NULL, \n"
                     "run_slice_id INTEGER NOT NULL,\n"
                     "first_spectrum_id INTEGER NOT NULL,\n"
                     "last_spectrum_id INTEGER NOT NULL,\n "
                     "FOREIGN KEY (run_slice_id) REFERENCES run_slice (id),\n"
                     "FOREIGN KEY (first_spectrum_id) REFERENCES tmp_spectrum (id),\n"
                     "FOREIGN KEY (last_spectrum_id) REFERENCES tmp_spectrum (id));"

                     "CREATE TABLE cv_term (\n"
                     "accession TEXT NOT NULL,\n"
                     "name TEXT NOT NULL,\n"
                     "unit_accession TEXT,\n"
                     "cv_id TEXT(10) NOT NULL,\n"
                     "PRIMARY KEY (accession),\n"
                     "FOREIGN KEY (unit_accession) REFERENCES cv_unit (accession),\n"
                     "FOREIGN KEY (cv_id) REFERENCES cv (id));"

                     "CREATE TABLE cv_unit (\n"
                     "accession TEXT NOT NULL,\n"
                     "name TEXT NOT NULL,\n"
                     "cv_id TEXT(10) NOT NULL,\n"
                     "PRIMARY KEY (accession),\n"
                     "FOREIGN KEY (cv_id) REFERENCES cv (id));"

                     "CREATE TABLE user_term (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "name TEXT NOT NULL,\n"
                     "type TEXT NOT NULL,\n"
                     "unit_accession TEXT,\n"
                     "FOREIGN KEY (unit_accession) REFERENCES cv_unit (accession));"

                     "CREATE TABLE target (\n"
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                     "param_tree TEXT NOT NULL,\n"
                     "shared_param_tree_id INTEGER,\n"
                     "scan_settings_id INTEGER NOT NULL,\n"
                     "FOREIGN KEY (shared_param_tree_id) REFERENCES shared_param_tree (id),\n"
                     "FOREIGN KEY (scan_settings_id) REFERENCES scan_settings (id));"

                     "CREATE VIRTUAL TABLE bounding_box_rtree USING rtree(\n"
                     "id INTEGER NOT NULL PRIMARY KEY,"
                     "min_mz REAL NOT NULL, \n"
                     "max_mz REAL NOT NULL, \n"
                     "min_time REAL NOT NULL, \n"
                     "max_time REAL NOT NULL);"

                     "CREATE VIRTUAL TABLE bounding_box_msn_rtree USING rtree(\n"
                     "id INTEGER NOT NULL PRIMARY KEY, \n"
                     "min_ms_level REAL NOT NULL, \n"
                     "max_ms_level REAL NOT NULL, \n"
                     "min_parent_mz REAL NOT NULL, \n"
                     "max_parent_mz REAL NOT NULL, \n"
                     "min_mz REAL NOT NULL, \n"
                     "max_mz REAL NOT NULL, \n"
                     "min_time REAL NOT NULL, \n"
                     "max_time REAL NOT NULL);", 0, 0, 0);

    if (r != SQLITE_OK) {
        LOG(ERROR) << "Could not create database tables properly, fatal error: Exiting...\n";
        exit(0);
    }
}

///Create indexes
/// @brief mzDBWriter::setIndexes
void mzDBWriter::createIndexes() {
    LOG(INFO) << "Creates indexes...";

    int r = sqlite3_exec(m_mzdbFile.db,
                         "CREATE UNIQUE INDEX spectrum_initial_id_idx ON spectrum (initial_id ASC,run_id ASC);"
                         "CREATE INDEX spectrum_ms_level_idx ON spectrum (ms_level ASC,run_id ASC);"
                         "CREATE UNIQUE INDEX run_name_idx ON run (name);"
                         "CREATE UNIQUE INDEX run_slice_mz_range_idx ON run_slice (begin_mz ASC,end_mz ASC,ms_level ASC,run_id ASC);"
                         "CREATE INDEX bounding_box_run_slice_idx ON bounding_box (run_slice_id ASC);"
                         "CREATE INDEX bounding_box_first_spectrum_idx ON bounding_box (first_spectrum_id ASC); "
                         "CREATE UNIQUE INDEX controlled_vocabulary_full_name_idx ON cv (full_name);"
                         "CREATE INDEX controlled_vocabulary_uri_idx ON cv (uri);"
                         "CREATE UNIQUE INDEX source_file_name_idx ON source_file (name);"
                         "CREATE UNIQUE INDEX sample_name_idx ON sample (name);"
                         "CREATE UNIQUE INDEX software_name_idx ON software (name);"
                         "CREATE UNIQUE INDEX instrument_configuration_name_idx ON instrument_configuration (name);"
                         "CREATE UNIQUE INDEX processing_method_number_idx ON processing_method (number ASC);"
                         "CREATE UNIQUE INDEX data_processing_name_idx ON data_processing (name);"
                         "CREATE UNIQUE INDEX chromatogram_name_idx ON chromatogram (name);"
                         "CREATE UNIQUE INDEX cv_term_name_idx ON cv_term (name ASC);"
                         "CREATE UNIQUE INDEX user_term_name_idx ON user_term (name ASC);"
                         "CREATE UNIQUE INDEX cv_unit_name_idx ON cv_unit (name ASC);"
                         "CREATE INDEX spectrum_bb_first_spectrum_id_idx ON spectrum (bb_first_spectrum_id ASC);"
                         , 0, 0, 0);
    m_mzdbFile.stmt = 0;
    //LOG(INFO) << "sqlite3_exec return code: '" << r << "'";
    if (r != SQLITE_OK) {
        LOG(WARNING) << "WARNING: could not set up properly table indexes\nPerformance will strongly affected !";
    }
}

///check then fill empty metadata
void mzDBWriter::checkMetaData() {

    vector<SoftwarePtr>& softwares = m_msdata->softwarePtrs;
    SoftwarePtr mzdbSoftPtr(new Software("raw2mzDB", CVParam(), SOFT_VERSION_STR));
    softwares.push_back(mzdbSoftPtr);

    if (m_msdata->dataProcessingPtrs.empty() ) {
        LOG(INFO) << "updating dataProcessing...";

        if (!  m_msdata->allDataProcessingPtrs().empty()) {
            DataProcessingPtr dataProc = m_msdata->allDataProcessingPtrs().front();

            if (m_originFileFormat == MS_Thermo_RAW_format)
                dataProc->id = THERMO_DATA_PROC;
            else if (m_originFileFormat == MS_ABI_WIFF_format && ! m_swathMode)
                dataProc->id = ABI_DATA_PROC;
            else if (m_originFileFormat == MS_ABI_WIFF_format && m_swathMode)
                dataProc->id = ABI_SWATH_DATA_PROC;
            else if (m_originFileFormat == MS_mzML_format)
                dataProc->id = XML_DATA_PROC;
            else
                dataProc->id = "mzdb_conversion";

            ProcessingMethod method;
            method.softwarePtr = mzdbSoftPtr;
            method.order = dataProc->processingMethods.size();
            method.cvParams.push_back(CVParam(MS_file_format_conversion, "Conversion to mzdb"));
            dataProc->processingMethods.push_back(method);
            m_msdata->dataProcessingPtrs.push_back(dataProc);

        }  else {
            LOG(WARNING) << "TODO: rebuild the entire dataProcessing, not done yet";
            /*printf("data processings not found...Creating a default instance.\n");
                vector<DataProcessingPtr>& dataProcessings = _msdata->dataProcessingPtrs;

                DataProcessingPtr scanProcessing(new DataProcessing("scan_processing"));
                DataProcessingPtr chromProcessing(new DataProcessing("chrom_processing"));

                ProcessingMethod origin;
                origin.order = 1;
                origin.softwarePtr = softwares[0];
                origin.cvParams.push_back(CVParam(MS_Xcalibur, "acquisition"));
                ProcessingMethod pwiz;
                pwiz.order = 2;
                pwiz.softwarePtr = softwares[1];
                pwiz.cvParams.push_back(CVParam(MS_pwiz, "reader"));
                ProcessingMethod mzdbScanProcess;
                mzdbScanProcess.order = 3;
                mzdbScanProcess.softwarePtr = softwares[2];
                mzdbScanProcess.cvParams.push_back(CVParam(MS_peak_picking, "peakPicking"));
                mzdbScanProcess.cvParams.push_back(CVParam(MS_wavelength, "writing to mzdb"));

                ProcessingMethod mzdbChromProcess;
                mzdbChromProcess.order = 3;
                mzdbChromProcess.softwarePtr = softwares[2];
                mzdbChromProcess.cvParams.push_back(CVParam(MS_wavelength, "writing to mzdb"));

                scanProcessing->processingMethods.push_back(origin);
                scanProcessing->processingMethods.push_back(pwiz);
                //scanProcessing->processingMethods.push_back(mzdbScanProcess);

                chromProcessing->processingMethods.push_back(origin);
                chromProcessing->processingMethods.push_back(pwiz);
                //chromProcessing->processingMethods.push_back(mzdbChromProcess);

                dataProcessings.push_back(scanProcessing);
                dataProcessings.push_back(chromProcessing);*/
        }
    }
    //just put in place the defined sourceFilePtr in fileContent and run.defaultSourceFilePtr

}


PWIZ_API_DECL mzDBWriter::mzDBWriter(mzdb::MzDBFile& f,
                                     MSDataPtr msdata,
                                     CVID originFileFormat,
                                     map<int, DataMode>& dataModeByMsLevel,
                                     string buildDate,
                                     map<int, double> resolutions,
                                     bool compress,
                                     bool safeMode):
    m_mzdbFile(f),
    m_dataModeByMsLevel(dataModeByMsLevel),
    m_originFileFormat(originFileFormat),
    m_msdata(msdata),
    m_paramsCollecter(f),

    m_Mode(0),
    m_PreferPeakPickingVendor(true),
    
    // store build date
    m_buildDate(buildDate),
    
    m_resolutions(resolutions),

    // booleans determining if using compression (generally not a good idea for thermo rawfiles)
    // but interesting when converting Wiff files. swathMode: Enable or disable the swath mode
    m_compress(compress),
    m_swathMode(false),
    
    m_safeMode(safeMode),

    //various counter
    m_progressionCounter(0),
    m_emptyPrecCount(0),
    max_ms_level(2) {

    //determine spectrumListPtr real type
    pwiz::msdata::SpectrumListPtr spectrumList = m_msdata->run.spectrumListPtr;
    this->determineSpectrumListType(spectrumList);


    //implementation goes here
    m_metadataExtractor = std::move(this->getMetadataExtractor());
    
    this->buildDataEncodingRowByID();
    
    this->computeResolutions();
}


void mzDBWriter::insertMetaData() {

    Run& run = m_msdata->run;

    //update cvParams
    if (m_swathMode) {
        const CVParam swathMode(MS_acquisition_parameter, "SWATH acquisition");
        run.cvParams.push_back(swathMode);
    } else {
        const CVParam swathMode(MS_acquisition_parameter, "DDA acquisition");
        run.cvParams.push_back(swathMode);
    }

    const vector<SoftwarePtr>& softwares = m_msdata->softwarePtrs;
    const vector<DataProcessingPtr>& dataProcessings = m_msdata->dataProcessingPtrs;
    const vector<ScanSettingsPtr>& scanSettings = m_msdata->scanSettingsPtrs;
    vector<SamplePtr>& samples = m_msdata->samplePtrs;
    const vector<InstrumentConfigurationPtr>& insconfs = m_msdata->instrumentConfigurationPtrs;

    //if empty can not do anything...
    const vector<ParamGroupPtr>& paramGroups = m_msdata->paramGroupPtrs;

    //special parameters of mzdb file eg size of bbs
    m_mzdbFile.userParams.push_back(
                UserParam(MS1_BB_MZ_WIDTH_STR, boost::lexical_cast<string>(m_mzdbFile.bbHeight), XML_FLOAT));
    m_mzdbFile.userParams.push_back(
                UserParam(MSN_BB_MZ_WIDTH_STR, boost::lexical_cast<string>(m_mzdbFile.bbHeightMsn), XML_FLOAT));
    m_mzdbFile.userParams.push_back(
                UserParam(MS1_BB_TIME_WIDTH_STR, boost::lexical_cast<string>(m_mzdbFile.bbWidth), XML_FLOAT));
    m_mzdbFile.userParams.push_back(
                UserParam(MSN_BB_TIME_WIDTH_STR, boost::lexical_cast<string>(m_mzdbFile.bbWidthMsn), XML_FLOAT));
    string b = m_mzdbFile.noLoss ? TRUE_STR : FALSE_STR;
    m_mzdbFile.userParams.push_back(
                UserParam(IS_LOSSLESS_STR, b, XML_BOOLEAN));
    m_mzdbFile.userParams.push_back(
                UserParam(ORIGIN_FILE_FORMAT_STR, cvTermInfo(this->m_originFileFormat).name, XML_STRING) );
    // adding a full build version, such as "raw2mzDB 0.9.8 2016-06-03 08:54:16.719122700 +0000"
    // for some unknown reasons, inserting the date alone will not work as expected and only "2016" will be inserted
    // it's probably considered as a number until the first non-number character
    string buildVersion = "raw2mzDB ";
    buildVersion += SOFT_VERSION_STR;
    if(!m_buildDate.empty()) {
        buildVersion += " " + m_buildDate;
    }
    m_mzdbFile.userParams.push_back(UserParam(BUILD_VERSION, buildVersion, XML_STRING));
    
    // MS resolutions, either calculated or provided by the user
    if(m_storeResolutions) {
        stringstream resolutions;
        for(size_t i = 0; i < m_resolutions.size(); i++) {
            if(i > 0) resolutions << ";";
            resolutions << "MS" << (i+1) << ":" << m_resolutions[i+1];
        }
        m_mzdbFile.userParams.push_back(UserParam(RESOLUTIONS_STR, resolutions.str(), XML_STRING));
    }

    m_mzdbFile.userParams.push_back( this->m_metadataExtractor->getExtraDataAsUserText());

    //compression options
    //string compressed = _compress ? "true" : "false";
    //_mzdb.userParams.push_back(UserParam("compressed", compressed, "boolean"));

    //update userMap
    m_paramsCollecter.updateUserMap(m_mzdbFile);

    const char* sql_1 = "INSERT INTO mzdb VALUES (?, ?, ?, ?, ?);";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_1, -1, &(m_mzdbFile.stmt), 0);

    //version
    sqlite3_bind_text(m_mzdbFile.stmt, 1, SCHEMA_VERSION_STR, 3, SQLITE_STATIC);

    //timestamp
    sqlite3_bind_int(m_mzdbFile.stmt, 2, time(NULL));

    //filecontent TODO: add a user param to specify  a mzdb conversion
    ostringstream os;
    pwiz::minimxml::XMLWriter writer(os);
    IO::write(writer, m_msdata->fileDescription.fileContent);
    string fileContent = os.str();
    sqlite3_bind_text(m_mzdbFile.stmt, 3, fileContent.c_str(), fileContent.length(), SQLITE_STATIC);

    //contact
    if ( ! m_msdata->fileDescription.contacts.empty()) {
        ostringstream os_2;
        pwiz::minimxml::XMLWriter writer_2(os_2);
        IO::write(writer_2, m_msdata->fileDescription.contacts[0]);
        string contact = os.str();
        sqlite3_bind_text(m_mzdbFile.stmt, 4, contact.c_str(), contact.length(), SQLITE_STATIC);
    } else {
        sqlite3_bind_text(m_mzdbFile.stmt, 4, "", 0, SQLITE_STATIC);
    }

    //param tree
    string& tree = ISerializer::serialize(m_mzdbFile);
    sqlite3_bind_text(m_mzdbFile.stmt, 5, tree.c_str(), tree.length(), SQLITE_STATIC);

    sqlite3_step(m_mzdbFile.stmt);

    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //DATA PROCESSINGS
    const char* sql_2 = "INSERT INTO data_processing VALUES (NULL, ?);";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_2, -1, &(m_mzdbFile.stmt), 0);
    for (auto s = dataProcessings.begin(); s != dataProcessings.end(); ++s){
        sqlite3_bind_text(m_mzdbFile.stmt, 1, (*s)->id.c_str(), (*s)->id.length(), SQLITE_STATIC);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);

        //increase data processing ID
        m_mzdbFile.dataProcessingID++;
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //SCAN SETTINGS TODO: shared_param_tree_id WARNING this is not a param container
    const char* sql_3 ="INSERT INTO scan_settings VALUES (NULL, ?, NULL);";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_3, -1, &(m_mzdbFile.stmt), 0);
    for (auto s = scanSettings.begin(); s != scanSettings.end(); ++s) {
        string scanSettingsString = "";
        sqlite3_bind_text(m_mzdbFile.stmt, 1, scanSettingsString.c_str(), scanSettingsString.length(), SQLITE_STATIC);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //DATAENCODING
    // are now inserted on the fly
    //for (auto it = m_dataEncodingByID.begin(); it != m_dataEncodingByID .end(); ++it) {
    //    auto dataEncodingRow = it->second;
    //    int r = sqlite3_exec(m_mzdbFile.db, dataEncodingRow.buildSQL().c_str(), 0, 0, 0);
    //    if (r != SQLITE_OK)
    //        LOG(ERROR) << "Error inserting dataEncoding row";
    //}

    //-----------------------------------------------------------------------------------------------------------
    //SOFTWARE
    const char* sql_7 = "INSERT INTO software VALUES (NULL, ?, ?, ?, NULL)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_7, -1, &(m_mzdbFile.stmt), 0);
    for (auto s = softwares.begin(); s != softwares.end(); ++s) {
        m_paramsCollecter.updateCVMap(**s);
        m_paramsCollecter.updateUserMap(**s);
        string& id = (*s)->id;
        string& version = (*s)->version;
        string& params = ISerializer::serialize(**s, m_serializer);
        sqlite3_bind_text(m_mzdbFile.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, version.c_str(), version.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
        //sqlite3_bind_int(_mzdb.stmt, 4, 45);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);

    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //PROCESSING METHOD TODO shared param _tree
    vector<ProcessingMethod> alreadyIn;
    const char* sql_8 = "INSERT INTO processing_method VALUES(NULL, ?, ?, NULL, ?, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_8, -1, &(m_mzdbFile.stmt), 0);
    for(auto s = dataProcessings.begin(); s != dataProcessings.end(); ++s) {
        auto& pm = (*s)->processingMethods;
        for(auto procmet = pm.begin(); procmet != pm.end(); ++procmet) {
            m_paramsCollecter.updateCVMap(*procmet);
            m_paramsCollecter.updateUserMap(*procmet);
            sqlite3_bind_int(m_mzdbFile.stmt, 1, (*procmet).order);
            string& params = ISerializer::serialize(*procmet, m_serializer);
            sqlite3_bind_text(m_mzdbFile.stmt, 2, params.c_str(), params.length(), SQLITE_STATIC);
            int pos_ = find(dataProcessings.begin(), dataProcessings.end(), *s) - dataProcessings.begin();
            sqlite3_bind_int(m_mzdbFile.stmt, 3, pos_ + 1);
            int pos = find(softwares.begin(), softwares.end(), (*procmet).softwarePtr) - softwares.begin();
            sqlite3_bind_int(m_mzdbFile.stmt, 4, pos + 1);
            sqlite3_step(m_mzdbFile.stmt);
            sqlite3_reset(m_mzdbFile.stmt);
            alreadyIn.push_back(*procmet);
        }
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //SAMPLE TODO shared param tree, TODO: fill it cause it is actually empty

    if (samples.empty()) {
        LOG(INFO) << "Pwiz Sample was empty. Filling it...";
        samples.push_back(this->m_metadataExtractor->getSample());
    }

    const char* sql_9 = "INSERT INTO sample VALUES(NULL, ?, ?, NULL)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_9, -1, &(m_mzdbFile.stmt), 0);
    for (auto sample = samples.begin(); sample != samples.end(); ++sample) {
        m_paramsCollecter.updateCVMap(**sample);
        m_paramsCollecter.updateUserMap(**sample);
        // FIXME use (*sample)->id instead
        string& name = (*sample)->name;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
        string& sampleString = ISerializer::serialize(**sample, m_serializer);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, sampleString.c_str(), sampleString.length(), SQLITE_STATIC);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //SOURCEFILE TODO shared param tree
    vector<SourceFilePtr> sourceFiles;
    const char* sql_10 = "INSERT INTO source_file VALUES(NULL, ?, ?, ?, NULL)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_10, -1, &(m_mzdbFile.stmt), 0);
//    if (! scanSettings.empty()) {
//        for( auto scanset = scanSettings.begin(); scanset != scanSettings.end(); ++scanset) {
//            const auto& sfs = (*scanset)->sourceFilePtrs;
//            for(auto sourceFile = sfs.begin(); sourceFile != sfs.end(); ++sourceFile) {
//                m_paramsCollecter.updateCVMap(**sourceFile);
//                m_paramsCollecter.updateUserMap(**sourceFile);
//                string& name = (*sourceFile)->name;
//                string& location = (*sourceFile)->location;
//                string params = ISerializer::serialize(**sourceFile, m_serializer);//_serializer->serialize<SourceFile>(**sourceFile);
//                sqlite3_bind_text(m_mzdbFile.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
//                sqlite3_bind_text(m_mzdbFile.stmt, 2, location.c_str(), location.length(), SQLITE_STATIC);
//                sqlite3_bind_text(m_mzdbFile.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
//                sqlite3_step(m_mzdbFile.stmt);
//                sqlite3_reset(m_mzdbFile.stmt);

//                sourceFiles.push_back(*sourceFile);
//                m_mzdbFile.sourceFileID++;
//            }
//            sqlite3_finalize(m_mzdbFile.stmt);
//            m_mzdbFile.stmt = 0;
//        }
//    } else {
    LOG(INFO) << "use default source file";
    const auto& sourceFile = m_msdata->run.defaultSourceFilePtr;
    m_paramsCollecter.updateCVMap(*sourceFile);
    m_paramsCollecter.updateUserMap(*sourceFile);
    string& name = sourceFile->name;
    string& location = sourceFile->location;
    string& params = ISerializer::serialize(*sourceFile, m_serializer);
    sqlite3_bind_text(m_mzdbFile.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
    sqlite3_bind_text(m_mzdbFile.stmt, 2, location.c_str(), location.length(), SQLITE_STATIC);
    sqlite3_bind_text(m_mzdbFile.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
    sqlite3_step(m_mzdbFile.stmt);

    sourceFiles.push_back(sourceFile);
    m_mzdbFile.sourceFileID++;

    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;
//    }

    int sourceFileCounter = 1;
    const char* sql_11 = "INSERT INTO source_file_scan_settings_map VALUES (? , ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_11, -1, &(m_mzdbFile.stmt), 0);
    for(auto scanset = scanSettings.begin(); scanset != scanSettings.end(); ++scanset) {
        const auto& sf = (*scanset)->sourceFilePtrs;
        for (size_t i=0; i < sf.size(); ++i) {
            int pos = find(scanSettings.begin(), scanSettings.end(), *scanset) - scanSettings.begin();
            sqlite3_bind_int(m_mzdbFile.stmt, 1, pos + 1);
            sqlite3_bind_int(m_mzdbFile.stmt, 2, sourceFileCounter);
            sqlite3_step(m_mzdbFile.stmt);
            sqlite3_reset(m_mzdbFile.stmt);
            sourceFileCounter++;
        }
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //TABLE_PARAM_TREE_SCHEMA TODO for paramGroup ?

    sqlite3_exec(m_mzdbFile.db,
                 "INSERT INTO param_tree_schema VALUES ('sample_schema', 'xsd','<xs:complexType name=\"SampleType\"><xs:annotation><xs:documentation>Expansible description of the sample used to generate the dataset, named in sampleName.</xs:documentation></xs:annotation><xs:complexContent mixed=\"false\"><xs:extension base=\"dx:ParamGroupType\"><xs:attribute name=\"id\"type=\"xs:ID\" use=\"required\"><xs:annotation><xs:documentation>A unique identifier across the samples with which to reference this sample description.</xs:documentation></xs:annotation></xs:attribute><xs:attribute name=\"name\" type=\"xs:string\"use=\"optional\"><xs:annotation><xs:documentation>An optional name for the sample description, mostly intended as a quick mnemonic.</xs:documentation></xs:annotation></xs:attribute></xs:extension></xs:complexContent></xs:complexType>')"
                 "INSERT INTO param_tree_schema VALUES ('software_schema', 'xsd', '')"
                 "INSERT INTO param_tree_schema VALUES ('instrument_configuration_schema', 'xsd','')"
                 "INSERT INTO param_tree_schema VALUES ('scan_settings_schema', 'xsd', '')"
                 "INSERT INTO param_tree_schema VALUES ('data_processing_schema','xsd', '')"
                 "INSERT INTO param_tree_schema VALUES ('processing_method_schema','xsd', '')"
                 "INSERT INTO param_tree_schema VALUES ('source_file_schema', 'xsd', '')"
                 "INSERT INTO table_param_tree_schema VALUES ('sample', 'sample_schema' )"
                 "INSERT INTO table_param_tree_schema VALUES ('software', 'software_schema')"
                 "INSERT INTO table_param_tree_schema VALUES ('instrument_configuration', 'instrument_configuration_schema')"
                 "INSERT INTO table_param_tree_schema VALUES ('scan_settings', 'scan_settings_schema')"
                 "INSERT INTO table_param_tree_schema VALUES ('data_processing', 'data_processing_schema')"
                 "INSERT INTO table_param_tree_schema VALUES ('processing_method', 'processing_method_schema')"
                 "INSERT INTO table_param_tree_schema VALUES ('source_file', 'source_file_schema')", 0, 0, 0);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //SHARED_PARAM_TREE

    const char* sql_26 = "INSERT INTO shared_param_tree VALUES (NULL, ?, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_26, -1, &(m_mzdbFile.stmt), 0);
    for ( auto paramGroup = paramGroups.begin(); paramGroup != paramGroups.end(); ++paramGroup) {
        ostringstream os;
        pwiz::minimxml::XMLWriter writer(os);
        IO::write(writer, **paramGroup);
        string groupParams =  os.str();
        string& id= (*paramGroup)->id;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, groupParams.c_str(), groupParams.length(), SQLITE_STATIC);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
        m_mzdbFile.sharedParamTreeID++;
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //INSTRUMENT CONFIGURATION TODO shared param tree

    const char* sql_27 = "INSERT INTO instrument_configuration VALUES(NULL, ?, ?, ?, NULL, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_27, -1, &(m_mzdbFile.stmt), 0);
    for( auto insconf = insconfs.begin(); insconf != insconfs.end(); ++insconf) {
        m_paramsCollecter.updateCVMap(**insconf);
        m_paramsCollecter.updateUserMap(**insconf);
        string& id = (*insconf)->id;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, id.c_str(), id.length(), SQLITE_TRANSIENT);
        if (! (*insconf)->cvParams.empty() || ! (*insconf)->userParams.empty()) {
            string insconfString = ISerializer::serialize(**insconf, m_serializer);
            sqlite3_bind_text(m_mzdbFile.stmt, 2, insconfString.c_str(), insconfString.length(), SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(m_mzdbFile.stmt, 2);
        }
        //component list
        ostringstream os_2;
        pwiz::minimxml::XMLWriter::Config conf;
        conf.initialStyle = pwiz::minimxml::XMLWriter::StyleFlag_Inline;
        conf.indentationStep = 0;
        pwiz::minimxml::XMLWriter writer_2(os_2, conf);
        IO::write(writer_2, (*insconf)->componentList);
        string componentListString = os_2.str();
        sqlite3_bind_text(m_mzdbFile.stmt, 3, componentListString.c_str(), componentListString.length(), SQLITE_TRANSIENT);

        int pos = find(softwares.begin(), softwares.end(), (*insconf)->softwarePtr) - softwares.begin();
        sqlite3_bind_int(m_mzdbFile.stmt, 4, pos + 1);
        //sqlite3_bind_int(m_mzdbFile.stmt, 4, 1);
        int rc = sqlite3_step(m_mzdbFile.stmt);
        if (rc != SQLITE_DONE) {
            LOG(ERROR) << "Error inserting instrument config metadata.";
            LOG(ERROR) << "SQLITE ERROR CODE: " << rc <<":" << sqlite3_errmsg(m_mzdbFile.db);
        }
        m_mzdbFile.instrumentConfigurationID++;
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //RUN
    const char* sql_28 = "INSERT INTO run VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_28, -1, &(m_mzdbFile.stmt), 0);

    const string& runName= run.id;
    sqlite3_bind_text(m_mzdbFile.stmt, 1, runName.c_str(), runName.length(), SQLITE_STATIC);

    const string& runTimeStamp = run.startTimeStamp;
    sqlite3_bind_text(m_mzdbFile.stmt, 2, runTimeStamp.c_str(), runTimeStamp.length(), SQLITE_STATIC);

    //param tree
    m_paramsCollecter.updateCVMap(run);
    m_paramsCollecter.updateUserMap(run);

    string& runString = ISerializer::serialize(run, m_serializer);
    sqlite3_bind_text(m_mzdbFile.stmt, 3, runString.c_str(), runString.length(), SQLITE_STATIC);

    // NULL is for shared param tree as it is not defined for the moment
    if (m_mzdbFile.sharedParamTreeID)
        sqlite3_bind_int(m_mzdbFile.stmt, 4, m_mzdbFile.sharedParamTreeID);
    else
        sqlite3_bind_null(m_mzdbFile.stmt, 4);

    //sample
    sqlite3_bind_int(m_mzdbFile.stmt, 5, 1);

    //instrument config
    //int pos_ = find(insconfs.begin(), insconfs.end(), run.defaultInstrumentConfigurationPtr) - insconfs.begin();
    sqlite3_bind_int(m_mzdbFile.stmt, 6, m_mzdbFile.instrumentConfigurationID);  //pos_ + 1);

    //default sourcefile
    //int pos__ = find(sourceFiles.begin(), sourceFiles.end(), run.defaultSourceFilePtr) - sourceFiles.begin();
    sqlite3_bind_int(m_mzdbFile.stmt, 7, m_mzdbFile.sourceFileID);  //pos__ + 1);

    //default spectrum processing
    sqlite3_bind_int(m_mzdbFile.stmt, 8, 1);

    //default chromatogram processing
    sqlite3_bind_int(m_mzdbFile.stmt, 9, 1);

    int rc = sqlite3_step(m_mzdbFile.stmt);
    if (rc != SQLITE_DONE) {
        LOG(ERROR) << "Error inserting run metadata.";
        LOG(ERROR) << "SQLITE ERROR CODE: " << (int)rc << ": " << sqlite3_errmsg(m_mzdbFile.db);
    }

    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //CHROMATOGRAM activation not yet implemented

    const ChromatogramListPtr& chromList = m_msdata->run.chromatogramListPtr;

    const char* sql_29 = "INSERT INTO chromatogram VALUES(NULL, ?, ?, ?, ?, ?, ?, NULL, ?, ?, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_29, -1, &(m_mzdbFile.stmt), 0);
    size_t chromListSize = chromList->size();
    for (size_t i = 0; i < chromListSize; ++i) {
        const pwiz::msdata::ChromatogramPtr& chrom = chromList->chromatogram(i, true);
        m_paramsCollecter.updateCVMap(*chrom);
        m_paramsCollecter.updateUserMap(*chrom);

        const string& id = chrom->id;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);

        bool precursorEmpty = chrom->precursor.empty();

        string activationCode=""; // = std::string(UNKNOWN_STR);
        if (! precursorEmpty)
            activationCode = getActivationCode(chrom->precursor.activation);
        sqlite3_bind_text(m_mzdbFile.stmt, 2, activationCode.c_str(), activationCode.length(), SQLITE_STATIC);


        // Populate blob data
        BinaryDataArray& x_data = *(chrom->binaryDataArrayPtrs[0]);
        BinaryDataArray& y_data = *(chrom->binaryDataArrayPtrs[1]);

        int N = x_data.data.size() * 2 * sizeof(float) + sizeof(int);
        vector<byte> data;
        data.reserve(N);
        size_t s = x_data.data.size();
        put<size_t>(s, data);

        for (size_t p_ = 0; p_ < s; ++p_) {
            float x  = (float) x_data.data[p_];
            float y = (float) y_data.data[p_];
            put<float>(x, data);
            put<float>(y, data);
        }
        /*if (_compress) {
        // COMRPESSION STUFF
            vector<byte> outBuf;
            size_t out_size = snappy::MaxCompressedLength(data.size());
            outBuf.resize(out_size);
            size_t output_length;
            snappy::RawCompress(reinterpret_cast<const char*>(&data[0]), data.size(), reinterpret_cast<char*>(&outBuf[0]), &output_length);
            outBuf.resize(output_length);
            sqlite3_bind_blob(_mzdb.stmt, 3, &outBuf[0], outBuf.size(), SQLITE_STATIC);
        } else {*/
        sqlite3_bind_blob(m_mzdbFile.stmt, 3, &data[0], data.size(), SQLITE_STATIC);
        //}
        string& params = ISerializer::serialize(*chrom, m_serializer);
        sqlite3_bind_text(m_mzdbFile.stmt, 4, params.c_str(), params.length(), SQLITE_STATIC);

        if (! precursorEmpty) {
            ostringstream os_2;
            pwiz::minimxml::XMLWriter writer_2(os_2);
            IO::write(writer_2, chrom->precursor);
            string precparams =  os_2.str();
            sqlite3_bind_text(m_mzdbFile.stmt, 5, precparams.c_str(), precparams.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(m_mzdbFile.stmt, 5);
        }

        if (! chrom->product.empty()) {
            ostringstream os_3;
            pwiz::minimxml::XMLWriter writer_3(os_3);
            IO::write(writer_3, chrom->product);
            string prodparams =  os_3.str();
            sqlite3_bind_text(m_mzdbFile.stmt, 6, prodparams.c_str(), prodparams.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(m_mzdbFile.stmt, 6);
        }

        //runID
        sqlite3_bind_int(m_mzdbFile.stmt, 7, 1);

        //data proc id
        //int pos = find(dataProcessings.begin(), dataProcessings.end(), chrom->dataProcessingPtr) - dataProcessings.begin();
        sqlite3_bind_int(m_mzdbFile.stmt, 8, 1); //pos + 1);

        //data encoding id
        sqlite3_bind_int(m_mzdbFile.stmt, 9, 1);

        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;
}

///@brief mzDBWriter::checkAndFixRunSliceNumberAnId
void mzDBWriter::checkAndFixRunSliceNumberAnId() {
    vector<int> runSliceIds;
    sqlite3_prepare_v2(m_mzdbFile.db, "SELECT id from run_slice ORDER BY ms_level, begin_mz", -1, &(m_mzdbFile.stmt), 0);
    while( sqlite3_step(m_mzdbFile.stmt) == SQLITE_ROW) {
        runSliceIds.push_back(sqlite3_column_int(m_mzdbFile.stmt, 0));
    }

    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;
    //--- ---
    if ( ! std::is_sorted(runSliceIds.begin(), runSliceIds.end()) ) {
        LOG(INFO) << "Detected problem in run slice number...fixing it";
        int runSliceNb = 1;
        sqlite3_prepare_v2(m_mzdbFile.db, "UPDATE run_slice set number=? WHERE id=?", -1, &(this->m_mzdbFile.stmt), 0);
        for (size_t i = 0; i < runSliceIds.size(); ++i) {
            sqlite3_bind_int(m_mzdbFile.stmt, 1, runSliceNb);
            sqlite3_bind_int(m_mzdbFile.stmt, 2, runSliceIds[i]);
            sqlite3_step(this->m_mzdbFile.stmt);
            sqlite3_reset(this->m_mzdbFile.stmt);
            runSliceNb++;
        }
        sqlite3_finalize(this->m_mzdbFile.stmt);
        m_mzdbFile.stmt = 0;
    }
    closeMzDbFile();
}

int mzDBWriter::getMaxMsLevel() {
    return max_ms_level;
}

/**
 * @brief isSwathAcquisition
 * Checks if the current analysis is DIA or DDA (default is DDA)
 *
 * How to tell if an analysis is DIA or DDA ?
 * In case of a DIA analysis, the CVParam MS_isolation_window_target_m_z has to be set
 * CVParams MS_isolation_window_lower_offset and MS_isolation_window_upper_offset may also be set depending on the file format
 * DDA analysis may have such information, but they should not be reproducible
 * In a DIA analysis, all MS/MS for one MS spectrum should have the same targets
 *
 */
void mzDBWriter::isSwathAcquisition() {
    // do not enter the procedure if the analysis cannot be treated
    if(m_Mode == 4) {
        LOG(INFO) << "DDA/DIA detection is not operational for the current file, fallback to DDA mode";
        return;
    }
    
    LOG(INFO) << "DDA / DIA test";
    
    vector<double> refTargets = PwizHelper::determineIsolationWindowStarts(m_msdata->run.spectrumListPtr);
    // check what has been seen
    if(refTargets.size() == 0) {
        LOG(INFO) << "DDA Mode detected";
        m_swathMode = false;
    } else {
        LOG(INFO) << "DIA Mode detected";
        m_swathMode = true;
        // print the isolation windows
        //for(int i = 0; i < refTargets.size() - 1; i++) {
        //    int windowIndex = i + 1;
        //    float windowStartMz = refTargets[i];
        //    float windowSize = refTargets[i+1] - refTargets[i];
        //    LOG(INFO) << "Swath Window #" << windowIndex <<", Start: " << windowStartMz << ",  Size: " <<  windowSize;
        //}
    }
}

vector<double> computeResolution(pwiz::msdata::detail::SpectrumList_ABI* spectrumList, int msLevel, int id, double minIntensityFactor, double maxIntensityFactor) {
    int minPeaksPerSpectrum = 10;
    vector<double> resolutions;
    
    // get spectrum in profile and centroid mode
    pwiz::util::IntegerSet levelsToCentroid;
    levelsToCentroid.insert(msLevel);
    pwiz::msdata::SpectrumPtr spectrum = spectrumList->spectrum(id, true);
    pwiz::msdata::SpectrumPtr centroidSpectrum = spectrumList->spectrum(id, true, levelsToCentroid);

    // check that there's enough peaks (more than 10) ; if not, another spectrum will be selected (outside this function)
    if(centroidSpectrum->getMZArray()->data.size() > minPeaksPerSpectrum) {
    
        // compute all FWHM for this spectrum
        const vector<double>& mzBuffer = spectrum->getMZArray()->data;
        const vector<double>& intBuffer = spectrum->getIntensityArray()->data;
        DataPointsCollection<double, double> spectrumData(mzBuffer, intBuffer, spectrum);
        mzPeakFinderUtils::PeakPickerParams p;
        vector<std::shared_ptr<Centroid<double, double>>> centroids;
        vector<pwiz::msdata::MZIntensityPair> pairs;
        centroidSpectrum->getMZIntensityPairs(pairs);
        centroids.resize(pairs.size());
        float rt = static_cast<float>(centroidSpectrum->scanList.scans[0].cvParam(pwiz::msdata::MS_scan_start_time).timeInSeconds());
        double maxIntensity = 0;
        std::transform(pairs.begin(), pairs.end(), centroids.begin(), [&rt, &maxIntensity, centroidSpectrum](pwiz::msdata::MZIntensityPair& p) -> std::shared_ptr<Centroid<double, double> > {
            double mz = (double)p.mz;
            double ints = (double)p.intensity;
            if(p.intensity > maxIntensity) maxIntensity = p.intensity;
            return std::make_shared<Centroid<double, double> >(mz, ints, rt);
        });
        spectrumData.setDetectedPeaks(centroids, p, mzPeakFinderUtils::CWT_DISABLED);
        vector<std::shared_ptr<Centroid<double, double>>> optimizedCentroids;
        spectrumData.optimize(optimizedCentroids, mzPeakFinderUtils::GAUSS_OPTIMIZATION, true);
        
        // compute resolution on "ordinary" centroids (not the lowest neither the highest peaks)
        double minIntensity = maxIntensity * minIntensityFactor;
        maxIntensity = maxIntensity * maxIntensityFactor;
        for(size_t i = 0; i < optimizedCentroids.size(); i++) {
            auto centroid = optimizedCentroids[i];
            if(centroid->intensity > minIntensity && centroid->intensity < maxIntensity) {
                double mz = centroid->mz;
                double mzLwhm = mz - centroid->lwhm;
                double mzRwhm = mz + centroid->rwhm;
                double div = mzRwhm - mzLwhm;
                if(div > 0) resolutions.push_back(mz / div);
            }
        }
    }
    return resolutions;
}

double roundResolution(double r) {
    // find divider (if resolution is 20000, divider should be 10000)
    int div = 10;
    while(r > div) div = div * 10;
    div /= 10;
    // round resolution
    // multiply resolution by 2 so 24237 would be rounded to 25000
    // divide by div and add 0.49 before rounding, at the end it will round 20100 to 20000 and 24000 to 25000
    // divide by 2 and multiply by div to cancel what have been done before
    // these operations should produce a number similar to the input value but rounded
    return (floor((r * 2) / div) / 2) * div;
}

void mzDBWriter::computeResolutions(int nbSpectraToConsider, double minIntensityFactor, double maxIntensityFactor) {
    /*
     * what we want
     * - resolution for each ms level
     * what we ignore
     * - the number of ms levels
     * - the number of spectrum per ms level
     * what we should know at the end
     * - the resolution per ms level
     * - the real number of ms level
     *
     */
    
    // resolutions will be stored in the mzDB file if they are provided by the user, or if they are computed here
    m_storeResolutions = true;
    // temporary map to know which MS level is in profile mode
    map<int, bool> profileMsLevels;
    
    //m_resolutions.clear();
    
    // do nothing if user has provided resolutions using CLI
    // it is possible that he gave only MS1 and MS2 values but no MS3 value, in this case the default value will be used
    if(m_resolutions.size() == 0) {
        // if raw file is not AB Sciex wiff file, use default values
        if(m_originFileFormat != MS_ABI_WIFF_format) { // or use m_Mode != 3 ?
            LOG(INFO) << "Using default resolution (" << DEFAULT_RESOLUTION << ")";
            m_storeResolutions = false; // no need to store these values
            // put default resolutions for all ms levels (but they wont be used for these file types)
            for(size_t msLevel = 1; msLevel <= max_ms_level; msLevel++) {
                m_resolutions[msLevel] = DEFAULT_RESOLUTION;
            }
        } else {
            LOG(INFO) << "Computing resolutions...";
            pwiz::util::IntegerSet levelsToCentroid;
            // first loop to get a map containing the number of spectrum per ms level
            map<int, vector<size_t>> spectrumIdsPerMsLevel;
            auto* spectrumList = static_cast<pwiz::msdata::detail::SpectrumList_ABI*>(m_msdata->run.spectrumListPtr.get());
            // avoid first and last 20%
            size_t nbSpectra = spectrumList->size();
            int pct20 = nbSpectra * 0.2;
            for(size_t i = pct20; i < nbSpectra - pct20; i++) {
                pwiz::msdata::SpectrumPtr spectrum = spectrumList->spectrum(i, false);
                int msLevel = spectrum->cvParam(pwiz::msdata::MS_ms_level).valueAs<int>();
                profileMsLevels[msLevel] = spectrum->hasCVParam(pwiz::msdata::MS_profile_spectrum);
                if(profileMsLevels[msLevel] && spectrumIdsPerMsLevel[msLevel].size() == 0) levelsToCentroid.insert(msLevel);
                spectrumIdsPerMsLevel[msLevel].push_back(i);
            }
            // max_ms_level does not seem to be used...
            max_ms_level = profileMsLevels.size();
            
            // for each ms level, define the spectra to treat
            map<int, vector<double>> resolutionsPerMsLevel;
            // do not consider MS1 spectra
            for(size_t msLevel = 2; msLevel <= max_ms_level; msLevel++) {
                // select ids all along the gradient
                int totalNbSpectra = spectrumIdsPerMsLevel[msLevel].size();
                int incr = (int)(totalNbSpectra / (nbSpectraToConsider+1));
                int id = incr;
                if(profileMsLevels[msLevel]) {
                    while(id < totalNbSpectra) {
                        // compute resolution for this spectrum
                        int spectrumId = id;
                        vector<double> resolution = computeResolution(spectrumList, msLevel, spectrumIdsPerMsLevel[msLevel][spectrumId], minIntensityFactor, maxIntensityFactor);
                        // if this spectrum does not have enough peaks, select the next one for the same MS level
                        while(resolution.size() == 0 && spectrumId < id + incr) {
                            resolution = computeResolution(spectrumList, msLevel, spectrumIdsPerMsLevel[msLevel][++spectrumId], minIntensityFactor, maxIntensityFactor);
                        }
                        resolutionsPerMsLevel[msLevel].insert(resolutionsPerMsLevel[msLevel].end(), resolution.begin(), resolution.end());
                        id += incr;
                    }
                }
            }
            
            // at the end, compute the average and/or median resolution per ms level
            m_resolutions[1] = DEFAULT_RESOLUTION; // except for MS1
            for(size_t msLevel = 2; msLevel <= max_ms_level; msLevel++) {
                size_t nbResolutions = resolutionsPerMsLevel[msLevel].size();
                if(profileMsLevels[msLevel]) {
                    if(nbResolutions > 0) {
                        //// average resolution
                        //double summedResolutions = 0;
                        //for(size_t i = 0; i < nbResolutions; i++) {
                        //    summedResolutions += resolutionsPerMsLevel[msLevel][i];
                        //}
                        //double resolution = roundResolution(summedResolutions / nbResolutions);
                        // median resolution
                        size_t n = nbResolutions / 2;
                        nth_element(resolutionsPerMsLevel[msLevel].begin(), resolutionsPerMsLevel[msLevel].begin() + n, resolutionsPerMsLevel[msLevel].end());
                        double resolution = roundResolution(resolutionsPerMsLevel[msLevel][n]);
                        m_resolutions[msLevel] = resolution;
                    } else {
                        LOG(WARNING) << "MS" << msLevel << " resolution could not be calculated, using default resolution (" << DEFAULT_RESOLUTION << ")";
                        m_resolutions[msLevel] = DEFAULT_RESOLUTION;
                    }
                } else {
                    m_resolutions[msLevel] = DEFAULT_RESOLUTION;
                }
            }
        }
    }
    
    for(int i = 0; i < m_resolutions.size(); i++) {
        int msLevel = i+1;
        LOG(INFO) << "MS" << msLevel << " resolution [" << (profileMsLevels[msLevel] ? "PROFILE" : "CENTROID") << "]: " << m_resolutions[msLevel];
    }
}

void mzDBWriter::computeResolutions() {
    computeResolutions(60, 0.35, 0.65);
}

}//end mzdb namesapce
