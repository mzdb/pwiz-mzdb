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
    const xml_node& n_ = doc.child(params_str).child(cv_params_str);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(access_nb_str).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(param_value_str).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
}



void setCvParams(ParamContainer& t, const xml_node& n)  {
    const xml_node& n_ = n.child(params_str).child(cv_params_str);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(access_nb_str).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(param_value_str).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
}


void setUserParams(ParamContainer& t, const char_t* n) {
    xml_document doc;
    auto parseResult = doc.load(n);
    const xml_node& n_ = doc.child(params_str).child(user_params_str);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string name, value, type;
        name = string(it->attribute(param_name_str).value());
        value = string(it->attribute(param_value_str).value());
        type = string(it->attribute(param_type_str).value());
        t.userParams.push_back(UserParam(name, value, type));
    }
}


void setUserParams(ParamContainer& t, const xml_node& n) {
    const xml_node& n_ = n.child(params_str).child(user_params_str);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        t.userParams.push_back(UserParam(string(it->attribute(param_name_str).value()),
                                         string(it->attribute(param_value_str).value()),
                                         string(it->attribute(param_type_str).value())));
    }
}


void setParams(ParamContainer& t, const pugi::char_t* n) {
    xml_document doc;
    auto parseResult = doc.load(n);
    if (! parseResult) {
        printf("Error parsing paramTree...can not set params\n");
    }
    const xml_node& n_ = doc.child(params_str).child(cv_params_str);
    for (auto it = n_.children().begin(); it != n_.children().end(); ++it) {
        string accession, value;
        accession = string(it->attribute(access_nb_str).value());
        int cvid = boost::lexical_cast<int>(accession);
        value = string(it->attribute(param_value_str).value());
        t.cvParams.push_back(CVParam((pwiz::cv::CVID) cvid, value));
    }
    const xml_node& n__ = doc.child(params_str).child(user_params_str);
    for (auto it = n__.children().begin(); it != n__.children().end(); ++it) {
        string name, value, type;
        name = string(it->attribute(param_name_str).value());
        value = string(it->attribute(param_value_str).value());
        type = string(it->attribute(param_type_str).value());
        t.userParams.push_back(UserParam(name, value, type));
    }
}



}


}
