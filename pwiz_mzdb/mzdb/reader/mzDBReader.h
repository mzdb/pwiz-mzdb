/* 
 * File:   mzDBReader.h
 * Author: Marco
 *
 * Created on 21 juin 2012, 14:08
 */

#ifndef MZDBREADER_H
#define	MZDBREADER_H

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/data/msdata/ChromatogramListBase.hpp"


#include <stdio.h>
#include <cstdio>
#include "boost/algorithm/string.hpp"

#include "../lib/sqlite3/include/sqlite3.h"
#include "../Lib/pugixml/include/pugixml.hpp"

#include "../utils/mzUtils.hpp"
#include "../utils/MzDBFile.h"


namespace mzdb {
    using namespace std;
    using namespace pwiz::msdata;


    class PWIZ_API_DECL mzDBReader {

        struct mzEmptyChromatogram: public pwiz::msdata::ChromatogramListBase {
            pwiz::msdata::ChromatogramIdentity ci;
            mzEmptyChromatogram(){}
            size_t size() const {return 0;}
            bool empty() const { return true;}
            const pwiz::msdata::ChromatogramIdentity& chromatogramIdentity(size_t index) const {
                return ci;
            }
            pwiz::msdata::ChromatogramPtr chromatogram(size_t index, bool getBinaryData = false) const {
                pwiz::msdata::ChromatogramPtr c(new pwiz::msdata::Chromatogram);
                return c;
            }
        };


        MzDBFile* _mzdb;
        bool _noLoss;

        void fillInMetaData(MSData&);


      public:
        PWIZ_API_DECL mzDBReader(MzDBFile*);
        //PWIZ_API_DECL ~mzDBReader(){printf("mzDBReader destrcutor\n");}

        /** main function read spectra, chromatograms and metadata */
        PWIZ_API_DECL void readMzRTreeDB(MSData& _msdata, bool cache = false);

        PWIZ_API_DECL inline string& filename() {return _mzdb->name;}
    };

    



} //end namespace


#endif	/* MZDBREADER_H */

