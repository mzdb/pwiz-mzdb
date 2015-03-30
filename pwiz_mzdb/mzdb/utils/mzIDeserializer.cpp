#include "mzIDeserializer.h"
#include "mzUtils.hpp"

namespace mzdb {

namespace IDeserializer {

using namespace pwiz::msdata;
using namespace pugi;


void setCvParams(ParamContainer& t, const char_t* n)  {
    xml_document doc;
    auto parseResult = doc.load(n);
    if (! parseResult) {
        printf("Error parsing paramTree...can not set cvParams\n");
    }
    const xml_node& n_ = doc.child(PARAMS_STR).child(CV_PARAMS_STR);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(ACCESS_NB_STR).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(PARAM_VALUE_STR).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
}



void setCvParams(ParamContainer& t, const xml_node& n)  {
    const xml_node& n_ = n.child(PARAMS_STR).child(CV_PARAMS_STR);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(ACCESS_NB_STR).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(PARAM_VALUE_STR).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
}


void setUserParams(ParamContainer& t, const char_t* n) {
    xml_document doc;
    auto parseResult = doc.load(n);
    const xml_node& n_  = doc.child(PARAMS_STR).child(USER_PARAMS_STR);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string name, value, type;
        name = string(it->attribute(PARAM_NAME_STR).value());
        value = string(it->attribute(PARAM_VALUE_STR).value());
        type = string(it->attribute(PARAM_TYPE_STR).value());
        t.userParams.push_back(UserParam(name, value, type));
    }
}


void setUserParams(ParamContainer& t, const xml_node& n) {
    const xml_node& n_ = n.child(PARAMS_STR).child(USER_PARAMS_STR);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        t.userParams.push_back(UserParam(string(it->attribute(PARAM_NAME_STR).value()),
                                                             string(it->attribute(PARAM_VALUE_STR).value()),
                                                             string(it->attribute(PARAM_TYPE_STR).value())));
    }
}


void setParams(ParamContainer& t, const pugi::char_t* n) {
    xml_document doc;
    auto parseResult = doc.load(n);
    if (! parseResult) {
        printf("Error parsing paramTree...can not set params\n");
    }
    const xml_node& n_ = doc.child(PARAMS_STR).child(CV_PARAMS_STR);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(ACCESS_NB_STR).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(PARAM_VALUE_STR).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
    const xml_node& n__ = doc.child(PARAMS_STR).child(USER_PARAMS_STR);
    for (auto it = n__.children().begin(); it != n__.children().end(); ++it) {
        string name, value, type;
        name = string(it->attribute(PARAM_NAME_STR).value());
        value = string(it->attribute(PARAM_VALUE_STR).value());
        type = string(it->attribute(PARAM_TYPE_STR).value());
        t.userParams.push_back(UserParam(name, value, type));
    }
}



}


}
