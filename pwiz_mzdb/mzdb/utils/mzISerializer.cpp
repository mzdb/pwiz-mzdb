#include "mzISerializer.h"
#include "boost/algorithm/string.hpp"
#include "mzUtils.hpp"

namespace mzdb {

namespace ISerializer {

using namespace pugi;
using namespace pwiz::msdata;

/** */
void getCvParams(const ParamContainer& object, xml_node& doc) {
    const vector<CVParam>& params = object.cvParams;
    if (params.empty())
        return;

    auto node = doc.append_child(CV_PARAMS_STR);

    for (unsigned int i = 0; i < params.size(); ++i) {
        const CVParam& param = params[i];
        string n = param.name();
        boost::replace_all(n, " ", "_");

        if (param.empty()) {
            continue;
        }

        auto n_ = node.append_child(CV_PARAM_STR);
        n_.append_attribute(CV_REF_STR).set_value(MS_NS_STR);
        n_.append_attribute(ACCESS_NB_STR).set_value(param.cvid); // concatener "MS:no Accession"
        n_.append_attribute(PARAM_NAME_STR).set_value(param.name().c_str());

        if (strcmp(param.value.c_str(), "") == 0)
            // does not append attribute value if it is empty
            continue;
        else {
            try {
                if (boost::find_first(param.value, "."))
                    n_.append_attribute(PARAM_VALUE_STR).set_value(param.valueAs<double>());
                else if (strcmp(param.value.c_str(), "") == 0 )
                    n_.append_attribute(PARAM_VALUE_STR).set_value("");
                else
                    n_.append_attribute(PARAM_VALUE_STR).set_value(param.valueAs<int>());

            } catch (boost::bad_lexical_cast &) {
                try {
                    if (param.value ==  TRUE_STR || param.value == FALSE_STR )
                        n_.append_attribute(PARAM_VALUE_STR).set_value(param.valueAs<bool>());
                    else
                        n_.append_attribute(PARAM_VALUE_STR).set_value(param.value.c_str());
                } catch (boost::bad_lexical_cast&) {
                    printf("Bad lexical cast raised: wrong cvparam value.\n");
                }
            }
        }
    }
}


/** */
void getUserParams(const ParamContainer& object, xml_node& doc) {
    const vector<UserParam>& params = object.userParams;
    if (params.empty())
        return;

    auto params_node = doc.append_child(USER_PARAMS_STR);
    for (unsigned int i = 0; i < params.size(); ++i) {
        string name = params[i].name;
        boost::replace_all(name, " ", "_");

        if (params[i].empty()) {
            continue;
        }

        if (params[i].name == "instrumentMethods" || params[i].name == "errorLog") {
            auto texts_node = doc.child(USER_TEXTS_STR);

            if (texts_node.empty())
                texts_node = doc.append_child(USER_TEXTS_STR);


            auto n = texts_node.append_child(USER_TEXT_STR);
            n.append_attribute(CV_REF_STR).set_value(MS_NS_STR);
            n.append_attribute(ACCESS_NB_STR).set_value(params[i].units); //TODO concatener "MS:no Accession"
            n.append_attribute(PARAM_NAME_STR).set_value(params[i].name.c_str());
            n.append_attribute(PARAM_TYPE_STR).set_value(params[i].type.c_str());
            n.text() = params[i].value.c_str();

        } else {
            auto n = params_node.append_child(USER_PARAM_STR);
            n.append_attribute(CV_REF_STR).set_value(MS_NS_STR);
            n.append_attribute(ACCESS_NB_STR).set_value(params[i].units); //TODO concatener "MS:no Accession"
            n.append_attribute(PARAM_NAME_STR).set_value(params[i].name.c_str());
            n.append_attribute(PARAM_TYPE_STR).set_value(params[i].type.c_str());
            try {
                if (boost::find_first(params[i].value, "."))
                    n.append_attribute(PARAM_VALUE_STR).set_value(params[i].valueAs<double>());
                else if (strcmp(params[i].value.c_str(), "") == 0 )
                    n.append_attribute(PARAM_VALUE_STR).set_value("");
                else
                    n.append_attribute(PARAM_VALUE_STR).set_value(params[i].valueAs<int>());
            } catch (boost::bad_lexical_cast &) {
                try {
                    if (params[i].value == TRUE_STR || params[i].value == FALSE_STR)
                        n.append_attribute(PARAM_VALUE_STR).set_value(params[i].valueAs<bool>());
                    else
                        n.append_attribute(PARAM_VALUE_STR).set_value(params[i].value.c_str());
                } catch (boost::bad_lexical_cast &) {
                    printf("Bad lexical cast raised: wrong user param value.\n");
                }
            }
        }
    }
}



string serialize(const ParamContainer& e, xml_string_writer &writer) {
    xml_document doc;
    auto n = doc.append_child(PARAMS_STR);
    getCvParams(e, n);
    getUserParams(e, n);
    doc.print(writer, "  ");//, "\t", format_raw | format_no_declaration); //no namespace no charriot char

	// Add "MS:" prefix 
    string runForAddMS = writer.getResult();
    if (!runForAddMS.empty()){
        string strCvRef("cvRef=\"");
        string strAccession("accession=\"");
        size_t foundPositionCvRef = runForAddMS.find(strCvRef);
        size_t foundPositionAccession = runForAddMS.find(strAccession);
        while (foundPositionCvRef != string::npos && foundPositionAccession != string::npos && runForAddMS.substr(foundPositionCvRef+7, 2) == "MS" && runForAddMS.substr(foundPositionAccession+11, 3) != "MS:"){ // add to all accession values
            runForAddMS.insert(foundPositionAccession+11,"MS:");
            foundPositionCvRef = runForAddMS.find(strCvRef, foundPositionCvRef+1);
            foundPositionAccession = runForAddMS.find(strAccession, foundPositionAccession+1);
        }
    }
    string& writerFixMS = runForAddMS;
    return runForAddMS;
}


}
