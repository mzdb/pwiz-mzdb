#include "mzSpectrumList.h"

#include "../../utils/mzIDeserializer.h"

#include <memory>

using namespace std;
using namespace pwiz::msdata;

namespace mzdb {


    mzSpectrumList::mzSpectrumList(MzDBFile& mzdb, MSData* msd, bool cache_, bool isNoLoss_) :
        _db(mzdb.db), _stmt(mzdb.stmt), _msdata(msd), _isCached(cache_), _isNoLoss(isNoLoss_) {

            /*const char* sql = "select max(id) from spectrum where ms_level = 1;";
            sqlite3_prepare_v2(this->_db, sql, -1, &(this->_stmt), 0);
            sqlite3_step(this->_stmt);
            this->_lastMs1Id = sqlite3_column_int(this->_stmt, 0);
            sqlite3_finalize(this->_stmt);
            this->_stmt = 0;
            this->_hasReachedLastMs2 = false;*/

            this->buildSpectrumIdentities();
            this->initIteration();
	}

    void mzSpectrumList::buildSpectrumIdentities() {
		//need to do that
        const char* sql = "SELECT spectrum.*, data_encoding.mode FROM spectrum,"
                "data_encoding WHERE spectrum.data_encoding_id = data_encoding.id "
                "ORDER BY spectrum.id";
        sqlite3_prepare_v2(this->_db, sql, -1, &(this->_stmt), 0);

        while (sqlite3_step(this->_stmt) == SQLITE_ROW) {

			mzSpectrumIdentity *si = new mzSpectrumIdentity;
            int idMzDB = sqlite3_column_int(this->_stmt, 0);
			si->idMzDB = idMzDB;
			si->index = idMzDB - 1;
            si->id = string((const char*) sqlite3_column_text(this->_stmt, 1));
			si->spotID = "";
            si->title = string((const char*) sqlite3_column_text(this->_stmt, 2));
            si->cycle = sqlite3_column_int(this->_stmt, 3);
            si->time = static_cast<float> (sqlite3_column_double(this->_stmt, 4));
            si->msLevel = sqlite3_column_int(this->_stmt, 5);

            int type = sqlite3_column_type(this->_stmt, 6);
			switch (type) {
			case 3:
                si->activation = string((const char*) sqlite3_column_text(this->_stmt, 6));
				break;
			case 5:
				si->activation = "";
				break;
			default:
				printf("Error getting spectrumIdentity(activation)");
			}
            si->tic = sqlite3_column_double(this->_stmt, 7);
            int type_ = sqlite3_column_type(this->_stmt, 8);
			switch (type_) {
			case 2:
                si->basePeakMz = sqlite3_column_double(this->_stmt, 8);
				break;
			case 5:
				si->basePeakMz = -1;
				break;
			default:
				printf("Error getting spectrumIdentity(basePeakMz)\n");
			}
			if (type_ == 5)
				si->basePeakIntensity = -1;
			else {
                si->basePeakIntensity = sqlite3_column_double(this->_stmt, 9);
			}

            int type__ = sqlite3_column_type(this->_stmt, 10);
			//printf("type:%d\n", type__);
			switch (type__) {
			case 2:
                si->precursorMz = sqlite3_column_double(this->_stmt, 10); ;
				break;
			case 5:
				si->precursorMz = -1;
				break;
			default:
				printf("Error getting spectrumIdentity(precursorMz)");
			}
			if (type__ == 5)
				si->precursorCharge = -1;
			else
                si->precursorCharge = sqlite3_column_double(this->_stmt, 11);
            si->dataPointsCount = sqlite3_column_int(this->_stmt, 12);

            //read paramTree
            string paramTree = string((const char*) sqlite3_column_text(this->_stmt, 13));
            IDeserializer::setParams(*si, paramTree.c_str());
            si->scansParamTree = (const char*)sqlite3_column_text(this->_stmt, 14);
            si->precursorsParamTree = (const char*)sqlite3_column_text(this->_stmt, 15);
            si->productsParamTree = (const char*)sqlite3_column_text(this->_stmt, 16);

            if (si->userParam("in_high_res").empty()) {
                printf("Empty high res user params, never supposed to happen\n");
                throw exception("Empty high res user params, never supposed to happen\n");
            }
            si->isInHighRes = (si->userParam("in_high_res").value == "true") ? true : false;
			PeakEncoding pe;
            if ( this->_isNoLoss )
				pe = NO_LOSS_PEAK;
			else if (si->isInHighRes)
				pe = HIGH_RES_PEAK;
            else {
				pe = LOW_RES_PEAK;
            }

            si->sourceFileID = sqlite3_column_int(this->_stmt, 19);
            si->dataProcessingID = sqlite3_column_int(this->_stmt, 21);
            si->dataEncodingID = sqlite3_column_int(this->_stmt, 22);
            si->bbFirstScanID = sqlite3_column_int(this->_stmt, 23);

            DataMode mode;
            string dataEncodingString = string((const char*) sqlite3_column_text(this->_stmt, 24));
			if (dataEncodingString == "centroided")
				mode = CENTROID;
            else if (dataEncodingString == "profile")
				mode = CENTROID; // treated a the same than centroid mode
            else if (dataEncodingString == "fitted")
				mode = FITTED;
			else
                throw exception("[buildSpectrumIdentities] encoding string is not one of 'centroided', 'profile', 'fitted'\n");
			//add stuff
            si->encodingMode = DataEncoding(idMzDB, mode, pe);
            //fill dataEncodings Map
            this->_dataEncodings[idMzDB] = si->encodingMode;
			_spectrumIdentities.push_back(si);
		}
        sqlite3_finalize(this->_stmt);
        this->_stmt = 0;

	}

    void mzSpectrumList::initIteration() {
        const char* sql = "SELECT bounding_box.* FROM bounding_box, spectrum WHERE spectrum.id = bounding_box.first_spectrum_id";
        sqlite3_prepare_v2(this->_db, sql, -1, &(this->_stmt), 0);
        //this->_scanIdx = 0;
        this->_overallScanIndex = 0;
        //this->_currentIndex = 0;
		
        while (this->_nextScans.empty()) {
            sqlite3_step(this->_stmt);
            nextBoundingBox(this->_nextScans);//fill the first bb
		}
        this->loadBoundingBoxes();
	}

    SpectrumPtr mzSpectrumList::spectrum(size_t index, bool getBinaryData) const {
        mzSpectrumList* s = (const_cast<mzSpectrumList*> (this));
		/*
		string str = boost::lexical_cast<string > (index);
		
		if ((size_t)s->cache == _spectrumIdentities.size()) {
			SpectrumPtr ptr(new Spectrum);
			ptr->scanList.set(MS_no_combination);

			mzSpectrumIdentity* si = (mzSpectrumIdentity*) _spectrumIdentities[index];

			float elutionTime = si->time;
			ptr->id = si->id;

			Scan scanPwiz;
			scanPwiz.spectrumID = si->id;
			scanPwiz.set(MS_scan_start_time, elutionTime, UO_second);
			ptr->scanList.scans.push_back(scanPwiz);
			s->cacheDB.getValue(str, ptr);
			return ptr;
		}*/
		SpectrumPtr p = s->next();
        //References::resolve(*p, *_msdata);
		//s->cacheDB.addKeyValue(str, p);
		//s->cache++;
		return p;

	}


    void mzSpectrumList::nextBoundingBox(vector<mzScan*>& scans) {

        byte* data = (byte*) sqlite3_column_blob(this->_stmt, 1);
        int size = sqlite3_column_bytes(this->_stmt, 1);

        int firstScanID = sqlite3_column_int(this->_stmt, 3);
		mzSpectrumIdentity* si = (mzSpectrumIdentity*) _spectrumIdentities[firstScanID - 1];
        int currentMsLevel = si->msLevel;
		map<int, vector<int> > scansInfos;

        int nbScans = IBlobReader::buildMapPositions(data, size, scansInfos, this->_dataEncodings);

		for (int i = 1; i <= nbScans; ++i) {
            int id = scansInfos[i][0];

            mzSpectrumIdentity* specIdent =  (mzSpectrumIdentity*) this->_spectrumIdentities[ id - 1 ];

            mzScan* s = new mzScan(specIdent->idMzDB, currentMsLevel, specIdent->time);

            IBlobReader::readData(data, size, i, s, scansInfos);
            if (currentMsLevel == 1) {
                scans.push_back(s);
            } else {
                //direct add to the mslevel > 1 (one spectrum per bounding box)
               this->_msnBuffers[currentMsLevel].push_back(s);
               this->_maxIdMzdbByMsLevel[currentMsLevel] = s->idMzDB;

           }
		}

	}


    void mzSpectrumList::loadBoundingBoxes() {

        this->_currentScans = this->_nextScans;
        //scanIdx = 0;

        while (sqlite3_step(this->_stmt) == SQLITE_ROW) {

            vector<mzScan*> scans;
            this->nextBoundingBox(scans);

			if (scans.empty()) {
				//MS2 or we finish
                if (this->_overallScanIndex == _spectrumIdentities.size()) {
					//we finished
                    this->_hasNext = false;
					return;
				}
				//ms2 buffer filled;
				continue;
			}

            if (this->_currentScans[0]->idMzDB == scans[0]->idMzDB) {

				for (size_t i = 0; i < scans.size(); i++) {
                    this->_currentScans[i]->addScanData(scans[i]);
                    delete scans[i];
				}
            } else {
                this->_nextScans = scans;
				break;
			}
		}
        for (size_t i = 0, l = this->_currentScans.size(); i < l; ++i ) {
            mzScan* s = this->_currentScans[i];
            int msLevel = s->msLevel;
            this->_msnBuffers[msLevel].push_back(s);
            this->_maxIdMzdbByMsLevel[msLevel] = s->idMzDB;
        }
	}

    SpectrumPtr mzSpectrumList::next() {
	
        mzSpectrumIdentity* si = (mzSpectrumIdentity*) this->_spectrumIdentities[this->_overallScanIndex];
        int msLevel = si->msLevel;
        int& maxIdForMsLevel = this->_maxIdMzdbByMsLevel[msLevel];
        while (si->idMzDB > maxIdForMsLevel) {
            this->loadBoundingBoxes();

        }
        deque<mzScan*>& m = this->_msnBuffers[msLevel];
        mzScan* scan = m.front();
        m.pop_front();

		
        if(! scan) {
            printf("Fatal error in iteration...exiting\n");
            exit(0);
        }


        this->_overallScanIndex++;
        //initiate a new scan
        
        scan->encoding = si->encodingMode;//JUST ADD to see if it fits;
        SpectrumPtr ptr(scan);
        ptr->dataProcessingPtr = dp_; //sets the dataProcessingPtr could be null


		ptr->id = si->id;
        ptr->sourceFilePtr = _msdata->run.defaultSourceFilePtr;

		//cvParams
        ptr->cvParams = si->cvParams;
        ptr->userParams = si->userParams;

        //precursor;
        /*
        istringstream is(string(si->precursorsParamTree));
        Precursor prec;
        IO::read(is, prec);
        ptr->precursors.push_back(prec);
        Product prod;
        istringstream is_2(string(si->productsParamTree));
        IO::read(is_2, prod);
        ptr->products.push_back(prod);
        ScanList scanList;
        istringstream is_3(string(si->scansParamTree));
        IO::read(is_3, scanList);
        ptr->scanList = scanList;*/
        ptr->index = scan->idMzDB - 1;
        ptr->defaultArrayLength = scan->mz.size();

//        if ( ptr->hasCVParam(MS_profile_spectrum) ) {
//            ptr->cvParams.erase(std::remove_if(ptr->cvParams.begin(),
//                                               ptr->cvParams.end(),
//                                               [](const CVParam& cv)->bool {
//                                                    if (cv.cvid == MS_profile_spectrum)
//                                                        return true;
//                                                    else
//                                                        return false;
//                                               }
//                                ),
//                                ptr->cvParams.end());
//            }
        ptr->cvParams.push_back(CVParam(MS_centroid_spectrum, ""));
        Scan s;
        s.cvParams.push_back(CVParam(MS_scan_start_time, boost::lexical_cast<string>(scan->rt), UO_second));
        ptr->scanList.scans.push_back(s);
        ptr->setMZIntensityArrays(scan->mz, scan->intensities, CVID::MS_intensity_unit);

        //currentIndex++;

        return ptr;
    }

    mzSpectrumList::~mzSpectrumList() {
        for(size_t i =0; i < _spectrumIdentities.size(); ++i) {
            delete _spectrumIdentities[i];
        }
    }



}
