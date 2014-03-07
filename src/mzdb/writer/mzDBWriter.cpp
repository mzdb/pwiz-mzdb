#include "mzDBWriter.hpp"
#include "boost/thread/thread.hpp"
#include "wchar.h"
#include "string.h"
//#include "windows.h"
#include "boost/algorithm/string.hpp"

#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h" //to test purpose


namespace mzdb {

using namespace std;
using namespace pwiz::msdata;

/**
 * Setup pragmas and tables
 * @brief mzDBWriter::createTables
 */
void mzDBWriter::createTables() {

    LOG(INFO) << "Settings SQLite pragmas...";
    // "PRAGMA mmap_size=268435456;"

    int r = sqlite3_exec(_mzdb.db,
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

    r = sqlite3_exec(_mzdb.db,
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
                    "max_ms_level REAL NOT NULL, \n" //actually this is needed by the sqlite implementation
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

/**
 * Create indexes
 * @brief mzDBWriter::setIndexes
 */
void mzDBWriter::setIndexes() {
    LOG(INFO) << "Creates indexes...";

    int r = sqlite3_exec(_mzdb.db,
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
    _mzdb.stmt = 0;
    if (r != SQLITE_OK) {
        LOG(WARNING) << "WARNING: could not set up properly table indexes\nPerformance will be slow !";
    }
}

/**
 *  check then fill empty metadata
 */
void mzDBWriter::checkMetaData() {

    //Run& run = _msdata->run;

    vector<SoftwarePtr>& softwares = _msdata->softwarePtrs;
    //MS_MS_GF_PEP = 1002056,
    //static const CVID MS_mzdb = static_cast<CVID>((int)MS_MS_GF_PEP + 1);
    SoftwarePtr mzdbSoftPtr(new Software("mzDB", CVParam(MS_mzdb, ""), SOFT_VERSION_STR));
    softwares.push_back(mzdbSoftPtr);

    if (_msdata->dataProcessingPtrs.empty() ) {
        LOG(INFO) << "updating dataProcessing...";

        if (!  _msdata->allDataProcessingPtrs().empty()) {
            DataProcessingPtr dataProc = _msdata->allDataProcessingPtrs()[0];
            dataProc->id = "mzdb_pwiz_reader_Thermo_conversion";
            ProcessingMethod method;
            method.softwarePtr = mzdbSoftPtr;
            method.order = dataProc->processingMethods.size();
            method.cvParams.push_back(CVParam(MS_mzdb, "conversion to mzdb"));
            dataProc->processingMethods.push_back(method);
            _msdata->dataProcessingPtrs.push_back(dataProc);

        }  else {
            LOG(WARNING) << "TODO: rebuild the entire dataProcessing";
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

/**
 * find index of an instrumentConfiguration
 * @param ic
 * @return
 */
int mzDBWriter::instrumentConfigurationIndex(const InstrumentConfigurationPtr & ic) const {
    const vector<InstrumentConfigurationPtr>& insconfs = _msdata->instrumentConfigurationPtrs;
    int pos = find(insconfs.begin(), insconfs.end(), ic) - insconfs.begin();
    return pos + 1;
}

/**
 * find index of a dataProcessing object
 * @param dp
 * @return
 */
int mzDBWriter::dataProcessingIndex(const DataProcessingPtr & dp) const {
    if (!dp)
        return 1; //by default pointer to scanProcess;
    const vector<DataProcessingPtr>& dataProcessings = _msdata->allDataProcessingPtrs();
    int pos = find(dataProcessings.begin(), dataProcessings.end(), dp) - dataProcessings.begin();
    return pos + 1;
}

/**
 * find index of a source file ptr
 * @brief mzDBWriter::sourceFileIndex
 * @param sf
 * @return
 */
int mzDBWriter::sourceFileIndex(const SourceFilePtr & sf) const {
    const vector<ScanSettingsPtr>& scanSettings = _msdata->scanSettingsPtrs;
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

/**
 * @brief mzDBWriter::paramGroupIndex
 * @param pg position of the paramGroup in msdata vector
 * @return
 */
int mzDBWriter::paramGroupIndex(const ParamGroupPtr& pg) const {
    const auto& paramGroups = _msdata->paramGroupPtrs;
    int pos = find(paramGroups.begin(), paramGroups.end(), pg) - paramGroups.begin();
    return pos + 1;
}

/**
 * @brief mzDBWriter::mzDBWriter
 * @param f: mzdb file
 * @param m: data mode (fitted, centroid, profile) by mslevel
 * @param compress or not
 * @return new instance
 */
PWIZ_API_DECL mzDBWriter::mzDBWriter(mzdb::MzDBFile& f, map<int, DataMode>& m, bool compress) :
    _mzdb(f),
    _msnMode(m),
    _scanCount(1),
    _cycleCount(0),
    _bbCount(1),
    _lastMinRunSliceIdx(0),
    _lastMaxRunSliceIdx(0),
    _runSliceCount(1),
    _progressionCounter(0) {


    _dataModePos[PROFILE] = 1;
    _dataModePos[FITTED] = 2;
    _dataModePos[CENTROID] = 3;

    ReaderPtr readers(new FullReaderList);
    vector<MSDataPtr> msdList;
    try {
        ( (FullReaderList*) readers.get() )->read(_mzdb.name, msdList);
    } catch (exception& e) {
        LOG(ERROR) << e.what() << endl;
        LOG(FATAL) << "This a fatal error. Exiting..." << endl;
        exit(0);
    }

    _msdata = msdList[0];
    _originFileFormat = pwiz::msdata::identifyFileFormat( readers, _mzdb.name );
    _metadataExtractor = std::move(this->getMetadataExtractor()); //assign metadata extractor;
    this->checkMetaData();//performs to fill empty metadata, and check conformity to mzML


}

/**
 * destructor, does not handle the delete of the database
 */
/*PWIZ_API_DECL mzDBWriter::~mzDBWriter() {
    delete _metadataExtractor;
}*/

/**
 * @brief mzDBWriter::insertMetaData
 * @param noLoss: boolean, if true encoding both mz and intensity in 64 bits
 */
void mzDBWriter::insertMetaData(bool noLoss) {

    Run& run = _msdata->run;
    const vector<SoftwarePtr>& softwares = _msdata->softwarePtrs;
    const vector<DataProcessingPtr>& dataProcessings = _msdata->dataProcessingPtrs;//allDataProcessingPtrs();
    const vector<ScanSettingsPtr>& scanSettings = _msdata->scanSettingsPtrs; //indexing at i+1
    vector<SamplePtr>& samples = _msdata->samplePtrs;
    const vector<InstrumentConfigurationPtr>& insconfs = _msdata->instrumentConfigurationPtrs;
    const vector<ParamGroupPtr>& paramGroups = _msdata->paramGroupPtrs; //if empty can not do anything...

    //special parameters of mzdb file eg size of bbs
    _mzdb.userParams.push_back(UserParam("ms1_bb_mz_width", boost::lexical_cast<string>(_mzdb.bbHeight), "xsd:float"));
    _mzdb.userParams.push_back(UserParam("msn_bb_mz_width", boost::lexical_cast<string>(_mzdb.bbHeightMsn), "xsd:float"));
    _mzdb.userParams.push_back(UserParam("ms1_bb_time_width", boost::lexical_cast<string>(_mzdb.bbWidth), "xsd:float"));
    _mzdb.userParams.push_back(UserParam("msn_bb_time_width", boost::lexical_cast<string>(_mzdb.bbWidthMsn), "xsd:float"));
    string b = noLoss ? "true" : "false";
    _mzdb.userParams.push_back(UserParam("is_lossless", b, "boolean"));
    _mzdb.userParams.push_back( UserParam("origin_file_format", cvTermInfo(this->_originFileFormat).name, "xsd:string") );

    _mzdb.userParams.push_back( this->_metadataExtractor->getExtraDataAsUserText());

    //string compressed = _compress ? "true" : "false";
    //_mzdb.userParams.push_back(UserParam("compressed", compressed, "boolean"));

    //update userMap
    updateUserMap(_mzdb);

    const char* sql_1 = "INSERT INTO mzdb VALUES (?, ?, ?, ?, ?);";
    sqlite3_prepare_v2(_mzdb.db, sql_1, -1, &(_mzdb.stmt), 0);

    //version
    sqlite3_bind_text(_mzdb.stmt, 1, SCHEMA_VERSION_STR, 3, SQLITE_STATIC);

    //timestamp
    sqlite3_bind_int(_mzdb.stmt, 2, time(NULL));

    //filecontent TODO: add a user param to specify  a mzdb conversion
    ostringstream os;
    pwiz::minimxml::XMLWriter writer(os);
    IO::write(writer, _msdata->fileDescription.fileContent);
    string fileContent = os.str();
    sqlite3_bind_text(_mzdb.stmt, 3, fileContent.c_str(), fileContent.length(), SQLITE_STATIC);

    //contact
    if ( ! _msdata->fileDescription.contacts.empty()) {
        ostringstream os_2;
        pwiz::minimxml::XMLWriter writer_2(os_2);
        IO::write(writer_2, _msdata->fileDescription.contacts[0]);
        string contact = os.str();
        sqlite3_bind_text(_mzdb.stmt, 4, contact.c_str(), contact.length(), SQLITE_STATIC);
    } else {
        sqlite3_bind_text(_mzdb.stmt, 4, "", 0, SQLITE_STATIC);
    }

    //param tree
    string& tree = ISerializer::serialize(_mzdb, _serializer);
    sqlite3_bind_text(_mzdb.stmt, 5, tree.c_str(), tree.length(), SQLITE_STATIC);

    sqlite3_step(_mzdb.stmt);
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    *DATA PROCESSINGS
    ***********************************************************************/
    const char* sql_2 = "INSERT INTO data_processing VALUES (NULL, ?);";
    sqlite3_prepare_v2(_mzdb.db, sql_2, -1, &(_mzdb.stmt), 0);
    for (auto s = dataProcessings.begin(); s != dataProcessings.end(); ++s){
        sqlite3_bind_text(_mzdb.stmt, 1, (*s)->id.c_str(), (*s)->id.length(), SQLITE_STATIC);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //SCAN SETTINGS TODO: shared_param_tree_id WARNING this is not a param container
    ***********************************************************************/
    const char* sql_3 ="INSERT INTO scan_settings VALUES (NULL, ?, NULL);";
    sqlite3_prepare_v2(_mzdb.db, sql_3, -1, &(_mzdb.stmt), 0);
    for (auto s = scanSettings.begin(); s != scanSettings.end(); ++s) {
        string scanSettingsString = "";
        sqlite3_bind_text(_mzdb.stmt, 1, scanSettingsString.c_str(), scanSettingsString.length(), SQLITE_STATIC);
        //sqlite3_bind_int(_mzdb.stmt, 2, (*s)->);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //DATAENCODING TODO check this WARNING MS2 64 bit mz Encoding not yet inserted
    ***********************************************************************/
    BinaryDataArray prof, cent;
    if (noLoss) {
        prof.cvParams.push_back(CVParam(MS_64_bit_float, "64_bit_float_mz"));
        prof.cvParams.push_back(CVParam(MS_64_bit_float, "64_bit_float_intensity"));
        //if (_compress)
        //    prof.cvParams.push_back(CVParam(MS_zlib_compression, "snappy compression"));
        cent = prof;
    } else {
        prof.cvParams.push_back(CVParam(MS_64_bit_float, "64_bit_float_mz"));
        prof.cvParams.push_back(CVParam(MS_32_bit_float, "32_bit_float_intensity"));
        //if (_compress)
        //    prof.cvParams.push_back(CVParam(MS_zlib_compression, "none"));

        cent.cvParams.push_back(CVParam(MS_32_bit_float, "32_bit_float_mz"));
        cent.cvParams.push_back(CVParam(MS_32_bit_float, "32_bit_float_intensity"));
        //if (_compress)
        //    cent.cvParams.push_back(CVParam(MS_zlib_compression, "none"));
    }
    string binaryProfString = ISerializer::serialize(prof, _serializer);
    string binaryCentString = ISerializer::serialize(cent, _serializer);
    //if (binaryProfString == binaryCentString)
    //    printf("problem...\n");
    string profileMode = "INSERT INTO data_encoding VALUES (NULL, 'profile', 'none', 'little_endian', '" + binaryProfString + "')";
    sqlite3_exec(_mzdb.db, profileMode.c_str(), 0, 0, 0);
    _mzdb.stmt = 0;
    string fittedMode = "INSERT INTO data_encoding VALUES (NULL, 'fitted', 'none', 'little_endian', '" + binaryProfString + "')";
    sqlite3_exec(_mzdb.db, fittedMode.c_str(), 0, 0, 0);
    _mzdb.stmt = 0;
    string centMode = "INSERT INTO data_encoding VALUES (NULL, 'centroided', 'none', 'little_endian', '" + binaryCentString + "')";
    sqlite3_exec(_mzdb.db, centMode.c_str(), 0, 0, 0);
    _mzdb.stmt = 0;


    /***********************************************************************
    //SOFTWARE
    ***********************************************************************/
    const char* sql_7 = "INSERT INTO software VALUES (NULL, ?, ?, ?, NULL)";
    sqlite3_prepare_v2(_mzdb.db, sql_7, -1, &(_mzdb.stmt), 0);
    for (auto s = softwares.begin(); s != softwares.end(); ++s) {
        updateCVMap(**s);
        updateUserMap(**s);
        string& id = (*s)->id;
        string& version = (*s)->version;
        string& params = ISerializer::serialize(**s, _serializer);
        sqlite3_bind_text(_mzdb.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, version.c_str(), version.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
        //sqlite3_bind_int(_mzdb.stmt, 4, 45);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);

    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //PROCESSING METHOD TODO shared param _tree
    ***********************************************************************/
    vector<ProcessingMethod> alreadyIn;
    const char* sql_8 = "INSERT INTO processing_method VALUES(NULL, ?, ?, NULL, ?, ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_8, -1, &(_mzdb.stmt), 0);
    for(auto s = dataProcessings.begin(); s != dataProcessings.end(); ++s) {
        auto& pm = (*s)->processingMethods;
        for(auto procmet = pm.begin(); procmet != pm.end(); ++procmet) {
            updateCVMap(*procmet);
            updateUserMap(*procmet);
            sqlite3_bind_int(_mzdb.stmt, 1, (*procmet).order);
            string& params = ISerializer::serialize(*procmet, _serializer);
            sqlite3_bind_text(_mzdb.stmt, 2, params.c_str(), params.length(), SQLITE_STATIC);
            int pos_ = find(dataProcessings.begin(), dataProcessings.end(), *s) - dataProcessings.begin();
            sqlite3_bind_int(_mzdb.stmt, 3, pos_ + 1);
            int pos = find(softwares.begin(), softwares.end(), (*procmet).softwarePtr) - softwares.begin();
            sqlite3_bind_int(_mzdb.stmt, 4, pos + 1);
            sqlite3_step(_mzdb.stmt);
            sqlite3_reset(_mzdb.stmt);
            alreadyIn.push_back(*procmet);
        }
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //SAMPLE TODO shared param tree, TODO: fill it cause it is actually empty
    ***********************************************************************/
    if (samples.empty()) {
        samples.push_back(this->_metadataExtractor->getSample());
    }

    const char* sql_9 = "INSERT INTO sample VALUES(NULL, ?, ?, NULL)";
    sqlite3_prepare_v2(_mzdb.db, sql_9, -1, &(_mzdb.stmt), 0);
    for (auto sample = samples.begin(); sample != samples.end(); ++sample) {
        updateCVMap(**sample);
        updateUserMap(**sample);
        string& name = (*sample)->name;
        sqlite3_bind_text(_mzdb.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
        string& sampleString = ISerializer::serialize(**sample, _serializer);
        sqlite3_bind_text(_mzdb.stmt, 2, sampleString.c_str(), sampleString.length(), SQLITE_STATIC);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /************************************************************************
    //SOURCEFILE TODO shared param tree
    ***********************************************************************/
    vector<SourceFilePtr> sourceFiles;
    const char* sql_10 = "INSERT INTO source_file VALUES(NULL, ?, ?, ?, NULL)";
    sqlite3_prepare_v2(_mzdb.db, sql_10, -1, &(_mzdb.stmt), 0);
    if (! scanSettings.empty()) {
        for( auto scanset = scanSettings.begin(); scanset != scanSettings.end(); ++scanset) {
            const auto& sfs = (*scanset)->sourceFilePtrs;
            for(auto sourceFile = sfs.begin(); sourceFile != sfs.end(); ++sourceFile) {
                updateCVMap(**sourceFile);
                updateUserMap(**sourceFile);
                string& name = (*sourceFile)->name;
                string& location = (*sourceFile)->location;
                string params = ISerializer::serialize(**sourceFile, _serializer);//_serializer->serialize<SourceFile>(**sourceFile);
                sqlite3_bind_text(_mzdb.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
                sqlite3_bind_text(_mzdb.stmt, 2, location.c_str(), location.length(), SQLITE_STATIC);
                sqlite3_bind_text(_mzdb.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
                sqlite3_step(_mzdb.stmt);
                sqlite3_reset(_mzdb.stmt);
                sourceFiles.push_back(*sourceFile);
            }
            sqlite3_finalize(_mzdb.stmt);
            _mzdb.stmt = 0;
        }
    } else {
        const auto& sourceFile = _msdata->run.defaultSourceFilePtr;
        updateCVMap(*sourceFile);
        updateUserMap(*sourceFile);
        string& name = sourceFile->name;
        string& location = sourceFile->location;
        string& params = ISerializer::serialize(*sourceFile, _serializer);
        sqlite3_bind_text(_mzdb.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, location.c_str(), location.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 3, params.c_str(), params.length(), SQLITE_STATIC);
        sqlite3_step(_mzdb.stmt);
        sourceFiles.push_back(sourceFile);
        sqlite3_finalize(_mzdb.stmt);
        _mzdb.stmt = 0;
    }

    int sourceFileCounter = 1;
    const char* sql_11 = "INSERT INTO source_file_scan_settings_map VALUES (? , ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_11, -1, &(_mzdb.stmt), 0);
    for(auto scanset = scanSettings.begin(); scanset != scanSettings.end(); ++scanset) {
        const auto& sf = (*scanset)->sourceFilePtrs;
        for (size_t i=0; i < sf.size(); ++i) {
            int pos = find(scanSettings.begin(), scanSettings.end(), *scanset) - scanSettings.begin();
            sqlite3_bind_int(_mzdb.stmt, 1, pos + 1);
            sqlite3_bind_int(_mzdb.stmt, 2, sourceFileCounter);
            sqlite3_step(_mzdb.stmt);
            sqlite3_reset(_mzdb.stmt);
            sourceFileCounter++;
        }
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //TABLE_PARAM_TREE_SCHEMA TODO for paramGroup ?
    ***********************************************************************/
    sqlite3_exec(_mzdb.db,
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
    _mzdb.stmt = 0;

    /***********************************************************************
    //SHARED_PARAM_TREE
    ***********************************************************************/
    const char* sql_26 = "INSERT INTO shared_param_tree VALUES (NULL, ?, ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_26, -1, &(_mzdb.stmt), 0);
    for ( auto paramGroup = paramGroups.begin(); paramGroup != paramGroups.end(); ++paramGroup) {
        ostringstream os;
        pwiz::minimxml::XMLWriter writer(os);
        IO::write(writer, **paramGroup);
        string groupParams =  os.str();
        string& id= (*paramGroup)->id;
        sqlite3_bind_text(_mzdb.stmt, 1, groupParams.c_str(), groupParams.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //INSTRUMENT CONFIGURATION TODO shared param tree
    ***********************************************************************/
    const char* sql_27 = "INSERT INTO instrument_configuration VALUES(NULL, ?, ?, ?, 1, ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_27, -1, &(_mzdb.stmt), 0);
    for( auto insconf = insconfs.begin(); insconf != insconfs.end(); ++insconf) {
        updateCVMap(**insconf);
        updateUserMap(**insconf);
        string& id = (*insconf)->id;
        sqlite3_bind_text(_mzdb.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);
        if (! (*insconf)->cvParams.empty() || ! (*insconf)->userParams.empty()) {
            string insconfString = ISerializer::serialize(**insconf, _serializer);
            sqlite3_bind_text(_mzdb.stmt, 2, insconfString.c_str(), insconfString.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(_mzdb.stmt, 2);
        }
        //component list
        ostringstream os_2;
        pwiz::minimxml::XMLWriter::Config conf;
        conf.initialStyle = pwiz::minimxml::XMLWriter::StyleFlag_Inline;
        conf.indentationStep = 0;
        pwiz::minimxml::XMLWriter writer_2(os_2, conf);
        IO::write(writer_2, (*insconf)->componentList);
        string componentListString = os_2.str();
        sqlite3_bind_text(_mzdb.stmt, 3, componentListString.c_str(), componentListString.length(), SQLITE_STATIC);

        int pos = find(softwares.begin(), softwares.end(), (*insconf)->softwarePtr) - softwares.begin();
        sqlite3_bind_int(_mzdb.stmt, 4, pos + 1);
        //sqlite3_bind_int(_mzdb.stmt, 4, 1);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //RUN TODO shared param tree
    ***********************************************************************/
    const char* sql_28 = "INSERT INTO run VALUES (NULL, ?, ?, ?, NULL, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_28, -1, &(_mzdb.stmt), 0);
    const string& runId = run.id;
    sqlite3_bind_text(_mzdb.stmt, 1, runId.c_str(), runId.length(), SQLITE_STATIC);
    const string& runTimeStamp = run.startTimeStamp;
    sqlite3_bind_text(_mzdb.stmt, 2, runTimeStamp.c_str(), runTimeStamp.length(), SQLITE_STATIC);
    //param tree
    updateCVMap(run);
    updateUserMap(run);
    string& runString = ISerializer::serialize(run, _serializer);
    sqlite3_bind_text(_mzdb.stmt, 3, runString.c_str(), runString.length(), SQLITE_STATIC);
    //int pos = find(samples.begin(), samples.end(), run.samplePtr) - samples.begin();
    sqlite3_bind_int(_mzdb.stmt, 4, 1);//pos + 1);
    int pos_ = find(insconfs.begin(), insconfs.end(), run.defaultInstrumentConfigurationPtr) - insconfs.begin();
    sqlite3_bind_int(_mzdb.stmt, 5, pos_ + 1);
    int pos__ = find(sourceFiles.begin(), sourceFiles.end(), run.defaultSourceFilePtr) - sourceFiles.begin();
    sqlite3_bind_int(_mzdb.stmt, 6, pos__ + 1);
    sqlite3_bind_int(_mzdb.stmt, 7, 1);
    sqlite3_bind_int(_mzdb.stmt, 8, 2);
    sqlite3_step(_mzdb.stmt);
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //CHROMATOGRAM activation not yet implemented
    ***********************************************************************/
    const ChromatogramListPtr& chromList = _msdata->run.chromatogramListPtr;

    const char* sql_29 = "INSERT INTO chromatogram VALUES(NULL, ?, ?, ?, ?, ?, ?, NULL, ?, ?, ?)";
    sqlite3_prepare_v2(_mzdb.db, sql_29, -1, &(_mzdb.stmt), 0);
    for (size_t i = 0; i < chromList->size(); ++i) {
        const ChromatogramPtr& chrom = chromList->chromatogram(i, true);
        //updateCVMap(*chrom);
        //updateUserMap(*chrom);

        string& id = chrom->id;
        sqlite3_bind_text(_mzdb.stmt, 1, id.c_str(), id.length(), SQLITE_STATIC);

        sqlite3_bind_text(_mzdb.stmt, 2, "Not yet implemented", 19, SQLITE_STATIC);//check in cv params

        BinaryDataArray& x_data = *(chrom->binaryDataArrayPtrs[0]);
        BinaryDataArray& y_data = *(chrom->binaryDataArrayPtrs[1]);

        //int N = x_data.data.size() * 2 * sizeof(float) + sizeof(int);

        vector<byte> data;
        //unsigned int wpos = 0;
        size_t s = x_data.data.size();
        put<size_t>(s, data);//x_data.data.size(), data);

        for (size_t p_ = 0; p_ < x_data.data.size(); ++p_) {
            float x  = (float) x_data.data[p_];
            float y = (float) y_data.data[p_];
            put<float>(x, data);//x_data.data[p_], data);
            put<float>(y, data);//y_data.data[p_], data);
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
        sqlite3_bind_blob(_mzdb.stmt, 3, &data[0], data.size(), SQLITE_STATIC);
        //}
        string& params = ISerializer::serialize(*chrom, _serializer);
        sqlite3_bind_text(_mzdb.stmt, 4, params.c_str(), params.length(), SQLITE_STATIC);

        if (! chrom->precursor.empty()) {
            ostringstream os_2;
            pwiz::minimxml::XMLWriter writer_2(os_2);
            IO::write(writer_2, chrom->precursor);
            string precparams =  os_2.str();
            sqlite3_bind_text(_mzdb.stmt, 5, precparams.c_str(), precparams.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(_mzdb.stmt, 5);
        }

        if (! chrom->product.empty()) {
            ostringstream os_3;
            pwiz::minimxml::XMLWriter writer_3(os_3);
            IO::write(writer_3, chrom->product);
            string prodparams =  os_3.str();
            sqlite3_bind_text(_mzdb.stmt, 6, prodparams.c_str(), prodparams.length(), SQLITE_STATIC);
        } else {
            sqlite3_bind_null(_mzdb.stmt, 6);
        }

        //runID
        sqlite3_bind_int(_mzdb.stmt, 7, 1);

        //data proc id
        int pos = find(dataProcessings.begin(), dataProcessings.end(), chrom->dataProcessingPtr) - dataProcessings.begin();
        sqlite3_bind_int(_mzdb.stmt, 8, pos + 1);

        //data encoding id
        sqlite3_bind_int(_mzdb.stmt, 9, 1);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;
}

/**
 * @brief mzDBWriter::insertCollectedCVTerms
 * insert found collected cv terms in the database
 */
void mzDBWriter::insertCollectedCVTerms() {

    /***********************************************************************
    //controlled vocabulary
    ***********************************************************************/
    sqlite3_exec(_mzdb.db, "INSERT INTO cv VALUES ('psi_ms', 'PSI Mass spectrometry', '3.29.0','http://psidev.cvs.sourceforge.net/viewvc/psidev/psi/psi-ms/mzML/controlledVocabulary/psi-ms.obo?revision=1.196');", 0, 0, 0);
    _mzdb.stmt = 0;

    /***********************************************************************
    //cv_term
    ***********************************************************************/
    vector<CVID> units;
    sqlite3_prepare_v2(_mzdb.db, "INSERT INTO cv_term VALUES (?, ?, ?, ?);", -1, &(_mzdb.stmt), 0);
    for (auto cvid = _cvids.begin(); cvid != _cvids.end(); ++cvid) {
        const CVParam& cvparam = cvid->second;

        if ( find(units.begin(), units.end(), cvparam.units) == units.end())
            units.push_back(cvparam.units);

        pwiz::cv::CVTermInfo termInfo = pwiz::cv::cvTermInfo(cvparam.cvid);
        string& accession = termInfo.id;
        string& name = termInfo.name;
        string unitAccession = cvparam.unitsName();
        sqlite3_bind_text(_mzdb.stmt, 1, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 3, unitAccession.c_str(), unitAccession.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 4, "psi_ms", 6, SQLITE_STATIC);
        //step then reset
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
    //user_term
    ***********************************************************************/
    sqlite3_prepare_v2(_mzdb.db, "INSERT INTO user_term VALUES (NULL, ?, ?, ?);", -1, &(_mzdb.stmt), 0);
    for (auto pair = _userParamsByName.begin(); pair != _userParamsByName.end(); ++pair) {
        const UserParam& userparam = pair->second;

        if ( find(units.begin(), units.end(), userparam.units) == units.end())
            units.push_back(userparam.units);

        const string& name = pair->first;
        const string& type = userparam.type;
        sqlite3_bind_text(_mzdb.stmt, 1, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, type.c_str(), type.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 3, "", 0, SQLITE_STATIC);
        //step then reset
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

    /***********************************************************************
     *CV UNIT
     **********************************************************************/
    sqlite3_prepare_v2(_mzdb.db, "INSERT INTO cv_unit VALUES (?, ?, ?);", -1, &(_mzdb.stmt), 0);
    for (auto cvid = units.begin(); cvid != units.end(); cvid ++) {
        const CVTermInfo& termInfo = pwiz::cv::cvTermInfo(*cvid);
        const string& accession = termInfo.id;
        const string& name = termInfo.name;
        sqlite3_bind_text(_mzdb.stmt, 1, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(_mzdb.stmt, 3, accession.c_str(), accession.length(), SQLITE_STATIC);
        sqlite3_step(_mzdb.stmt);
        sqlite3_reset(_mzdb.stmt);
    }
    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;

}

/**
 * @brief mzDBWriter::checkAndFixRunSliceNumberAnId
 * fix taken from the david's perl fixing
 */
void mzDBWriter::checkAndFixRunSliceNumberAnId() {
    vector<int> runSliceIds;
    sqlite3_prepare_v2(_mzdb.db, "SELECT id from run_slice ORDER BY ms_level, begin_mz", -1, &(_mzdb.stmt), 0);
    while( sqlite3_step(_mzdb.stmt) == SQLITE_ROW) {
        runSliceIds.push_back(sqlite3_column_int(_mzdb.stmt, 0));
    }

    sqlite3_finalize(_mzdb.stmt);
    _mzdb.stmt = 0;
    //--- ---
    if ( ! std::is_sorted(runSliceIds.begin(), runSliceIds.end()) ) {
        LOG(INFO) << "Detected problem in run slice number...fixing it";
        int runSliceNb = 1;
        sqlite3_prepare_v2(_mzdb.db, "UPDATE run_slice set number=? WHERE id=?", -1, &(this->_mzdb.stmt), 0);
        for (size_t i = 0; i < runSliceIds.size(); ++i) {
            sqlite3_bind_int(_mzdb.stmt, 1, runSliceNb);
            sqlite3_bind_int(_mzdb.stmt, 2, runSliceIds[i]);
            sqlite3_step(this->_mzdb.stmt);
            sqlite3_reset(this->_mzdb.stmt);
            runSliceNb++;
        }
        sqlite3_finalize(this->_mzdb.stmt);
        _mzdb.stmt = 0;
    }
}

/**
 * @brief mzDBWriter::updateCVMap
 * @param pc
 */
void mzDBWriter::updateCVMap(const ParamContainer& pc) {
    for (auto cvparam = pc.cvParams.begin(); cvparam != pc.cvParams.end(); ++cvparam) {
        _cvids[(*cvparam).name()] = *cvparam;
    }
}

/**
 * @brief mzDBWriter::updateUserMap
 * @param pc
 */
void mzDBWriter::updateUserMap(const ParamContainer& pc) {
    for (auto userparam = pc.userParams.begin(); userparam != pc.userParams.end(); ++userparam) {
        _userParamsByName[(*userparam).name] = *userparam;
    }
}

/**
 * return the dataMode given a pwiz spectrum
 * @brief getDataMode
 * @param ptr
 * @return
 */
DataMode mzDBWriter::getDataMode( const pwiz::msdata::SpectrumPtr s, DataMode wantedMode)  {
    const CVParam& isCentroided = s->cvParam(MS_centroid_spectrum);
    DataMode currentMode = !isCentroided.empty() ? CENTROID : PROFILE;
    DataMode effectiveMode;
    if (wantedMode == PROFILE && currentMode == PROFILE) {
        effectiveMode = PROFILE;
    } else if ((wantedMode == CENTROID && currentMode == PROFILE) || (wantedMode == FITTED && currentMode == PROFILE)) {
        effectiveMode = wantedMode;
    } else { // current is CENTROID nothing to do
        effectiveMode = CENTROID;
    }
    return effectiveMode;
}

}//end namesapce
