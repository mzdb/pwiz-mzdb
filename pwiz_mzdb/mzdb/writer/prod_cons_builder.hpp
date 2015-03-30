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
    DIAThermoProducer m_diaThermoProducer;
    DDAThermoProducer m_ddaThermoProducer;
    SwathABIProducer m_swathABIProducer;
    DDAABIProducer m_ddaABIProducer;
    SwathGenericProducer m_swathGenericProducer;
    DDAGenericProducer m_ddaGenericProducer;

    // Consumers
    DIAThermoConsumer m_diaThermoConsumer;
    DDAThermoConsumer m_ddaThermoConsumer;
    SwathABIConsumer m_swathABIConsumer;
    DDAABIConsumer m_ddaABIConsumer;
    SwathGenericConsumer m_swathGenericConsumer;
    DDAGenericConsumer m_ddaGenericConsumer;

public:
    PCBuilder(typename QueueType::QueueType& queue,
                   MzDBFile& mzdbFile,
                   mzParamsCollecter& paramsCollecter,
                   pwiz::msdata::CVID rawFileFormat,
                   map<int, DataMode>& dataModeByMsLevel):

    m_diaThermoProducer(queue, mzdbFile.name, dataModeByMsLevel),
    m_ddaThermoProducer(queue, mzdbFile.name, dataModeByMsLevel),
    m_swathABIProducer(queue, mzdbFile.name, dataModeByMsLevel),
    m_ddaABIProducer(queue, mzdbFile.name, dataModeByMsLevel),
    m_swathGenericProducer(queue, mzdbFile.name, dataModeByMsLevel),
    m_ddaGenericProducer(queue, mzdbFile.name, dataModeByMsLevel),

    m_diaThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
    m_ddaThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
    m_swathABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
    m_ddaABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
    m_swathGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel),
    m_ddaGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel)
    {

    }

    inline boost::thread getDIAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                                            SpectrumListThermo* spectrumList,
                                                                            int nscans,
                                                                            pwiz::msdata::CVID filetype,
                                                                            mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_diaThermoProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                                                        nscans, filetype, params);
    }

    inline  boost::thread getDDAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                                                 SpectrumListThermo* spectrumList,
                                                                                 int nscans,
                                                                                 map<int, double>& bbWidthManager,
                                                                                 pwiz::msdata::CVID filetype,
                                                                                 mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_ddaThermoProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                                                        nscans, bbWidthManager, filetype,
                                                                                        params);
    }

    inline  boost::thread getDDAABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                                          SpectrumListABI* spectrumList,
                                                                          int nscans,
                                                                          map<int, double>& bbWidthManager,
                                                                          pwiz::msdata::CVID filetype,
                                                                          mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_ddaABIProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                        nscans, bbWidthManager, filetype,
                                                        params);
    }

    inline  boost::thread getSwathABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI* spectrumList,
                                                    int nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_swathABIProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                          nscans, filetype, params);
    }

    inline  boost::thread getSwathGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                        SpectrumList* spectrumList,
                                                        int nscans,
                                                        pwiz::msdata::CVID filetype,
                                                        mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_swathGenericProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                              nscans, filetype, params);
    }

    inline  boost::thread getDDAGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                                                 SpectrumList* spectrumList,
                                                                                 int nscans,
                                                                                 map<int, double>& bbWidthManager,
                                                                                 pwiz::msdata::CVID filetype,
                                                                                 mzPeakFinderUtils::PeakPickerParams& params) {
        return this->m_ddaGenericProducer.getProducerThread(levelsToCentroid, spectrumList,
                                                            nscans, bbWidthManager, filetype,
                                                            params);
    }

    //consumers
    inline  boost::thread getDIAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->m_diaThermoConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                                                            runSlices, progressionCount, nscans);
    }

    inline  boost::thread getDDAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->m_ddaThermoConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }

    inline  boost::thread getDDAABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                  ISerializer::xml_string_writer& serializer,
                                                  map<int, double>& bbMzWidthByMsLevel,
                                                  map<int, map<int, int> >& runSlices,
                                                  int& progressionCount,
                                                  int nscans ) {
        return this->m_ddaABIConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                        runSlices, progressionCount, nscans);
    }

    inline  boost::thread getSwathABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                    ISerializer::xml_string_writer& serializer,
                                                    map<int, double>& bbMzWidthByMsLevel,
                                                    map<int, map<int, int> >& runSlices,
                                                    int& progressionCount,
                                                    int nscans ) {
        return this->m_swathABIConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                          runSlices, progressionCount, nscans);
    }

    inline  boost::thread getSwathGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                        ISerializer::xml_string_writer& serializer,
                                                        map<int, double>& bbMzWidthByMsLevel,
                                                        map<int, map<int, int> >& runSlices,
                                                        int& progressionCount,
                                                        int nscans ) {
        return this->m_swathGenericConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                              runSlices, progressionCount, nscans);
    }

    inline  boost::thread getDDAGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->m_ddaGenericConsumer.getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                            runSlices, progressionCount, nscans);
    }

};



}

#endif // PCBUILDER_HPP
