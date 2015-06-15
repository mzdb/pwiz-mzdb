#ifndef MZMETADATAREADER_H
#define MZMETADATAREADER_H

#include "pwiz/data/msdata/MSData.hpp"

#include "../lib/pugixml/include/pugixml.hpp"

using namespace std;

namespace mzdb {


    /**
     * Static functions for setting _pwiz cvParams_ object from
     * XML raw char* or parsed into pugi_xml node objects.
     *
     * Basically, the XML is coming from _param tree_ column stored
     * inb the mzDB, which are litterally XML chunks
     */
    namespace IDeserializer {

        /**
         * create cvParams and attach it to the _pwiz paramContainer_ object
         * from raw char pointer.
         */
        void setCvParams(pwiz::msdata::ParamContainer&, const pugi::char_t*);

        /**
         * create cvParams and attach it to the _pwiz paramContainer_ object
         * from XML parsed into pugi data structure.
         */
        void setCvParams(pwiz::msdata::ParamContainer&, const pugi::xml_node&);

        /**
         * create userParams and attach it to the _pwiz paramContainer_ object
         * from raw char pointer.
         */
        void setUserParams(pwiz::msdata::ParamContainer&, const pugi::char_t*);

        /**
         * create userParams and attach it to the _pwiz paramContainer_ object
         * from XML parsed into pugi data structure.
         */
        void setUserParams(pwiz::msdata::ParamContainer&, const pugi::xml_node&);
        /* for the moment this is implemented in the mzdbfile class but may be
         * in the future implemented here
        PWIZ_API_DECL bool isNoLoss(MzDBFile*);

        PWIZ_API_DECL pair<double, double> getBBSize(MzDBFile*, int);*/

        /**
         * Wrapper for setting on the same time _cvParams_ and _userParams_
         * to a _pwiz paramContainer_ from a param tree XML chunk.
         */
        void setParams(pwiz::msdata::ParamContainer &, const pugi::char_t*);

    }
}


#endif // MZMETADATAREADER_H

