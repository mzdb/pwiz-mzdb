#ifndef PCBUILDER_HPP
#define PCBUILDER_HPP

#include "pwiz/data/vendor_readers/Thermo/SpectrumList_Thermo.hpp"
#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"

#include "dda_consumer.h"
#include "dda_producer.h"
#include "swath_consumer.hpp"
#include "swath_producer.hpp"
#include "metadata_extractor.hpp"


namespace mzdb {

template<class QueueType,
         class PeakPicker>
class PCBuilder  {
protected:

    typedef pwiz::msdata::detail::SpectrumList_Thermo SpectrumListThermo;
    typedef pwiz::msdata::detail::SpectrumList_ABI SpectrumListABI;
    typedef pwiz::msdata::SpectrumList SpectrumList;


    // producer
    typedef  mzSwathProducer<QueueType,
    mzThermoMetadataExtractor,
    PeakPicker,
    SpectrumListThermo> DIAThermoProducer;

    typedef  mzDDAProducer< QueueType,
    mzThermoMetadataExtractor,
    PeakPicker,
    SpectrumListThermo> DDAThermoProducer;

    typedef  mzSwathProducer<QueueType,
    mzABSciexMetadataExtractor,
    PeakPicker,
    SpectrumListABI> SwathABIProducer;

    typedef  mzDDAProducer<QueueType,
    mzABSciexMetadataExtractor,
    PeakPicker,
    SpectrumListABI> DDAABIProducer;

    typedef mzSwathProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumList> SwathGenericProducer;

    typedef mzDDAProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumList> DDAGenericProducer;

    //Consumers
    typedef  mzSwathConsumer<QueueType,
    SpectrumListThermo> DIAThermoConsumer;

    typedef  mzDDAConsumer<QueueType,
    SpectrumListThermo> DDAThermoConsumer;

    typedef  mzSwathConsumer<QueueType,
    SpectrumListABI> SwathABIConsumer;

    typedef  mzDDAConsumer<QueueType,
    SpectrumListABI> DDAABIConsumer;

    typedef  mzSwathConsumer<QueueType,
    SpectrumList> SwathGenericConsumer;

    typedef  mzDDAConsumer<QueueType,
    SpectrumList> DDAGenericConsumer;


    // Producers
    unique_ptr<DIAThermoProducer> mDiaThermoProducer;
    unique_ptr<DDAThermoProducer> mDdaThermoProducer;
    unique_ptr<SwathABIProducer> mSwathABIProducer;
    unique_ptr<DDAABIProducer> mDdaABIProducer;
    unique_ptr<SwathGenericProducer> mSwathGenericProducer;
    unique_ptr<DDAGenericProducer> mDdaGenericProducer;

    // Consumers
    unique_ptr<DIAThermoConsumer> mDiaThermoConsumer;
    unique_ptr<DDAThermoConsumer> mDdaThermoConsumer;
    unique_ptr<SwathABIConsumer> mSwathABIConsumer;
    unique_ptr<DDAABIConsumer> mDdaABIConsumer;
    unique_ptr<SwathGenericConsumer> mSwathGenericConsumer;
    unique_ptr<DDAGenericConsumer> mDdaGenericConsumer;

public:
    PCBuilder(typename QueueType::QueueType& queue,
              MzDBFile& mzdbFile,
              mzParamsCollecter& paramsCollecter,
              pwiz::msdata::CVID rawFileFormat,
              map<int, DataMode>& dataModeByMsLevel):
        mDiaThermoProducer(nullptr),
        mDdaThermoProducer(nullptr),
        mSwathABIProducer(nullptr),
        mDdaABIProducer(nullptr),
        mSwathGenericProducer(nullptr),
        mDdaGenericProducer(nullptr),
        mDiaThermoConsumer(nullptr),
        mDdaThermoConsumer(nullptr),
        mSwathABIConsumer(nullptr),
        mDdaABIConsumer(nullptr),
        mSwathGenericConsumer(nullptr),
        mDdaGenericConsumer(nullptr)
    {
        if (rawFileFormat == pwiz::msdata::MS_Thermo_RAW_format) {
            mDiaThermoProducer = unique_ptr<DIAThermoProducer>(new DIAThermoProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaThermoProducer = unique_ptr<DDAThermoProducer>(new DDAThermoProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDiaThermoConsumer = unique_ptr<DIAThermoConsumer>(new DIAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));
            mDdaThermoConsumer = unique_ptr<DDAThermoConsumer>(new DDAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));

        } else if (rawFileFormat == pwiz::msdata::MS_ABI_WIFF_format) {
            mSwathABIProducer = unique_ptr<SwathABIProducer>(new SwathABIProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaABIProducer = unique_ptr<DDAABIProducer>(new DDAABIProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mSwathABIConsumer = unique_ptr<SwathABIConsumer>(new SwathABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));
            mDdaABIConsumer = unique_ptr<DDAABIConsumer>(new DDAABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));
        } else {
            mSwathGenericProducer = unique_ptr<SwathGenericProducer>(new SwathGenericProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaGenericProducer = unique_ptr<DDAGenericProducer>(new DDAGenericProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mSwathGenericConsumer = unique_ptr<SwathGenericConsumer>(new SwathGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));
            mDdaGenericConsumer = unique_ptr<DDAGenericConsumer>(new DDAGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel));
        }

    }

    /** DIA thermo producer */
    inline boost::thread getDIAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListThermo* spectrumList,
                                                    int nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaThermoProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, filetype, params);
    }

    /** DDA thermo producer  */
    inline  boost::thread getDDAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListThermo* spectrumList,
                                                     int nscans,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaThermoProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, bbWidthManager, filetype,
                                                           params);
    }

    /** DDA ABI producer */
    inline  boost::thread getDDAABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                  SpectrumListABI* spectrumList,
                                                  int nscans,
                                                  map<int, double>& bbWidthManager,
                                                  pwiz::msdata::CVID filetype,
                                                  mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                        nscans, bbWidthManager, filetype,
                                                        params);
    }

    /** */
    inline  boost::thread getSwathABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI* spectrumList,
                                                    int nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                          nscans, filetype, params);
    }

    /** */
    inline  boost::thread getSwathGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                        SpectrumList* spectrumList,
                                                        int nscans,
                                                        pwiz::msdata::CVID filetype,
                                                        mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathGenericProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                              nscans, filetype, params);
    }

    /** */
    inline  boost::thread getDDAGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                      SpectrumList* spectrumList,
                                                      int nscans,
                                                      map<int, double>& bbWidthManager,
                                                      pwiz::msdata::CVID filetype,
                                                      mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaGenericProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                            nscans, bbWidthManager, filetype,
                                                            params);
    }

    //consumers
    /** */
    inline  boost::thread getDIAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDiaThermoConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }

    /** */
    inline  boost::thread getDDAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDdaThermoConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }

    /** */
    inline  boost::thread getDDAABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                  ISerializer::xml_string_writer& serializer,
                                                  map<int, double>& bbMzWidthByMsLevel,
                                                  map<int, map<int, int> >& runSlices,
                                                  int& progressionCount,
                                                  int nscans ) {
        return this->mDdaABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                        runSlices, progressionCount, nscans);
    }

    /** */
    inline  boost::thread getSwathABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                    ISerializer::xml_string_writer& serializer,
                                                    map<int, double>& bbMzWidthByMsLevel,
                                                    map<int, map<int, int> >& runSlices,
                                                    int& progressionCount,
                                                    int nscans ) {
        return this->mSwathABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                          runSlices, progressionCount, nscans);
    }

    /** */
    inline  boost::thread getSwathGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                        ISerializer::xml_string_writer& serializer,
                                                        map<int, double>& bbMzWidthByMsLevel,
                                                        map<int, map<int, int> >& runSlices,
                                                        int& progressionCount,
                                                        int nscans ) {
        return this->mSwathGenericConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                              runSlices, progressionCount, nscans);
    }

    /** */
    inline  boost::thread getDDAGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                      ISerializer::xml_string_writer& serializer,
                                                      map<int, double>& bbMzWidthByMsLevel,
                                                      map<int, map<int, int> >& runSlices,
                                                      int& progressionCount,
                                                      int nscans ) {
        return this->mDdaGenericConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                            runSlices, progressionCount, nscans);
    }

};



}

#endif // PCBUILDER_HPP
