#ifndef MZUSERTEXT_H
#define MZUSERTEXT_H


#include "pwiz/data/msdata/MSData.hpp"


//we introduce here a new kind of param called UserText: metadata specifically
//contained in the raw file that we wanted to be in the mzDB. It is stored as
//plain text
struct UserText : public pwiz::msdata::UserParam {
    inline UserText(const std::string& s="", const std::string& s_="", const std::string& s__=""): pwiz::msdata::UserParam(s, s_, s__){

    }
};
#endif // MZUSERTEXT_H
