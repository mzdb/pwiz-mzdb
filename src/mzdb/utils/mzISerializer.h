#ifndef MZSERIALIZERINTERFACE_H
#define MZSERIALIZERINTERFACE_H

#include "pwiz/data/msdata/MSData.hpp"
#include "../lib/pugixml/include/pugixml.hpp"
#include "iostream"

namespace mzdb {

using namespace std;

namespace ISerializer {


struct xml_string_writer : pugi::xml_writer {
    string result;

    virtual void write(const void* data, size_t size) {
        result = string(static_cast<const char*> (data), size);
    }

    string& getResult() {return result;}
};


void getCvParams(const pwiz::msdata::ParamContainer&, pugi::xml_node& doc);

void getUserParams(const pwiz::msdata::ParamContainer&, pugi::xml_node& doc);

string& serialize(const pwiz::msdata::ParamContainer&, xml_string_writer&);

}//end serialization

}//end mzdb


#endif // MZSERIALIZERINTERFACE_H
