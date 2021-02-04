#include "pwiz/data/msdata/MSDataFile.hpp"

#include "mzDBReader.h"
#include "mzChromatogramList.h"
#include "mzSpectrumList.h"
#include "mzRegionExtractor.h"

using namespace std;
using namespace pwiz::msdata;
using namespace pugi;

namespace mzdb {

///constructor
PWIZ_API_DECL mzDBReader::mzDBReader(MzDBFile& mzdb) : mMzdb(mzdb){

    if (sqlite3_open_v2(mMzdb.name.c_str(), &(mMzdb.db), SQLITE_OPEN_READONLY, 0) != SQLITE_OK) {
		std::cerr << "[mzDbReader:constructor] Fatal Error, the specified file does not exist or is not a sqlite mzdb file.";//LOG(ERROR) 
        exit(1);
    }

    int r = sqlite3_exec(mMzdb.db, "PRAGMA synchronous=OFF;"
                                    "PRAGMA journal_mode=OFF;"
                                    "PRAGMA temp_store=3;"
                                    "PRAGMA cache_size=8000;"
                                    //"PRAGMA mmap_size=268435456;"
                                    , 0, 0, 0);
    if ( r != SQLITE_OK) LOG(WARNING) << "Pragma settings failed...low performance ?";
    else std::cout << "Pragmas look OK...";//LOG(INFO) 
    mMzdb.stmt = 0;

    // check if no loss
    mNoLoss = mMzdb.isNoLoss();
    if (mNoLoss)
        LOG(INFO) << "No Loss detected mode\n";

}

/// main function which creates custom spectrumList and chormatogramList
PWIZ_API_DECL void  mzDBReader::readMzDB(MSData& msdata, bool cache) {
    SpectrumListPtr slptr(new mzSpectrumList(mMzdb, &msdata, cache, mNoLoss));
	std::cout << " Custom spectrumList created"; //LOG(INFO)
    msdata.run.spectrumListPtr = slptr;

    //TODO create real chromatogram reader
    ChromatogramListPtr clptr( new mzEmptyChromatogram ); //new mzChromatogramList(_mzdb, &_msdata));
    msdata.run.chromatogramListPtr = clptr;
	std::cout << "Custom chromatogram created"; //LOG(INFO)

    fillInMetaData(msdata);
	std::cout << " Filling metadata done"; //LOG(INFO)

    //--- needed do not really why but if lack may occur really bad things
    References::resolve(msdata);

}

void mzDBReader::fillInMetaData(MSData& msdata) {
    //first getting softwares;
    const char * sql = "SELECT name, version, param_tree FROM software";
    sqlite3_prepare_v2(mMzdb.db, sql, -1, &(mMzdb.stmt), 0);

    while (sqlite3_step(mMzdb.stmt) == SQLITE_ROW) {
        string id = string((const char*) sqlite3_column_text(mMzdb.stmt, 0));
        string version = string((const char*) sqlite3_column_text(mMzdb.stmt, 1));
        const char_t *result = (const char_t *) sqlite3_column_text(mMzdb.stmt, 2);

        SoftwarePtr s(new Software(id));
        s->version = version;
        IDeserializer::setParams(*s, result);
        msdata.softwarePtrs.push_back(s);
    }
    sqlite3_finalize(mMzdb.stmt);
    mMzdb.stmt = 0;

    //TODO: fill samples et scan settings
    //get samples
    //SamplePtr s(new Sample("s1", "sample1"));
    //_msdata.samplePtrs.push_back(s);
    //printf("after sample\n");

    //scan settings
    //ScanSettingsPtr scanSett(new ScanSettings("scanSettings1"));
    //_msdata.scanSettingsPtrs.push_back(scanSett);
    //printf("after scan_settings\n");


    //get SourceFile
    const char* sql_4 = "SELECT id, name, location, param_tree FROM source_file";
    sqlite3_prepare_v2(mMzdb.db, sql_4, -1, &(mMzdb.stmt), 0);
    while (sqlite3_step(mMzdb.stmt) == SQLITE_ROW) {
        string name = string((const char*) sqlite3_column_text(mMzdb.stmt, 0));
        string location = string((const char*) sqlite3_column_text(mMzdb.stmt, 1));
        const char_t* r = (const char_t*) sqlite3_column_text(mMzdb.stmt, 2);
        string id = "RAW1";
        SourceFilePtr s(new SourceFile(id, name, location));
        IDeserializer::setParams(*s, r);
        msdata.fileDescription.sourceFilePtrs.push_back(s);
        msdata.run.defaultSourceFilePtr = s;
        if (! msdata.scanSettingsPtrs.empty())
            msdata.scanSettingsPtrs[0]->sourceFilePtrs.push_back(s);
    }
    sqlite3_finalize(mMzdb.stmt);
    mMzdb.stmt = 0;

    msdata.id = msdata.run.defaultSourceFilePtr->name;
    //printf("after sourceFile\n");

    //dataProcessing
    vector<DataProcessingPtr>& vDataProc = msdata.dataProcessingPtrs;
    const char* sql_5 = "SELECT name FROM data_processing";
    sqlite3_prepare_v2(mMzdb.db, sql_5, -1, &(mMzdb.stmt), 0);
    while (sqlite3_step(mMzdb.stmt) == SQLITE_ROW) {
        const char* r = (const char*) sqlite3_column_text(mMzdb.stmt, 0);
        DataProcessingPtr d(new DataProcessing(string(r)));
        vDataProc.push_back(d);
        //By default dataProcessing does not exist in current msdata
        //vector of data processing (see pwiz implementation) but is owned by spectrumListPtr
        //_msdata.dataProcessingPtrs.push_back(d);
        //_msdata.run.spectrumListPtr->

    }
    sqlite3_finalize(mMzdb.stmt);
    mMzdb.stmt = 0;

    //printf("after data processing\n");
    if (vDataProc.size() == 1) {
        ((mzSpectrumList*)msdata.run.spectrumListPtr.get())->setDataProcessingPtr(vDataProc.front());
       // ((mzChromatogramList*)_msdata.run.chromatogramListPtr.get())->setDataProcessingPtr(vDataProc.front());
    } else if ( vDataProc.size() > 1){
        printf("Dont know which dataProcessing to attribute to SpectrumListPtr\n, set the last one by default\n");
        ((mzSpectrumList*)msdata.run.spectrumListPtr.get())->setDataProcessingPtr(vDataProc.back());
       // ((mzChromatogramList*)_msdata.run.chromatogramListPtr.get())->setDataProcessingPtr(vDataProc.back());
    } else {
		std::cout << "Missing a dataProcessing; this is never supposed to happen"; //LOG(WARNING)
    }

    //processing method
    const char* sql_6 = "SELECT id, param_tree, data_processing_id, software_id FROM processing_method";
    sqlite3_prepare_v2(mMzdb.db, sql_6, -1, &(mMzdb.stmt), 0);
    while (sqlite3_step(mMzdb.stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(mMzdb.stmt, 0);
        const char_t* r = (const char_t*) sqlite3_column_text(mMzdb.stmt, 1);
        int dataProcessingID = sqlite3_column_int(mMzdb.stmt, 2);
        int softwareID = sqlite3_column_int(mMzdb.stmt, 3);

        ProcessingMethod p;
        p.order = id;
        p.softwarePtr = msdata.softwarePtrs[softwareID - 1];
        msdata.allDataProcessingPtrs()[dataProcessingID - 1]->processingMethods.push_back(p);
        IDeserializer::setParams(p, r);
    }
    sqlite3_finalize(mMzdb.stmt);
    mMzdb.stmt = 0;

    //instrument Config
    const char* sql_7 = "SELECT name, param_tree, software_id, component_list FROM instrument_configuration";
    sqlite3_prepare_v2(mMzdb.db, sql_7, -1, &(mMzdb.stmt), 0);
    while (sqlite3_step(mMzdb.stmt) == SQLITE_ROW) {
        const char* name = (const char*) sqlite3_column_text(mMzdb.stmt, 0);

        int softwareID = sqlite3_column_int(mMzdb.stmt, 2);
        InstrumentConfigurationPtr ic(new InstrumentConfiguration(string(name)));
        ic->softwarePtr = msdata.softwarePtrs[softwareID - 1];
        const char_t* tree = (const char_t*) sqlite3_column_text(mMzdb.stmt, 1);
        if (tree) {
            IDeserializer::setParams(*ic, tree);
        }
        const char_t* componentListString = (const char_t*) sqlite3_column_text(mMzdb.stmt, 3);
        if (componentListString) {
            xml_document doc; doc.load(componentListString);
            auto componentList = doc.child("componentList");
            for ( auto it = componentList.begin(); it != componentList.end(); ++it) {

                ComponentType t;
                string name = string(it->name());
                if (name == "source") {
                    t = ComponentType_Source;
                } else if (name == "analyser") {
                    t = ComponentType_Analyzer;
                } else
                    t = ComponentType_Detector;
                int order = boost::lexical_cast<int>(string(it->attribute("order").value()));
                Component c(t, order);
                IDeserializer::setCvParams(c, *it);
                IDeserializer::setUserParams(c, *it);
                ic->componentList.push_back(c);
            }
        }
        msdata.instrumentConfigurationPtrs.push_back(ic);
    }
    sqlite3_finalize(mMzdb.stmt);
    mMzdb.stmt = 0;

    if ( ! msdata.instrumentConfigurationPtrs.empty()) {
        msdata.run.defaultInstrumentConfigurationPtr = msdata.instrumentConfigurationPtrs[0];
    }
    msdata.run.id = msdata.id;
}


void mzDBReader::convertTo(CVID cvid, bool noLoss) {

    pwiz::msdata::MSData msd;
    this->readMzDB(msd, noLoss);

    switch (cvid) {
        case (MS_mzML_format): {
            try {
                MSDataFile::WriteConfig conf;
                conf.format = MSDataFile::Format_mzML;
                conf.indexed = true;
                pwiz::util::IterationListenerRegistry iterationListenerRegistry;
                const size_t iterationPeriod = 100;
                iterationListenerRegistry.addListener(pwiz::util::IterationListenerPtr(new UserFeedbackIterationListener), iterationPeriod);
                pwiz::util::IterationListenerRegistry* pILR = &iterationListenerRegistry ;

                if (!noLoss) {
					std::cout << "conversion to intensity 32 bits\n"; // LOG(INFO) 
                    conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
                    conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
                    conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_32;
                }
                else {
					std::cout << "conversion to intensity 64 bits\n";// LOG(INFO) 
                    conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
                    conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
                    conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_64;
                }
                MSDataFile::write(msd, this->filename() + ".mzML", conf, pILR);
            } catch (exception &e) {
                cout << e.what() << endl;
            }
        }
        default :{
			std::cout << "conversion not supported"; //LOG(WARNING) 
        }
    }
}

/// Simple example showing how to enumerate spectra
/// note: to get a random spectra, must implements a getSpectrum method
void mzDBReader::enumerateSpectra()  {
    //---readmzdb content using an empty msdata object
    pwiz::msdata::MSData msd;
    this->readMzDB(msd);

    SpectrumList& sl = *msd.run.spectrumListPtr;
    size_t totalArrayLength = 0;

    clock_t beginTime = clock();
    cout << sl.size() << endl;
    for (size_t i = 0; i < sl.size(); ++i) {
        SpectrumPtr s = sl.spectrum(i, false);
        totalArrayLength += s->defaultArrayLength;
        if (i + 1 == sl.size() || ((i + 1) % 100) == 0)
            cout << "Enumerating spectra: " << (i + 1) << '/' << sl.size() << " (" << totalArrayLength << " data points)\r" << flush;
    }
    cout << endl;
    clock_t endTime = clock();
    cout << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << endl <<endl;
}


void mzDBReader::extractRegion(double mzmin, double mzmax, double rtmin, double rtmax, int msLevel, vector<mzScan*> results) {
    mzRegionExtractor regionExtractor(this->mMzdb);

    //providing a vector for storing results
    try {
        regionExtractor.rTreeExtraction(mzmin, mzmax, rtmin, rtmax, msLevel, results);
    } catch(exception& e) {
         std::cerr << "Error: " << e.what(); //LOG(ERROR)
    } catch(...) {
		std::cerr << "unknown error, failed."; //LOG(ERROR)
    }
}


void mzDBReader::extractRunSlice(double mzmin, double mzmax, int msLevel, vector<mzScan*> results) {
    mzRegionExtractor regionExtractor(this->mMzdb);

    //providing a vector for storing results
    try {
        regionExtractor.runSliceExtraction(mzmin, mzmax, msLevel, results);
    } catch(exception& e) {
		std::cerr  << "Error: " << e.what();//LOG(ERROR)
    } catch(...) {
		std::cerr << "unknown error, failed.";//LOG(ERROR)
    }
}


}//end namespace





