#ifndef ABSTRACTMETADATAEXTRACTOR_H
#define ABSTRACTMETADATAEXTRACTOR_H


#include "pwiz/data/common/cv.hpp"
#include "../msdata/mzUserText.h"
#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h"
#include "../lib/glog/glog/logging.h"

namespace mzdb {

/**
 *interface for extracting metadata
*/
class mzIMetadataExtractor {
protected:
    std::string filepath;

public:
    inline mzIMetadataExtractor(std::string& s): filepath(s) {

    }

    virtual UserText getExtraDataAsUserText() = 0;
    virtual pwiz::msdata::SamplePtr getSample() = 0;
    virtual bool isInHighRes(pwiz::msdata::SpectrumPtr) = 0;
    virtual inline double getLowMass() = 0;
    virtual inline double getHighMass() = 0;


};

/**
 *  Abstract class implementing the extraction interface, and calling
 *  the derivde class implementation
 */
template<class Derived, int>
class mzAbstractMetadataExtractor : public mzIMetadataExtractor {

public:
    inline mzAbstractMetadataExtractor(std::string& s): mzIMetadataExtractor(s) {}

     virtual inline UserText getExtraDataAsUserText() {
        return static_cast<Derived*>(this)->getExtraDataAsUserText();
    }

     virtual inline pwiz::msdata::SamplePtr getSample() {
        return static_cast<Derived*>(this)->getSample();
    }

    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr s) {
        return static_cast<Derived*>(this)->isInHighRes(s);
    }

    virtual inline double getLowMass() {
        return static_cast<Derived*>(this)->getLowMass();
    }

    virtual inline double getHighMass() {
        return static_cast<Derived*>(this)->getHighMass();
    }

};


/**
 * @brief The mzEmptyMetadataExtractor class
 */
class mzEmptyMetadataExtractor : public mzAbstractMetadataExtractor< mzEmptyMetadataExtractor, 0 > {
public:
    inline mzEmptyMetadataExtractor(std::string& f) : mzAbstractMetadataExtractor< mzEmptyMetadataExtractor, 0 >(f) {}
    virtual inline UserText getExtraDataAsUserText() {
        return UserText();
   }

    /**/
    virtual inline pwiz::msdata::SamplePtr getSample() {
        return pwiz::msdata::SamplePtr(new pwiz::msdata::Sample());
   }

    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr p) {
        if (p->hasCVParam(pwiz::msdata::MS_MSn_spectrum))
            return false;
        return true;
   }

   /* return default values ?*/
   virtual inline double getLowMass() {return 300.0;}

   /* return default values ?*/
   virtual inline double getHighMass() {return 1500.0;}

};


/**
 * @brief The mzThermoMetadataExtractor class
 */
class mzThermoMetadataExtractor : public mzAbstractMetadataExtractor< mzThermoMetadataExtractor, (int) pwiz::cv::MS_Thermo_RAW_format> {
    pwiz::vendor_api::Thermo::RawFilePtr _rawfilePtr;

    public:
    inline mzThermoMetadataExtractor(std::string& f) : mzAbstractMetadataExtractor< mzThermoMetadataExtractor, (int) pwiz::cv::MS_Thermo_RAW_format>(f),
                                                  _rawfilePtr(pwiz::vendor_api::Thermo::RawFile::create(f)){

    }


     virtual UserText getExtraDataAsUserText() {
        std::auto_ptr<pwiz::vendor_api::Thermo::LabelValueArray> l = _rawfilePtr->getInstrumentMethods();
        std::string instrumentMethods;

        for (int i=0; i < l->size(); ++i) {
            std::string v = l->value(i) + "\n";
            instrumentMethods += v;
        }
        return UserText("instrumentMethods", instrumentMethods, "xsd:string");

    }

     virtual pwiz::msdata::SamplePtr getSample() {

        std::string filename = "", filenameId = "";
        try {
            filename = _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowRawFileName);
        } catch (exception& e) {
            printf("%s\n", e.what());
        }

        try {
            filenameId =  _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowSampleID);
        } catch (exception& e) {
            printf("%s\n", e.what());
        }


        pwiz::msdata::SamplePtr sample(new pwiz::msdata::Sample(filename, filenameId));
        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowComment),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowComment),
                                                                 "xsd:string"));
        } catch (exception&) {}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowDataPath),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowDataPath),
                                                                 "xsd:string"));
        } catch (exception&) {}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::CreatorID),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::CreatorID),
                                                                 "xsd:string"));
        } catch (exception&) { LOG(INFO) << "creator id user param failed";}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowCalibrationFile),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowCalibrationFile),
                                                                 "xsd:string"));
        } catch (exception&) {LOG(INFO) << "seqrowcalibrationfile param failed";}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::InstSoftwareVersion),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::InstSoftwareVersion),
                                                                 "xsd:string"));
        } catch (exception&) {LOG(INFO) << "instsoftware version param failed";}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowInstrumentMethod),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowInstrumentMethod),
                                                                 "xsd:string"));
        } catch (exception&) {LOG(INFO) << "seqrowintrumentmethod version param failed";}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowProcessingMethod),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowProcessingMethod),
                                                                 "xsd:string"));
        } catch (exception&) {LOG(INFO) << "seqrowprocessingmethod version param failed";}

        return sample;
    }

    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr p) {
        const std::string& scanparam = p->scanList.scans[0].cvParam(pwiz::msdata::MS_filter_string).value;
        bool highRes = (scanparam.find("FTMS") != std::string::npos) ? true : false;
        return highRes;
   }

    /* return default values ?*/
    virtual inline double getLowMass() {return this->_rawfilePtr->value(pwiz::vendor_api::Thermo::LowMass);}

    /* return default values ?*/
    virtual inline double getHighMass() {return this->_rawfilePtr->value(pwiz::vendor_api::Thermo::HighMass);}
};

}


#endif // ABSTRACTMETADATAEXTRACTOR_H
