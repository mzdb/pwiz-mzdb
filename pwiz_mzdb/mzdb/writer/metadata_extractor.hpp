/*
 * Copyright 2014 CNRS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//author marc.dubois@ipbs.fr

#ifndef ABSTRACTMETADATAEXTRACTOR_H
#define ABSTRACTMETADATAEXTRACTOR_H


#include "pwiz/data/common/cv.hpp"
#include "pwiz_aux/msrc/utility/vendor_api/thermo/RawFile.h"
#include "pwiz_aux/msrc/utility/vendor_api/ABI/WiffFile.hpp"

#include "../utils/glog/logging.h"
#include "../utils/mzUtils.hpp"
#include "user_text.h"

namespace mzdb {


/// interface for extracting metadata, defining the minimum knowledge about
/// on sample
class mzIMetadataExtractor {

protected:
    std::string filepath;


public:

    /// Path of the raw file as parameter
    inline mzIMetadataExtractor(std::string& s): filepath(s) {}

    /// This function is Thermo-specific. But may return additional informations
    /// for others constructors
    virtual UserText getExtraDataAsUserText() = 0;

    virtual pwiz::msdata::SamplePtr getSample() = 0;

    /// Important function to determine resolution of a spectra. It will
    /// determine the encoding mode of its mzs, intensities
    virtual bool isInHighRes(pwiz::msdata::SpectrumPtr) = 0;

    /// Suppose to return the lowest mz observed during the run
    virtual inline double getLowMass() = 0;

    /// Suppose to return the highest mz observed during the run
    virtual inline double getHighMass() = 0;


};


/// Abstract class implementing the extraction interface, and calling
/// the derived class implementation (CRTP pattern)
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



/// Extractor used when provided raw file is a xml based file
/// A priori we do not have extra knowledge about the method etc...
class mzEmptyMetadataExtractor : public mzAbstractMetadataExtractor< mzEmptyMetadataExtractor, 0 > {

public:

    inline mzEmptyMetadataExtractor(std::string& f) : mzAbstractMetadataExtractor< mzEmptyMetadataExtractor, 0 >(f) {}
    virtual inline UserText getExtraDataAsUserText() {
        return UserText();
   }

    virtual inline pwiz::msdata::SamplePtr getSample() {
        return pwiz::msdata::SamplePtr(new pwiz::msdata::Sample());
   }

    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr p) {
        if (p->hasCVParam(pwiz::msdata::MS_MSn_spectrum))
            return false;
        return true;
   }

   //return default values ?
   virtual inline double getLowMass() {return 300.0;}

   // return default values ?
   virtual inline double getHighMass() {return 1500.0;}

};

/// ABI Sciex metadata extractor

#ifdef _WIN32
class mzABSciexMetadataExtractor : public mzAbstractMetadataExtractor< mzABSciexMetadataExtractor, (int) pwiz::cv::MS_ABI_WIFF_format > {

    pwiz::vendor_api::ABI::WiffFilePtr _wiffFilePtr;
    double _lowMz, _highMz;

    public:

    /// Ctor
    inline mzABSciexMetadataExtractor(std::string& f) : mzAbstractMetadataExtractor<mzABSciexMetadataExtractor,
                                                        (int) pwiz::cv::MS_ABI_WIFF_format >(f) {
        try {
        _wiffFilePtr = pwiz::vendor_api::ABI::WiffFile::create(f);
        } catch (...) {

        }
    }

    /// Data fetch from api constructor, could methods details etc...
    virtual inline UserText getExtraDataAsUserText() {
        return UserText();
   }

    /**/
    virtual inline pwiz::msdata::SamplePtr getSample() {
        return pwiz::msdata::SamplePtr(new pwiz::msdata::Sample());
   }

    /// Always return true, WARNING
    /// It is true for triple TOF but not for the others
    ///  TODO: check ?
    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr p) {
        return true;
   }

    /// Note: the following define start and end mzs of swath window
    /// so this is not always relevant
   /* return default values ?*/
   virtual inline double getLowMass() {
        if (! _wiffFilePtr) {
            LOG(WARNING) << "Wiff file shared pointer seems to be null";
            return 0.0;
        }
        //TODO: getting the first spectrum to gather lowest, highest Mass
        // acquisition
        if (! _lowMz) {
            auto& experiment = _wiffFilePtr->getExperiment(1, 1, 1);
            experiment->getAcquisitionMassRange(_lowMz, _highMz);
        }
        return _lowMz;
    }

   /* return default values ?*/
   virtual inline double getHighMass() {
        if (! _wiffFilePtr) {
            LOG(WARNING) << "Wiff file shared pointer seems to be null";
            return 0.0;
        }
        //TODO: getting the first spectrum to gather lowest, highest Mass
        // acquisition
        if (! _highMz) {
            auto& experiment = _wiffFilePtr->getExperiment(1, 1, 1);
            experiment->getAcquisitionMassRange(_lowMz, _highMz);
        }
        return _highMz;
    }

};

#endif

/**
 * @brief The mzThermoMetadataExtractor class
 */
#ifdef _WIN32
class mzThermoMetadataExtractor : public mzAbstractMetadataExtractor< mzThermoMetadataExtractor, (int) pwiz::cv::MS_Thermo_RAW_format> {
    pwiz::vendor_api::Thermo::RawFilePtr _rawfilePtr;

    public:
    inline mzThermoMetadataExtractor(std::string& f) : mzAbstractMetadataExtractor< mzThermoMetadataExtractor, (int) pwiz::cv::MS_Thermo_RAW_format>(f) {
        try {
            _rawfilePtr = pwiz::vendor_api::Thermo::RawFile::create(f);
         } catch (...) {

        }
    }


     virtual UserText getExtraDataAsUserText() {
        std::auto_ptr<pwiz::vendor_api::Thermo::LabelValueArray> l = _rawfilePtr->getInstrumentMethods();
        std::string instrumentMethods;

        for (int i=0; i < l->size(); ++i) {
            std::string v = l->value(i) + "\n";
            instrumentMethods += v;
        }
        return UserText("instrumentMethods", instrumentMethods, XML_STRING);

    }

     virtual pwiz::msdata::SamplePtr getSample() {

        std::string filename = "", filenameId = "";
        try {
            filename = _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowRawFileName);
        } catch (exception& ) {
        }

        try {
            filenameId =  _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowSampleID);
        } catch (exception& ) {
        }


        pwiz::msdata::SamplePtr sample(new pwiz::msdata::Sample(filename, filenameId));
        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowComment),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowComment),
                                                                 XML_STRING));
        } catch (exception&) {}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowDataPath),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowDataPath),
                                                                 XML_STRING));
        } catch (exception&) {}

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::CreatorID),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::CreatorID),
                                                                 XML_STRING));
        } catch (exception&) { }

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowCalibrationFile),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowCalibrationFile),
                                                                 XML_STRING));
        } catch (exception&) { }

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::InstSoftwareVersion),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::InstSoftwareVersion),
                                                                 XML_STRING));
        } catch (exception&) { }

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowInstrumentMethod),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowInstrumentMethod),
                                                                 XML_STRING));
        } catch (exception&) { }

        try {
            sample->userParams.push_back(pwiz::msdata::UserParam(_rawfilePtr->name(pwiz::vendor_api::Thermo::SeqRowProcessingMethod),
                                                                 _rawfilePtr->value(pwiz::vendor_api::Thermo::SeqRowProcessingMethod),
                                                                 XML_STRING));
        } catch (exception&) { }

        return sample;
    }

    virtual inline bool isInHighRes(pwiz::msdata::SpectrumPtr p) {
        const std::string& scanparam = p->scanList.scans[0].cvParam(pwiz::msdata::MS_filter_string).value;
        bool highRes = (scanparam.find("FTMS") != std::string::npos) ? true : false;
        return highRes;
   }

    /* return default values ?*/
    virtual inline double getLowMass() {
        if (this->_rawfilePtr)
            return this->_rawfilePtr->value(pwiz::vendor_api::Thermo::LowMass);
        return 0.0;
     }

    /* return default values ?*/
    virtual inline double getHighMass() {
        if (this->_rawfilePtr)
            return this->_rawfilePtr->value(pwiz::vendor_api::Thermo::HighMass);
        return 0.0;
     }
};

#endif

}


#endif // ABSTRACTMETADATAEXTRACTOR_H
