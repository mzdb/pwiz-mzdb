
#include "mzDBReader.h"
#include "msdata/mzChromatogramList.h"
#include "msdata/mzSpectrumList.h"


namespace mzdb {

	using namespace std;
	using namespace pwiz::msdata;
	using namespace pugi;

PWIZ_API_DECL mzDBReader::mzDBReader(MzDBFile* mzdb) : _mzdb(mzdb){
    if (! _mzdb) {
        printf("[mzDBreader constructor]: No good mzDbFile pointer !");
        return;
    }
    if (sqlite3_open_v2(_mzdb->name.c_str(), &(_mzdb->db), SQLITE_OPEN_READWRITE, 0) != SQLITE_OK) {
        printf("[mzDbReader:constructor] Fatal Error, the specified file does not exist or is not a sqlite mzdb file.\n");
        exit(1);
    }

    int r = sqlite3_exec(_mzdb->db, "PRAGMA synchronous=OFF;"
                                    "PRAGMA journal_mode=OFF;"
                                    "PRAGMA temp_store=3;"
                                    "PRAGMA cache_size=8000000;"
                                    "PRAGMA mmap_size=268435456;"
                                    , 0, 0, 0);
    if ( r != SQLITE_OK) printf("Pragma settings failed...low performance ?\n");
    else printf("Pragmas look OK...\n");
    _mzdb->stmt = 0;

    _noLoss = _mzdb->isNoLoss();
    if (_noLoss)
        printf("No Loss detected mode\n");

}




    PWIZ_API_DECL void  mzDBReader::readMzRTreeDB(MSData& _msdata, bool cache) {
        printf("Try to create custom spectrumList...");
        SpectrumListPtr slptr(new mzSpectrumList(_mzdb, &_msdata, cache, _noLoss));
        printf("done\n");
        _msdata.run.spectrumListPtr = slptr;

        /*printf("Try to create custom chromatogramList...");
        ChromatogramListPtr clptr( new mzEmptyChromatogram );//new mzChromatogramList(_mzdb, &_msdata));
        _msdata.run.chromatogramListPtr = clptr;
        printf("done\n");*/

        printf("filling metadata...");
        fillInMetaData(_msdata);
        printf("done\n");

        References::resolve(_msdata);

    }



    void mzDBReader::fillInMetaData(MSData& _msdata) {
		//first getting softwares;
		const char * sql = "SELECT name, version, param_tree FROM software";
        sqlite3_prepare_v2(_mzdb->db, sql, -1, &(_mzdb->stmt), 0);

        while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
            string id = string((const char*) sqlite3_column_text(_mzdb->stmt, 0));
            string version = string((const char*) sqlite3_column_text(_mzdb->stmt, 1));
            const char_t *result = (const char_t *) sqlite3_column_text(_mzdb->stmt, 2);

			SoftwarePtr s(new Software(id));
			s->version = version;
            IDeserializer::setParams(*s, result);
			_msdata.softwarePtrs.push_back(s);
        }
        sqlite3_finalize(_mzdb->stmt);
        _mzdb->stmt = 0;

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
        sqlite3_prepare_v2(_mzdb->db, sql_4, -1, &(_mzdb->stmt), 0);
        while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
            string name = string((const char*) sqlite3_column_text(_mzdb->stmt, 0));
            string location = string((const char*) sqlite3_column_text(_mzdb->stmt, 1));
            const char_t* r = (const char_t*) sqlite3_column_text(_mzdb->stmt, 2);
            string id = "RAW1";
            SourceFilePtr s(new SourceFile(id, name, location));
            IDeserializer::setParams(*s, r);
			_msdata.fileDescription.sourceFilePtrs.push_back(s);
			_msdata.run.defaultSourceFilePtr = s;
            if (! _msdata.scanSettingsPtrs.empty())
                _msdata.scanSettingsPtrs[0]->sourceFilePtrs.push_back(s);
		}
        sqlite3_finalize(_mzdb->stmt);
        _mzdb->stmt = 0;

        _msdata.id = _msdata.run.defaultSourceFilePtr->name;
        //printf("after sourceFile\n");

		//dataProcessing
        vector<DataProcessingPtr>& vDataProc = _msdata.dataProcessingPtrs;
		const char* sql_5 = "SELECT name FROM data_processing";
        sqlite3_prepare_v2(_mzdb->db, sql_5, -1, &(_mzdb->stmt), 0);
        while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
            const char* r = (const char*) sqlite3_column_text(_mzdb->stmt, 0);
			DataProcessingPtr d(new DataProcessing(string(r)));
            vDataProc.push_back(d);
            //By default dataProcessing does not exist in current msdata
            //vector of data processing (see pwiz implementation) but is owned by spectrumListPtr
            //_msdata.dataProcessingPtrs.push_back(d);
            //_msdata.run.spectrumListPtr->

		}
        sqlite3_finalize(_mzdb->stmt);
        _mzdb->stmt = 0;

        //printf("after data processing\n");
        if (vDataProc.size() == 1) {
            ((mzSpectrumList*)_msdata.run.spectrumListPtr.get())->setDataProcessingPtr(vDataProc.front());
           // ((mzChromatogramList*)_msdata.run.chromatogramListPtr.get())->setDataProcessingPtr(vDataProc.front());
        } else if ( vDataProc.size() > 1){
            printf("Dont know which dataProcessing to attribute to SpectrumListPtr\n, set the last one by default\n");
            ((mzSpectrumList*)_msdata.run.spectrumListPtr.get())->setDataProcessingPtr(vDataProc.back());
           // ((mzChromatogramList*)_msdata.run.chromatogramListPtr.get())->setDataProcessingPtr(vDataProc.back());
        } else {
            printf("Missing a dataProcessing; this is never supposed to happen\n");
        }
        //printf("after setting dataProcessing\n");

		//processing method
        const char* sql_6 = "SELECT id, param_tree, data_processing_id, software_id FROM processing_method";
        sqlite3_prepare_v2(_mzdb->db, sql_6, -1, &(_mzdb->stmt), 0);
        while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(_mzdb->stmt, 0);
            const char_t* r = (const char_t*) sqlite3_column_text(_mzdb->stmt, 1);
            int dataProcessingID = sqlite3_column_int(_mzdb->stmt, 2);
            int softwareID = sqlite3_column_int(_mzdb->stmt, 3);

			ProcessingMethod p;
            p.order = id;
			p.softwarePtr = _msdata.softwarePtrs[softwareID - 1];
            _msdata.allDataProcessingPtrs()[dataProcessingID - 1]->processingMethods.push_back(p);
            IDeserializer::setParams(p, r);
		}
        sqlite3_finalize(_mzdb->stmt);
        _mzdb->stmt = 0;
        //printf("after processing method\n");

		//instrument Config
        const char* sql_7 = "SELECT name, param_tree, software_id, component_list FROM instrument_configuration";
        sqlite3_prepare_v2(_mzdb->db, sql_7, -1, &(_mzdb->stmt), 0);
        while (sqlite3_step(_mzdb->stmt) == SQLITE_ROW) {
            const char* name = (const char*) sqlite3_column_text(_mzdb->stmt, 0);

            int softwareID = sqlite3_column_int(_mzdb->stmt, 2);
			InstrumentConfigurationPtr ic(new InstrumentConfiguration(string(name)));
			ic->softwarePtr = _msdata.softwarePtrs[softwareID - 1];
            const char_t* tree = (const char_t*) sqlite3_column_text(_mzdb->stmt, 1);
            if (tree) {
                IDeserializer::setParams(*ic, tree);
            }
            const char_t* componentListString = (const char_t*) sqlite3_column_text(_mzdb->stmt, 3);
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
			_msdata.instrumentConfigurationPtrs.push_back(ic);
		}
        sqlite3_finalize(_mzdb->stmt);
        _mzdb->stmt = 0;

        if ( ! _msdata.instrumentConfigurationPtrs.empty()) {
			_msdata.run.defaultInstrumentConfigurationPtr = _msdata.instrumentConfigurationPtrs[0];
        }
		_msdata.run.id = _msdata.id;
	}

}//end namespace





