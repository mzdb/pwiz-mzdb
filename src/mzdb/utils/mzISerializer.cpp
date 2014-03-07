#include "mzISerializer.h"
#include "boost/algorithm/string.hpp"
#include "mzUtils.hpp"

namespace mzdb {

namespace ISerializer {

using namespace pugi;
using namespace pwiz::msdata;

void getCvParams(const ParamContainer& object, xml_node& doc) {
    const vector<CVParam>& params = object.cvParams;
    if (params.empty())
        return;

    auto node = doc.append_child(cv_params_str);

    for (unsigned int i = 0; i < params.size(); ++i) {
        string n = params[i].name();
        boost::replace_all(n, " ", "_");

        if (params[i].empty()) {
            continue;
        }

        auto n_ = node.append_child(cv_param_str);
        n_.append_attribute(cv_ref_str).set_value(ms_ns_str);
        n_.append_attribute(access_nb_str).set_value(params[i].cvid); //TODO concatener "MS:no Accession"
        n_.append_attribute(param_name_str).set_value(params[i].name().c_str());

        try {
            if (boost::find_first(params[i].value, "."))
                n_.append_attribute(param_value_str).set_value(params[i].valueAs<double>());
            else if (strcmp(params[i].value.c_str(), "") == 0 )
                n_.append_attribute(param_value_str).set_value("");
            else
                n_.append_attribute(param_value_str).set_value(params[i].valueAs<int>());

        } catch (boost::bad_lexical_cast &) {
            if (params[i].value ==  true_str || params[i].value == false_str )
                n_.append_attribute(param_value_str).set_value(params[i].valueAs<bool>());
            else
                n_.append_attribute(param_value_str).set_value(params[i].value.c_str());
        }
    }

}



void getUserParams(const ParamContainer& object, xml_node& doc) {
    const vector<UserParam>& params = object.userParams;
    if (params.empty())
        return;

    auto params_node = doc.append_child(user_params_str);
    for (unsigned int i = 0; i < params.size(); ++i) {
        string name = params[i].name;
        boost::replace_all(name, " ", "_");
        if (params[i].empty()) {
            continue;
        }
        if (params[i].name == "instrumentMethods" || params[i].name == "errorLog") {
            auto texts_node = doc.child(user_texts_str);

            if (texts_node.empty())
                texts_node = doc.append_child(user_texts_str);

            auto n = texts_node.append_child(user_text_str);
            n.append_attribute(cv_ref_str).set_value(ms_ns_str);
            n.append_attribute(access_nb_str).set_value(params[i].units); //TODO concatener "MS:no Accession"
            n.append_attribute(param_name_str).set_value(params[i].name.c_str());
            n.append_attribute(param_type_str).set_value(params[i].type.c_str());
            n.text() = params[i].value.c_str();
        } else {
            auto n = params_node.append_child(user_param_str);
            n.append_attribute(cv_ref_str).set_value(ms_ns_str);
            n.append_attribute(access_nb_str).set_value(params[i].units); //TODO concatener "MS:no Accession"
            n.append_attribute(param_name_str).set_value(params[i].name.c_str());
            n.append_attribute(param_type_str).set_value(params[i].type.c_str());
            try {
                if (boost::find_first(params[i].value, "."))
                    n.append_attribute(param_value_str).set_value(params[i].valueAs<double>());
                else if (strcmp(params[i].value.c_str(), "") == 0 )
                    n.append_attribute(param_value_str).set_value("");
                else
                    n.append_attribute(param_value_str).set_value(params[i].valueAs<int>());
            } catch (boost::bad_lexical_cast &) {
                if (params[i].value == true_str || params[i].value == false_str)
                    n.append_attribute(param_value_str).set_value(params[i].valueAs<bool>());
                else
                    n.append_attribute(param_value_str).set_value(params[i].value.c_str());
            }
        }
    }
}



string& serialize(const ParamContainer& e, xml_string_writer& writer) {
    xml_document doc;
    auto n = doc.append_child(params_str);
    getCvParams(e, n);
    getUserParams(e, n);
    doc.print(writer, "  ");//, "\t", format_raw | format_no_declaration); //no namespace no charriot char
    return writer.getResult();
}


}


}
