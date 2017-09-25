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

/*
 * @file prod_cons_builder.hpp
 * @brief In charge of building adapted `producers` and `consumers` in order to handle conversion.
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

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
              //map<int, DataEncoding>& dataEncodingByID,
              vector<DataEncoding> dataEncodings,
              map<int, double> resolutions,
              bool safeMode):
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
            mDiaThermoProducer = unique_ptr<DIAThermoProducer>(new DIAThermoProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaThermoProducer = unique_ptr<DDAThermoProducer>(new DDAThermoProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDiaThermoConsumer = unique_ptr<DIAThermoConsumer>(new DIAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaThermoConsumer = unique_ptr<DDAThermoConsumer>(new DDAThermoConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        } else if (mode == 2) {
            mDiaBrukerProducer = unique_ptr<DIABrukerProducer>(new DIABrukerProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaBrukerProducer = unique_ptr<DDABrukerProducer>(new DDABrukerProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDiaBrukerConsumer = unique_ptr<DIABrukerConsumer>(new DIABrukerConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaBrukerConsumer = unique_ptr<DDABrukerConsumer>(new DDABrukerConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        } else if (mode == 3) {
            mSwathABIProducer = unique_ptr<SwathABIProducer>(new SwathABIProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaABIProducer = unique_ptr<DDAABIProducer>(new DDAABIProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mSwathABIConsumer = unique_ptr<SwathABIConsumer>(new SwathABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaABIConsumer = unique_ptr<DDAABIConsumer>(new DDAABIConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        } else if (mode == 4) {
            mDiaAgilentProducer = unique_ptr<DIAAgilentProducer>(new DIAAgilentProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaAgilentProducer = unique_ptr<DDAAgilentProducer>(new DDAAgilentProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDiaAgilentConsumer = unique_ptr<DIAAgilentConsumer>(new DIAAgilentConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaAgilentConsumer = unique_ptr<DDAAgilentConsumer>(new DDAAgilentConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        } else if (mode == 5) {
            mDiaABI_T2DProducer = unique_ptr<DIAABI_T2DProducer>(new DIAABI_T2DProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaABI_T2DProducer = unique_ptr<DDAABI_T2DProducer>(new DDAABI_T2DProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDiaABI_T2DConsumer = unique_ptr<DIAABI_T2DConsumer>(new DIAABI_T2DConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaABI_T2DConsumer = unique_ptr<DDAABI_T2DConsumer>(new DDAABI_T2DConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        } else {
            mSwathGenericProducer = unique_ptr<SwathGenericProducer>(new SwathGenericProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mDdaGenericProducer = unique_ptr<DDAGenericProducer>(new DDAGenericProducer(queue, mzdbFile, dataModeByMsLevel, resolutions, safeMode));
            mSwathGenericConsumer = unique_ptr<SwathGenericConsumer>(new SwathGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
            mDdaGenericConsumer = unique_ptr<DDAGenericConsumer>(new DDAGenericConsumer(queue, mzdbFile, paramsCollecter, rawFileFormat, dataEncodings));
        }

    }

    /** DIA thermo producer */
    inline boost::thread getDIAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListThermo* spectrumList,
                                                    pair<int, int>& cycleRange,
                                                    pair<int, int>& rtRange,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaThermoProducer->getProducerThread(levelsToCentroid, spectrumList, cycleRange, rtRange, filetype, params);
    }

    /** DDA thermo producer  */
    inline  boost::thread getDDAThermoProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListThermo* spectrumList,
                                                     pair<int, int>& cycleRange,
                                                     pair<int, int>& rtRange,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaThermoProducer->getProducerThread(levelsToCentroid, spectrumList, cycleRange, rtRange, bbWidthManager, filetype, params);
    }


    /** bruker producer */
    inline boost::thread getDIABrukerProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListBruker* spectrumList,
                                                    pair<int, int>& cycleRange,
                                                    pair<int, int>& rtRange,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaBrukerProducer->getProducerThread(levelsToCentroid, spectrumList, cycleRange, rtRange, filetype, params);
    }

    /** bruker producer  */
    inline  boost::thread getDDABrukerProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListBruker* spectrumList,
                                                     pair<int, int>& cycleRange,
                                                     pair<int, int>& rtRange,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaBrukerProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           cycleRange, rtRange, bbWidthManager, filetype,
                                                           params);
    }


    /** DDA ABI producer */
    inline  boost::thread getDDAABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                  SpectrumListABI* spectrumList,
                                                  pair<int, int>& cycleRange,
                                                  pair<int, int>& rtRange,
                                                  map<int, double>& bbWidthManager,
                                                  pwiz::msdata::CVID filetype,
                                                  mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                        cycleRange, rtRange, bbWidthManager, filetype,
                                                        params);
    }

    /** SWATH ABI producer */
    inline  boost::thread getSwathABIProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI* spectrumList,
                                                    pair<int, int>& cycleRange,
                                                    pair<int, int>& rtRange,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathABIProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                          cycleRange, rtRange, filetype, params);
    }


    /** DIA agilent producer */
    inline boost::thread getDIAAgilentProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListAgilent* spectrumList,
                                                    pair<int, int>& cycleRange,
                                                    pair<int, int>& rtRange,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaAgilentProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           cycleRange, rtRange, filetype, params);
    }

    /** DDA agilent producer  */
    inline  boost::thread getDDAAgilentProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListAgilent* spectrumList,
                                                     pair<int, int>& cycleRange,
                                                     pair<int, int>& rtRange,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaAgilentProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           cycleRange, rtRange, bbWidthManager, filetype,
                                                           params);
    }


    /** DIA abi t2D producer */
    inline boost::thread getDIAABI_T2DProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                    SpectrumListABI_T2D* spectrumList,
                                                    pair<int, int>& cycleRange,
                                                    pair<int, int>& rtRange,
                                                    pwiz::msdata::CVID filetype,
                                                    mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDiaABI_T2DProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           cycleRange, rtRange, filetype, params);
    }

    /** DDA abi t2d producer  */
    inline  boost::thread getDDAABI_T2DProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                     SpectrumListABI_T2D* spectrumList,
                                                     pair<int, int>& cycleRange,
                                                     pair<int, int>& rtRange,
                                                     map<int, double>& bbWidthManager,
                                                     pwiz::msdata::CVID filetype,
                                                     mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaABI_T2DProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                           cycleRange, rtRange, bbWidthManager, filetype,
                                                           params);
    }


    /** */
    inline  boost::thread getSwathGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                        SpectrumList* spectrumList,
                                                        pair<int, int>& cycleRange,
                                                        pair<int, int>& rtRange,
                                                        pwiz::msdata::CVID filetype,
                                                        mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mSwathGenericProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                              cycleRange, rtRange, filetype, params);
    }

    /** */
    inline  boost::thread getDDAGenericProducerThread(pwiz::util::IntegerSet& levelsToCentroid,
                                                      SpectrumList* spectrumList,
                                                      pair<int, int>& cycleRange,
                                                      pair<int, int>& rtRange,
                                                      map<int, double>& bbWidthManager,
                                                      pwiz::msdata::CVID filetype,
                                                      mzPeakFinderUtils::PeakPickerParams& params) {
        return this->mDdaGenericProducer->getProducerThread(levelsToCentroid, spectrumList,
                                                            cycleRange, rtRange, bbWidthManager, filetype, params);
    }

    //consumers
    /** */
    inline  boost::thread getDIAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, double>& bbTimeWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDiaThermoConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }

    /** */
    inline  boost::thread getDDAThermoConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDdaThermoConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }


    /** dia bruker consumer */
    inline  boost::thread getDIABrukerConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, double>& bbTimeWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDiaBrukerConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }

    /** dda bruker consumer */
    inline  boost::thread getDDABrukerConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDdaBrukerConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }



    /** dda abi consumer */
    inline  boost::thread getDDAABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                  ISerializer::xml_string_writer& serializer,
                                                  map<int, double>& bbMzWidthByMsLevel,
                                                  map<int, map<int, int> >& runSlices,
                                                  int& progressionCount,
                                                  int cycleRange ) {
        return this->mDdaABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                        runSlices, progressionCount, cycleRange);
    }

    /** swath abi consumer */
    inline  boost::thread getSwathABIConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                    ISerializer::xml_string_writer& serializer,
                                                    map<int, double>& bbMzWidthByMsLevel,
                                                    map<int, double>& bbTimeWidthByMsLevel,
                                                    map<int, map<int, int> >& runSlices,
                                                    int& progressionCount,
                                                    int cycleRange ) {
        return this->mSwathABIConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                          runSlices, progressionCount, cycleRange);
    }

    /** dia agilent */
    inline  boost::thread getDIAAgilentConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, double>& bbTimeWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDiaAgilentConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }

    /** dda agilent */
    inline  boost::thread getDDAAgilentConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDdaAgilentConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }

    /** dia abi t2D */
    inline  boost::thread getDIAABI_T2DConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, double>& bbTimeWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDiaABI_T2DConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }

    /** dda abi t2D */
    inline  boost::thread getDDAABI_T2DConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                     ISerializer::xml_string_writer& serializer,
                                                     map<int, double>& bbMzWidthByMsLevel,
                                                     map<int, map<int, int> >& runSlices,
                                                     int& progressionCount,
                                                     int cycleRange ) {
        return this->mDdaABI_T2DConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                           runSlices, progressionCount, cycleRange);
    }


    /** */
    inline  boost::thread getSwathGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                        ISerializer::xml_string_writer& serializer,
                                                        map<int, double>& bbMzWidthByMsLevel,
                                                        map<int, double>& bbTimeWidthByMsLevel,
                                                        map<int, map<int, int> >& runSlices,
                                                        int& progressionCount,
                                                        int cycleRange ) {
        return this->mSwathGenericConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel, bbTimeWidthByMsLevel,
                                                              runSlices, progressionCount, cycleRange);
    }

    /** */
    inline  boost::thread getDDAGenericConsumerThread(pwiz::msdata::MSDataPtr msdata,
                                                      ISerializer::xml_string_writer& serializer,
                                                      map<int, double>& bbMzWidthByMsLevel,
                                                      map<int, map<int, int> >& runSlices,
                                                      int& progressionCount,
                                                      int cycleRange ) {
        return this->mDdaGenericConsumer->getConsumerThread(msdata, serializer, bbMzWidthByMsLevel,
                                                            runSlices, progressionCount, cycleRange);
    }

};



} // end namespace

#endif // PCBUILDER_HPP
