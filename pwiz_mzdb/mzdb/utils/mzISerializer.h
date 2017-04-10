/**
  * @file mzISerializer.h
  * @brief Static functions to handle serialization of cvParam to xml chunk to be stored in _param_tree_ column
  * @author Marc Dubois marc.dubois@ipbs.fr
  */

#ifndef MZSERIALIZERINTERFACE_H
#define MZSERIALIZERINTERFACE_H

#include "pwiz/data/msdata/MSData.hpp"
#include "../lib/pugixml/include/pugixml.hpp"
#include "iostream"


using namespace std;

namespace mzdb {

/**
 * Static functions to handle serialization of cvParam to xml chunk
 * to be stored in _param_tree_ column
 */
namespace ISerializer {


/**
 * The xml_string_writer struct
 * =============================
 *
 * write serialization result into a std::string object
 */
struct xml_string_writer : pugi::xml_writer {
    string result;

    virtual void write(const void* data, size_t size) {
        result = string(static_cast<const char*> (data), size);
    }

    string getResult() {return result;}
};


/**
 * serialize cvParam into xml_node object
 *
 * @param doc XML document object pugi::xml object
 */
void getCvParams(const pwiz::msdata::ParamContainer&, pugi::xml_node& doc);

/**
 * serialize userParam into xml_node object
 *
 * @param doc XML document object pugi::xml object
 */
void getUserParams(const pwiz::msdata::ParamContainer&, pugi::xml_node& doc);


/**
 * wrapper: calls getCvParams and getUserParams
 *
 * @return string result of the serialization
 */
string serialize(const pwiz::msdata::ParamContainer&, xml_string_writer &);

/**
 * wrapper: calls getCvParams and getUserParams
 *
 * @return string result of the serialization
 */
string serialize(const pwiz::msdata::ParamContainer&);

}//end serialization

}//end mzdb


#endif // MZSERIALIZERINTERFACE_H
