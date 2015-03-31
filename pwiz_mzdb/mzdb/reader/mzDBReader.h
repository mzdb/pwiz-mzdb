/* 
 * File:   mzDBReader.h
 * Author: Marco
 *
 * Created on 21 juin 2012, 14:08
 */

#ifndef MZDBREADER_H
#define	MZDBREADER_H

#include "pwiz/data/msdata/MSDataFile.hpp"
#include "pwiz/data/msdata/ChromatogramListBase.hpp"


#include <stdio.h>
#include <cstdio>
#include <iostream>
#include "boost/algorithm/string.hpp"

#include "../lib/sqlite3/include/sqlite3.h"
#include "../Lib/pugixml/include/pugixml.hpp"

#include "../utils/mzUtils.hpp"
#include "../utils/mzDBFile.h"

using namespace std;
using namespace pwiz::msdata;

namespace mzdb {

    class PWIZ_API_DECL mzDBReader {

        /* empty chromatogram List */
        struct mzEmptyChromatogram: public pwiz::msdata::ChromatogramListBase {

            pwiz::msdata::ChromatogramIdentity ci;

            mzEmptyChromatogram(){}

            inline size_t size() const {return 0;}

            inline bool empty() const { return true;}

            inline const pwiz::msdata::ChromatogramIdentity& chromatogramIdentity(size_t index) const {
                return ci;
            }

            inline pwiz::msdata::ChromatogramPtr chromatogram(size_t index, bool getBinaryData = false) const {
                pwiz::msdata::ChromatogramPtr c(new pwiz::msdata::Chromatogram);
                return c;
            }
        };

        class UserFeedbackIterationListener : public pwiz::util::IterationListener
        {
            public:

            virtual Status update(const UpdateMessage& updateMessage)
            {
                // add tabs to erase all of the previous line
                std::cout << updateMessage.iterationIndex+1 << "/" << updateMessage.iterationCount << "\t\t\t\r" << std::flush;

                // spectrum and chromatogram lists both iterate; put them on different lines
                if (updateMessage.iterationIndex+1 == updateMessage.iterationCount)
                    std::cout << std::endl;
                return pwiz::util::IterationListener::Status_Ok;
            }
        };


        MzDBFile& _mzdb;
        bool _noLoss;

        void fillInMetaData(MSData&);


      public:
        PWIZ_API_DECL mzDBReader(MzDBFile&);
        //PWIZ_API_DECL ~mzDBReader(){printf("mzDBReader destrcutor\n");}

        /* main function read spectra, chromatograms and metadata */
        PWIZ_API_DECL void readMzDB(MSData& _msdata, bool cache = false);

        PWIZ_API_DECL inline string& filename() {return _mzdb.name;}

        /* convert the mzdb into mzML or mzXML or mz5 */
        PWIZ_API_DECL void convertTo(CVID cvid=pwiz::msdata::MS_mzML_format, bool noLoss=false);

         PWIZ_API_DECL void enumerateSpectra();

    };

} //end namespace


#endif	/* MZDBREADER_H */
