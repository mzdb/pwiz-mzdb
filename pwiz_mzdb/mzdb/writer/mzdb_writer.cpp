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

//author marc.dubois@ipbs.fr

#include "wchar.h"
#include "string.h"

#include "mzdb_writer.hpp"
#include "version.h"

#include "boost/thread/thread.hpp"
#include "boost/algorithm/string.hpp"
#ifdef _WIN32
#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h" //to test purpose
#endif

namespace mzdb {

using namespace std;
using namespace pwiz::msdata;

///Setup pragmas and tables
///@brief mzDBWriter::createTables
void mzDBWriter::createTables() {

    LOG(INFO) << "Settings SQLite pragmas...";
    // "PRAGMA mmap_size=268435456;"

    int r = sqlite3_exec(m_mzdbFile.db,
                         // "PRAGMA mmap_size=268435456;"
                         "PRAGMA encoding = 'UTF-8';"
                         "PRAGMA page_size=4096;"
                         "PRAGMA synchronous=OFF;"
                         "PRAGMA journal_mode=OFF;"
                         "PRAGMA temp_store=3;"
                         "PRAGMA cache_size=2000;"
                         "PRAGMA foreign_keys=OFF;"
                         "PRAGMA automatic_index=OFF;"
                         "PRAGMA locking_mode=EXCLUSIVE;"
                         "PRAGMA ignore_check_constraints=ON;", 0, 0, 0);
    if (r != SQLITE_OK) {
        LOG(ERROR) << "SQLITE_RETURN: " << r;
        LOG(INFO) << "Error setting database PRAGMAS: slow performance...\n";
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
                    "param_tree TEXT NOT NULL);"

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
                    "PRIMARY KEY (id));\n"

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

                    "CREATE TABLE spectrum (\n"
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
                    "FOREIGN KEY (bb_first_spectrum_id) REFERENCES spectrum (id));"

                    "CREATE TABLE bounding_box (\n"
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                    "data BLOB NOT NULL, \n"
                    "run_slice_id INTEGER NOT NULL,\n"
                    "first_spectrum_id INTEGER NOT NULL,\n"
                    "last_spectrum_id INTEGER NOT NULL,\n "
                    "FOREIGN KEY (run_slice_id) REFERENCES run_slice (id),\n"
                    "FOREIGN KEY (first_spectrum_id) REFERENCES spectrum (id),\n"
                    "FOREIGN KEY (last_spectrum_id) REFERENCES spectrum (id));"

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
    if (r != SQLITE_OK) {
        LOG(WARNING) << "WARNING: could not set up properly table indexes\nPerformance will strongly affected !";
    }
}

///check then fill empty metadata
void mzDBWriter::checkMetaData() {

    vector<SoftwarePtr>& softwares = m_msdata->softwarePtrs;
    SoftwarePtr mzdbSoftPtr(new Software("mzDB", CVParam(MS_Progenesis_LC_MS, ""), SOFT_VERSION_STR));
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
            method.cvParams.push_back(CVParam(MS_Progenesis_LC_MS, "conversion to mzdb"));
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

///@brief mzDBWriter::mzDBWriter
///@param f: mzdb file
///@param m: data mode (fitted, centroid, profile) by mslevel
///@param compress or not
///@return new instance
PWIZ_API_DECL mzDBWriter::mzDBWriter(mzdb::MzDBFile& f,
                                                              map<int, DataMode>& dataModeByMsLevel,
                                                              CVID originFileFormat,
                                                              MSDataPtr msdata,
                                                              bool compress) :
    m_mzdbFile(f),
    m_dataModeByMsLevel(dataModeByMsLevel),
    m_originFileFormat(originFileFormat),
    m_msdata(msdata),
    m_paramsCollecter(f),

    // booleans determining if using compression (generally not a good idea for thermo rawfiles)
    // but interesting when converting Wiff files. swathMode: Enable or disable the swath mode
    m_compress(compress),
    m_swathMode(false),

    //various counter
    m_progressionCounter(0),
    m_emptyPrecCount(0) {

        //implem goes here
        m_metadataExtractor = std::move(this->getMetadataExtractor());
    }


///@brief mzDBWriter::insertMetaData
///@param noLoss: boolean, if true encoding both mz and intensity in 64 bits
void mzDBWriter::insertMetaData(bool noLoss) {

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
    string b = noLoss ? TRUE_STR : FALSE_STR;
    m_mzdbFile.userParams.push_back(
                UserParam(IS_LOSSLESS_STR, b, XML_BOOLEAN));
    m_mzdbFile.userParams.push_back(
                UserParam(ORIGIN_FILE_FORMAT_STR, cvTermInfo(this->m_originFileFormat).name, XML_STRING) );

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
    string& tree = ISerializer::serialize(m_mzdbFile, m_serializer);
    sqlite3_bind_text(m_mzdbFile.stmt, 5, tree.c_str(), tree.length(), SQLITE_STATIC);

    int rc_ = sqlite3_step(m_mzdbFile.stmt);
//    if (rc_ != SQLITE_DONE)
//        LOG(ERROR) << "mzdb metdata failed: SQLITE ERROR CODE: " << rc_;
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
    //DATAENCODING TODO check this WARNING MS2 64 bit mz Encoding not yet inserted

    BinaryDataArray prof, cent;
    if (noLoss) {
        prof.cvParams.push_back(CVParam(MS_64_bit_float, _64_BIT_MZ));
        prof.cvParams.push_back(CVParam(MS_64_bit_float, _64_BIT_INTENSITY));
        //if (_compress)
        //    prof.cvParams.push_back(CVParam(MS_zlib_compression, "snappy compression"));
        cent = prof;
    } else {
        prof.cvParams.push_back(CVParam(MS_64_bit_float,  _64_BIT_MZ));
        prof.cvParams.push_back(CVParam(MS_32_bit_float, _32_BIT_INTENSITY));
        //if (_compress)
        //    prof.cvParams.push_back(CVParam(MS_zlib_compression, "none"));

        cent.cvParams.push_back(CVParam(MS_32_bit_float, _32_BIT_MZ));
        cent.cvParams.push_back(CVParam(MS_32_bit_float, _32_BIT_INTENSITY));
        //if (_compress)
        //    cent.cvParams.push_back(CVParam(MS_zlib_compression, "none"));
    }
    string binaryProfString = ISerializer::serialize(prof, m_serializer);
    string binaryCentString = ISerializer::serialize(cent, m_serializer);
    string profileMode = "INSERT INTO data_encoding VALUES (NULL, 'profile', 'none', 'little_endian', '" + binaryProfString + "')";
    sqlite3_exec(m_mzdbFile.db, profileMode.c_str(), 0, 0, 0);
    m_mzdbFile.stmt = 0;
    string fittedMode = "INSERT INTO data_encoding VALUES (NULL, 'fitted', 'none', 'little_endian', '" + binaryProfString + "')";
    sqlite3_exec(m_mzdbFile.db, fittedMode.c_str(), 0, 0, 0);
    m_mzdbFile.stmt = 0;
    string centMode = "INSERT INTO data_encoding VALUES (NULL, 'centroided', 'none', 'little_endian', '" + binaryCentString + "')";
    sqlite3_exec(m_mzdbFile.db, centMode.c_str(), 0, 0, 0);
    m_mzdbFile.stmt = 0;


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
    if (! scanSettings.empty()) {
        for( auto scanset = scanSettings.begin(); scanset != scanSettings.end(); ++scanset) {
            const auto& sfs = (*scanset)->sourceFilePtrs;
            for(auto sourceFile = sfs.begin(); sourceFile != sfs.end(); ++sourceFile) {
                m_paramsCollecter.updateCVMap(**sourceFile);
                m_paramsCollecter.updateUserMap(**sourceFile);
                string& name = (*sourceFile)->name;
                string& location = (*sourceFile)->location;
                string params = ISerializer::serialize(**sourceFile, m_serializer);//_serializer->serialize<SourceFile>(**sourceFile);
                sqlite3_bind_text(m_mzdbFile.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
                sqlite3_bind_text(m_mzdbFile.stmt, 2, location.c_str(), location.length(), SQLITE_STATIC);
                sqlite3_bind_text(m_mzdbFile.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
                sqlite3_step(m_mzdbFile.stmt);
                sqlite3_reset(m_mzdbFile.stmt);
                sourceFiles.push_back(*sourceFile);
            }
            sqlite3_finalize(m_mzdbFile.stmt);
            m_mzdbFile.stmt = 0;
        }
    } else {
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
        sqlite3_finalize(m_mzdbFile.stmt);
        m_mzdbFile.stmt = 0;
    }

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
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //INSTRUMENT CONFIGURATION TODO shared param tree

    const char* sql_27 = "INSERT INTO instrument_configuration VALUES(NULL, ?, ?, ?, 1, ?)";
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
        //sqlite3_bind_int(_mzdb.stmt, 4, 1);
        sqlite3_step(m_mzdbFile.stmt);
        sqlite3_reset(m_mzdbFile.stmt);
    }
    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //RUN TODO shared param tree

    const char* sql_28 = "INSERT INTO run VALUES (NULL, ?, ?, ?, NULL, ?, ?, ?, ?, ?)";
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

    //sample
    sqlite3_bind_int(m_mzdbFile.stmt, 4, 1);//pos + 1);

    //instrument config
    int pos_ = find(insconfs.begin(), insconfs.end(), run.defaultInstrumentConfigurationPtr) - insconfs.begin();
    sqlite3_bind_int(m_mzdbFile.stmt, 5, pos_ + 1);

    //default sourcefile
    int pos__ = find(sourceFiles.begin(), sourceFiles.end(), run.defaultSourceFilePtr) - sourceFiles.begin();
    sqlite3_bind_int(m_mzdbFile.stmt, 6, pos__ + 1);

    //default spectrum processing
    sqlite3_bind_int(m_mzdbFile.stmt, 7, 1);

    //default chromatogram processing
    sqlite3_bind_int(m_mzdbFile.stmt, 8, 2);

    int rc = sqlite3_step(m_mzdbFile.stmt);
//    if (rc != SQLITE_DONE)
//        LOG(ERROR) << "Error inserting run metadata.";
//        LOG(ERROR) << "SQLITE ERROR CODE: " << (int)rc;

    sqlite3_finalize(m_mzdbFile.stmt);
    m_mzdbFile.stmt = 0;

    //-----------------------------------------------------------------------------------------------------------
    //CHROMATOGRAM activation not yet implemented

    const ChromatogramListPtr& chromList = m_msdata->run.chromatogramListPtr;

    const char* sql_29 = "INSERT INTO chromatogram VALUES(NULL, ?, ?, ?, ?, ?, ?, NULL, ?, ?, ?)";
    sqlite3_prepare_v2(m_mzdbFile.db, sql_29, -1, &(m_mzdbFile.stmt), 0);
    for (size_t i = 0; i < chromList->size(); ++i) {
        const ChromatogramPtr& chrom = chromList->chromatogram(i, true);
        m_paramsCollecter.updateCVMap(*chrom);
        m_paramsCollecter.updateUserMap(*chrom);

        string& id = chrom->id;
        sqlite3_bind_text(m_mzdbFile.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);

        bool precursorEmpty =chrom->precursor.empty();

        auto activationCode = std::string(UNKNOWN_STR);
        if (! precursorEmpty)
            activationCode = getActivationCode( chrom->precursor.activation );
        sqlite3_bind_text(m_mzdbFile.stmt, 2, activationCode.c_str(), 19, SQLITE_STATIC);


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
        int pos = find(dataProcessings.begin(), dataProcessings.end(), chrom->dataProcessingPtr) - dataProcessings.begin();
        sqlite3_bind_int(m_mzdbFile.stmt, 8, pos + 1);

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
}


bool mzDBWriter::isSwathAcquisition() {
    LOG(INFO) << "DDA / Swath test...";

    pwiz::msdata::SpectrumListPtr spectrumList = m_msdata->run.spectrumListPtr;
    int spectrumListSize = spectrumList->size();

    double lastMz = 0;
    int ms1CountWithAtLeastNMs2 = 0;
    int ms2Count = 0;
    int cycle = 0;
    bool ms1Found = false;


    for (int i = 0; i < spectrumListSize;  ++i) {
        // do not get binary data (second parameter) in order to be as fast as possible
        pwiz::msdata::SpectrumPtr ptr = spectrumList->spectrum(i, false);
        const int& msLevel = ptr->cvParam(MS_ms_level).valueAs<int>();

        if (msLevel == 1) {
            ++cycle;
            lastMz = 0;
            ms2Count = 0;
//            if (ms1CountWithAtLeastNMs2 == 20) {
//                LOG(INFO) << "Swath Mode detected ! Congrats";
//                m_swathMode = true;
//                return m_swathMode;
//            }

            ms1Found = true;
        }  else if (msLevel == 2) {
            const pwiz::msdata::SelectedIon& si = ptr->precursors.front().selectedIons.front();
            const double& precMz = si.cvParam(pwiz::msdata::MS_selected_ion_m_z).valueAs<double>();
            ++ms2Count;

            if (ms2Count == 10)
                ++ms1CountWithAtLeastNMs2;

            if (precMz <= lastMz) { // && ms2Count >= 3) {
                LOG(INFO) << "DDA Mode detected.";
                m_swathMode = false;
                return m_swathMode;
            }
            lastMz = precMz;
        }
    } // end for
    LOG(WARNING) << "Not able to detected DDA/SWATH mode: fallbacks to DDA";

    // directly return m_swathMode attribute as it is set to 'false' as default
    m_swathMode = true;
    return m_swathMode;
} // end function

}//end mzdb namesapce
