#ifndef PCBUILDER_HPP
#define PCBUILDER_HPP

#ifdef _WIN32
#include "pwiz/data/vendor_readers/Thermo/SpectrumList_Thermo.hpp"
#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"
#include "pwiz/data/vendor_readers/Bruker/SpectrumList_Bruker.hpp"
#include "pwiz/data/vendor_readers/Agilent/SpectrumList_Agilent.hpp"
#include "pwiz/data/vendor_readers/ABI/T2D/SpectrumList_ABI_T2D.hpp"
#endif

#include "dda_consumer.h"
#include "dda_producer.h"
#include "swath_consumer.hpp"
#include "swath_producer.hpp"
#include "metadata_extractor.hpp"


namespace mzdb {

/**
 * PCBuilder class
 * =================
 *
 * In charge of building adapted `producers` and `consumers` in order to handle conversion.
 */
template<class QueueType,
         class PeakPicker>
class PCBuilder  {
protected:

    ///typedef for main constructors spectrumList
    typedef pwiz::msdata::detail::SpectrumList_Thermo SpectrumListThermo;
    typedef pwiz::msdata::detail::SpectrumList_ABI SpectrumListABI;
    typedef pwiz::msdata::detail::SpectrumList_Bruker SpectrumListBruker;
    typedef pwiz::msdata::detail::SpectrumList_Agilent SpectrumListAgilent;
    typedef pwiz::msdata::detail::SpectrumList_ABI_T2D SpectrumListABI_T2D;

    ///default spectrumList (mzML...)
    typedef pwiz::msdata::SpectrumList SpectrumList;


    // producers definitions

    ///thermo DIA
    typedef  mzSwathProducer<QueueType,
    mzThermoMetadataExtractor,
    PeakPicker,
    SpectrumListThermo> DIAThermoProducer;

    ///thermo DDA
    typedef  mzDDAProducer< QueueType,
    mzThermoMetadataExtractor,
    PeakPicker,
    SpectrumListThermo> DDAThermoProducer;

    ///bruker DIA
    typedef mzSwathProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListBruker> DIABrukerProducer;

    ///bruker DDA
    typedef mzDDAProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListBruker> DDABrukerProducer;

    ///ABI swath
    typedef  mzSwathProducer<QueueType,
    mzABSciexMetadataExtractor,
    PeakPicker,
    SpectrumListABI> SwathABIProducer;

    ///ABI DDA
    typedef  mzDDAProducer<QueueType,
    mzABSciexMetadataExtractor,
    PeakPicker,
    SpectrumListABI> DDAABIProducer;

    ///agilent DIA
    typedef mzSwathProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListAgilent> DIAAgilentProducer;

    ///agilent DDA
    typedef mzDDAProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListAgilent> DDAAgilentProducer;

    ///ABI_T2D DIA
    typedef mzSwathProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListABI_T2D> DIAABI_T2DProducer;

    ///ABI_T2D DDA
    typedef mzDDAProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumListABI_T2D> DDAABI_T2DProducer;

    ///generic swath producer
    typedef mzSwathProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumList> SwathGenericProducer;

    ///generic dda producer
    typedef mzDDAProducer<QueueType,
    mzEmptyMetadataExtractor,
    PeakPicker,
    SpectrumList> DDAGenericProducer;

    //Consumers definiton

    ///thermo DIA
    typedef mzSwathConsumer<QueueType,
    SpectrumListThermo> DIAThermoConsumer;

    ///thermo DDA
    typedef mzDDAConsumer<QueueType,
    SpectrumListThermo> DDAThermoConsumer;

    ///Bruker DIA
    typedef mzSwathConsumer<QueueType,
    SpectrumListBruker> DIABrukerConsumer;

    ///bruker DDA
    typedef mzDDAConsumer<QueueType,
    SpectrumListBruker> DDABrukerConsumer;

    /// abi swath
    typedef mzSwathConsumer<QueueType,
    SpectrumListABI> SwathABIConsumer;

    ///abi swath
    typedef mzDDAConsumer<QueueType,
    SpectrumListABI> DDAABIConsumer;

    ///agilent DIA
    typedef mzSwathConsumer<QueueType,
    SpectrumListAgilent> DIAAgilentConsumer;

    ///agilent DDA
    typedef mzDDAConsumer<QueueType,
    SpectrumListAgilent> DDAAgilentConsumer;

    ///ABI_T2D DIA
    typedef mzSwathConsumer<QueueType,
    SpectrumListABI_T2D> DIAABI_T2DConsumer;

    ///ABI_T2D DDA
    typedef mzDDAConsumer<QueueType,
    SpectrumListABI_T2D> DDAABI_T2DConsumer;

    /// generic swath
    typedef  mzSwathConsumer<QueueType,
    SpectrumList> SwathGenericConsumer;

    /// generic dda
    typedef  mzDDAConsumer<QueueType,
    SpectrumList> DDAGenericConsumer;


    // Producers
    unique_ptr<DIAThermoProducer> mDiaThermoProducer;
    unique_ptr<DDAThermoProducer> mDdaThermoProducer;

    unique_ptr<DIABrukerProducer> mDiaBrukerProducer;
    unique_ptr<DDABrukerProducer> mDdaBrukerProducer;

    unique_ptr<SwathABIProducer> mSwathABIProducer;
    unique_ptr<DDAABIProducer> mDdaABIProducer;

    unique_ptr<DIAAgilentProducer> mDiaAgilentProducer;
    unique_ptr<DDAAgilentProducer> mDdaAgilentProducer;

    unique_ptr<DIAABI_T2DProducer> mDiaABI_T2DProducer;
    unique_ptr<DDAABI_T2DProducer> mDdaABI_T2DProducer;

    unique_ptr<SwathGenericProducer> mSwathGenericProducer;
    unique_ptr<DDAGenericProducer> mDdaGenericProducer;

    // Consumers
    unique_ptr<DIAThermoConsumer> mDiaThermoConsumer;
    unique_ptr<DDAThermoConsumer> mDdaThermoConsumer;

    unique_ptr<DIABrukerConsumer> mDiaBrukerConsumer;
    unique_ptr<DDABrukerConsumer> mDdaBrukerConsumer;

    unique_ptr<SwathABIConsumer> mSwathABIConsumer;
    unique_ptr<DDAABIConsumer> mDdaABIConsumer;

    unique_ptr<DIAAgilentConsumer> mDiaAgilentConsumer;
    unique_ptr<DDAAgilentConsumer> mDdaAgilentConsumer;

    unique_ptr<DIAABI_T2DConsumer> mDiaABI_T2DConsumer;
    unique_ptr<DDAABI_T2DConsumer> mDdaABI_T2DConsumer;

    unique_ptr<SwathGenericConsumer> mSwathGenericConsumer;
    unique_ptr<DDAGenericConsumer> mDdaGenericConsumer;

public:

    PCBuilder(typename QueueType::QueueType& queue,
              MzDBFile& mzdbFile,
              mzParamsCollecter& paramsCollecter,
              pwiz::msdata::CVID rawFileFormat, //will be deprecated
              int mode,
              map<int, DataMode>& dataModeByMsLevel,
              map<int, DataEncoding>& dataEncodingByID):
        //producers
        mDiaThermoProducer(nullptr),
        mDdaThermoProducer(nullptr),

        mDiaBrukerProducer(nullptr),
        mDdaBrukerProducer(nullptr),

        mSwathABIProducer(nullptr),
        mDdaABIProducer(nullptr),

        mDiaAgilentProducer(nullptr),
        mDdaAgilentProducer(nullptr),

        mDiaABI_T2DProducer(nullptr),
        mDdaABI_T2DProducer(nullptr),

        mSwathGenericProducer(nullptr),
        mDdaGenericProducer(nullptr),

        //consumers
        mDiaThermoConsumer(nullptr),
        mDdaThermoConsumer(nullptr),

        mDiaBrukerConsumer(nullptr),
        mDdaBrukerConsumer(nullptr),

        mSwathABIConsumer(nullptr),
        mDdaABIConsumer(nullptr),

        mDiaAgilentConsumer(nullptr),
        mDdaAgilentConsumer(nullptr),

        mDiaABI_T2DConsumer(nullptr),
        mDdaABI_T2DConsumer(nullptr),

        mSwathGenericConsumer(nullptr),
        mDdaGenericConsumer(nullptr)
    {

        if (mode == 1) {
            mDiaThermoProducer = unique_ptr<DIAThermoProducer>(new DIAThermoProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaThermoProducer = unique_ptr<DDAThermoProducer>(new DDAThermoProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDiaThermoConsumer = unique_ptr<DIAThermoConsumer>(new DIAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaThermoConsumer = unique_ptr<DDAThermoConsumer>(new DDAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        } else if (mode == 2) {
            mDiaBrukerProducer = unique_ptr<DIABrukerProducer>(new DIABrukerProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaBrukerProducer = unique_ptr<DDABrukerProducer>(new DDABrukerProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDiaBrukerConsumer = unique_ptr<DIABrukerConsumer>(new DIABrukerConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaBrukerConsumer = unique_ptr<DDABrukerConsumer>(new DDABrukerConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        } else if (mode == 3) {
            mSwathABIProducer = unique_ptr<SwathABIProducer>(new SwathABIProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaABIProducer = unique_ptr<DDAABIProducer>(new DDAABIProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mSwathABIConsumer = unique_ptr<SwathABIConsumer>(new SwathABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaABIConsumer = unique_ptr<DDAABIConsumer>(new DDAABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        } else if (mode == 4) {
            mDiaAgilentProducer = unique_ptr<DIAAgilentProducer>(new DIAAgilentProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaAgilentProducer = unique_ptr<DDAAgilentProducer>(new DDAAgilentProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDiaAgilentConsumer = unique_ptr<DIAAgilentConsumer>(new DIAAgilentConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaAgilentConsumer = unique_ptr<DDAAgilentConsumer>(new DDAAgilentConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        } else if (mode == 5) {
            mDiaABI_T2DProducer = unique_ptr<DIAABI_T2DProducer>(new DIAABI_T2DProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaABI_T2DProducer = unique_ptr<DDAABI_T2DProducer>(new DDAABI_T2DProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDiaABI_T2DConsumer = unique_ptr<DIAABI_T2DConsumer>(new DIAABI_T2DConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaABI_T2DConsumer = unique_ptr<DDAABI_T2DConsumer>(new DDAABI_T2DConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        } else {
            mSwathGenericProducer = unique_ptr<SwathGenericProducer>(new SwathGenericProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mDdaGenericProducer = unique_ptr<DDAGenericProducer>(new DDAGenericProducer(queue, mzdbFile.name, dataModeByMsLevel));
            mSwathGenericConsumer = unique_ptr<SwathGenericConsumer>(new SwathGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
            mDdaGenericConsumer = unique_ptr<DDAGenericConsumer>(new DDAGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataModeByMsLevel, dataEncodingByID));
        }

    }

    /** DIA thermo producer */
    inline boost::thread getDIAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListThermo* spectrumList,
                                                    pair<int, int>& nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaThermoProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, filetype, params);
    }

    /** DDA thermo producer  */
    inline  boost::thread getDDAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListThermo* spectrumList,
                                                     pair<int, int>& nscans,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaThermoProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, bbWidthManager, filetype,
                                                           params);
    }


    /** bruker producer */
    inline boost::thread getDIABrukerProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListBruker* spectrumList,
                                                    pair<int, int>& nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaBrukerProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, filetype, params);
    }

    /** bruker producer  */
    inline  boost::thread getDDABrukerProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListBruker* spectrumList,
                                                     pair<int, int>& nscans,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaBrukerProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, bbWidthManager, filetype,
                                                           params);
    }


    /** DDA ABI producer */
    inline  boost::thread getDDAABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                  SpectrumListABI* spectrumList,
                                                  pair<int, int>& nscans,
                                                  map<int, double>& bbWidthManager,
                                                  pwiz::msdata::CVID filetype,
                                                  mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                        nscans, bbWidthManager, filetype,
                                                        params);
    }

    /** SWATH ABI producer */
    inline  boost::thread getSwathABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI* spectrumList,
                                                    pair<int, int>& nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                          nscans, filetype, params);
    }


    /** DIA agilent producer */
    inline boost::thread getDIAAgilentProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListAgilent* spectrumList,
                                                    pair<int, int>& nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaAgilentProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, filetype, params);
    }

    /** DDA agilent producer  */
    inline  boost::thread getDDAAgilentProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListAgilent* spectrumList,
                                                     pair<int, int>& nscans,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaAgilentProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, bbWidthManager, filetype,
                                                           params);
    }


    /** DIA abi t2D producer */
    inline boost::thread getDIAABI_T2DProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI_T2D* spectrumList,
                                                    pair<int, int>& nscans,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaABI_T2DProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, filetype, params);
    }

    /** DDA abi t2d producer  */
    inline  boost::thread getDDAABI_T2DProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListABI_T2D* spectrumList,
                                                     pair<int, int>& nscans,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaABI_T2DProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           nscans, bbWidthManager, filetype,
                                                           params);
    }


    /** */
    inline  boost::thread getSwathGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                        SpectrumList* spectrumList,
                                                        pair<int, int>& nscans,
                                                        pwiz::msdata::CVID filetype,
                                                        mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathGenericProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                              nscans, filetype, params);
    }

    /** */
    inline  boost::thread getDDAGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                      SpectrumList* spectrumList,
                                                      pair<int, int>& nscans,
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
                                                     double& ms1RtWidth,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDiaThermoConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
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


    /** dia bruker consumer */
    inline  boost::thread getDIABrukerConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     double& ms1RtWidth,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDiaBrukerConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
                                                           runSlices, progressionCount, nscans);
    }

    /** dda bruker consumer */
    inline  boost::thread getDDABrukerConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDdaBrukerConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }



    /** dda abi consumer */
    inline  boost::thread getDDAABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                  ISerializer::xml_string_writer& serializer,
                                                  map<int, double>& bbMzWidthByMsLevel,
                                                  map<int, map<int, int> >& runSlices,
                                                  int& progressionCount,
                                                  int nscans ) {
        return this->mDdaABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                        runSlices, progressionCount, nscans);
    }

    /** swath abi consumer */
    inline  boost::thread getSwathABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                    ISerializer::xml_string_writer& serializer,
                                                    map<int, double>& bbMzWidthByMsLevel,
                                                    double& ms1RtWidth,
                                                    map<int, map<int, int> >& runSlices,
                                                    int& progressionCount,
                                                    int nscans ) {
        return this->mSwathABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
                                                          runSlices, progressionCount, nscans);
    }

    /** dia agilent */
    inline  boost::thread getDIAAgilentConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     double& ms1RtWidth,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDiaAgilentConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
                                                           runSlices, progressionCount, nscans);
    }

    /** dda agilent */
    inline  boost::thread getDDAAgilentConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDdaAgilentConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }

    /** dia abi t2D */
    inline  boost::thread getDIAABI_T2DConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     double& ms1RtWidth,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDiaABI_T2DConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
                                                           runSlices, progressionCount, nscans);
    }

    /** dda abi t2D */
    inline  boost::thread getDDAABI_T2DConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int nscans ) {
        return this->mDdaABI_T2DConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, nscans);
    }


    /** */
    inline  boost::thread getSwathGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                        ISerializer::xml_string_writer& serializer,
                                                        map<int, double>& bbMzWidthByMsLevel,
                                                        double& ms1RtWidth,
                                                        map<int, map<int, int> >& runSlices,
                                                        int& progressionCount,
                                                        int nscans ) {
        return this->mSwathGenericConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, ms1RtWidth,
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



} // end namespace

#endif // PCBUILDER_HPP
