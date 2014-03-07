#ifndef MZMETADATAREADER_H
#define MZMETADATAREADER_H

#include "pwiz/data/msdata/MSData.hpp"

#include "../lib/pugixml/include/pugixml.hpp"

namespace mzdb {

    using namespace std;

    /**
     *Previous version were using template instead of the ParamContainer
     */
    namespace IDeserializer {


        void setCvParams(pwiz::msdata::ParamContainer&, const pugi::char_t*);


        void setCvParams(pwiz::msdata::ParamContainer&, const pugi::xml_node&);


        void setUserParams(pwiz::msdata::ParamContainer&, const pugi::char_t*);


        void setUserParams(pwiz::msdata::ParamContainer&, const pugi::xml_node&);
        /* for the moment this is implemented in the mzdbfile class but may be
         * in the future implemented here
        PWIZ_API_DECL bool isNoLoss(MzDBFile*);

        PWIZ_API_DECL pair<double, double> getBBSize(MzDBFile*, int);*/


        void setParams(pwiz::msdata::ParamContainer &, const pugi::char_t*);

    }
}


#endif // MZMETADATAREADER_H

