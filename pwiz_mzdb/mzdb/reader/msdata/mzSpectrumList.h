/* 
* File:   mzSpectrumList.h
* Author: marco
*
* Created on 5 juillet 2012, 15:15
*/

#ifndef MZSPECTRUMLIST_H
#define	MZSPECTRUMLIST_H


#include <stdio.h>
#include <deque>

#include "pwiz/data/msdata/SpectrumListBase.hpp"
#include "pwiz/data/msdata/References.hpp"
#include "pwiz/data/msdata/IO.hpp"

#include "../../lib/sqlite3/include/sqlite3.h"

#include "mzScan.hpp"
#include "../../utils/mzUtils.hpp"
#include "../../utils/mzDBFile.h"
#include "../mzIBlobReader.hpp"
#include "mzDataCache.h"

namespace mzdb {

	using namespace std;

	/**
	Struct that maps what is in the database, inherit pwiz class SpectrumIdentity
	*/
    struct mzSpectrumIdentity : public pwiz::msdata::SpectrumIdentity, public pwiz::msdata::ParamContainer {
		int idMzDB;
		float time;
		string title;
		int cycle;
		int msLevel;
		string activation;
		float tic;
		double basePeakMz;
		float basePeakIntensity;
		double precursorMz;
		float precursorCharge;
		int dataPointsCount;
		int sourceFileID;
		int dataProcessingID;
		int dataEncodingID;
		int bbFirstScanID;
		bool isInHighRes;
		DataEncoding encodingMode;
        const char* scansParamTree;
        const char* productsParamTree;
        const char* precursorsParamTree;
	};


    class mzSpectrumList : public pwiz::msdata::SpectrumListBase {

	protected:

        vector<pwiz::msdata::SpectrumIdentity*> _spectrumIdentities;
        //MzDBFile* _mzdb;
        struct sqlite3* _db;
        struct sqlite3_stmt* _stmt;
        pwiz::msdata::MSData* _msdata;

        //unsigned int _scanIdx;
        unsigned int _overallScanIndex;

		//map containing currentScans; key correspond to the msLevel
        vector<mzScan*> _currentScans;
        vector<mzScan*> _nextScans;

        bool _hasNext;
        //string _sqlQuery;
        map<int, DataEncoding> _dataEncodings;

        bool _isCached, _isNoLoss;
        /*
        int _lastMs1Id;
        bool _hasReachedLastMs2;
        */
		//mzDataCache cacheDB;
        map<int, deque<mzScan*> > _msnBuffers;
        map<int, int> _maxIdMzdbByMsLevel;
        //int cache;


	public:
        /**interface from SpectrumList */
        mzSpectrumList(MzDBFile& mzdb, pwiz::msdata::MSData* msd, bool cache = false, bool isNoLoss_ = false);

        ~mzSpectrumList();

		//interface herited by SpectrumListBase
        pwiz::msdata::SpectrumPtr spectrum(size_t index, bool getBinaryData) const;

        size_t size() const {
            return this->_spectrumIdentities.size();
        }
       
        const pwiz::msdata::SpectrumIdentity& spectrumIdentity(size_t index) const {

            return *(this->_spectrumIdentities[index + 1]);
        }




	protected:

		void buildSpectrumIdentities();

		void initIteration() ;

        void nextBoundingBox(vector<mzScan*>& scans);

        void loadBoundingBoxes() ;

        pwiz::msdata::SpectrumPtr next();
	};





}

#endif	/* MZSPECTRUMLIST_H */

