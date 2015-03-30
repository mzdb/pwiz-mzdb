#include "boost/filesystem/path.hpp"

#include "pwiz/utility/misc/IterationListener.hpp"

#include "pwiz/analysis/passive/MSDataAnalyzer.hpp"
#include "pwiz/analysis/passive/MSDataCache.hpp"
#include "pwiz/analysis/passive/RegionSIC.hpp"
#include "pwiz/analysis/passive/RegionSlice.hpp"

#include "../reader/msdata/mzScan.hpp"
#include "mzDBConverter.h"

using namespace std;

namespace mzdb {

mzConverter::mzConverter( MzDBFile& mzdb) : _mzdb(mzdb) {
    mzdbReader = new mzDBReader(_mzdb);
}

mzConverter::~mzConverter() {
    delete mzdbReader;
}

void mzConverter::convert(bool noLoss, mzConverter::FileFormat fileFormat) {
    //use pwiz api to convert to the wanted format

#define f_size sizeof(float);
    //this->enumerateSpectra();
    mzdbReader->readMzDB(msd, noLoss);



    switch (fileFormat) {
    case ((int) MZ_XML): {
        try {

            MSDataFile::WriteConfig conf;
            conf.format = MSDataFile::Format_mzXML;
            conf.indexed = true;
            pwiz::util::IterationListenerRegistry iterationListenerRegistry;
            const size_t iterationPeriod = 100;
            iterationListenerRegistry.addListener(pwiz::util::IterationListenerPtr(new UserFeedbackIterationListener), iterationPeriod);
            pwiz::util::IterationListenerRegistry* pILR = &iterationListenerRegistry ;


            if (!noLoss) {
                printf("conversion to intensity 32 bits\n");
                conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
                conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
                conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_32;
            }
            else {
                printf("conversion to intensity 64 bits\n");
                conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
                conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
                conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_64;
            }
            MSDataFile::write(msd, mzdbReader->filename() + ".mzXML", conf, pILR);
            //printf("File written\n");
        } catch (exception &e) {
            std::cout << e.what() << std::endl;
        }
    }
    default :{
        printf("conversion not supported");
    }
    }
}


void mzConverter::regionExtractorPwizImpl(const MSDataFile& msdata,
                                                        const string& filename,
                                                        double minmz,
                                                        double maxmz,
                                                        double minrt,
                                                        double maxrt) const {

    ostream* log = &std::cerr;
    try {


        //printf("msdata read\n");
        //printf("number of spectra:%d\n", msdata->run.spectrumListPtr->size());
        pwiz::analysis::MSDataAnalyzerContainer analyzers;
        boost::shared_ptr<pwiz::analysis::MSDataCache> cache(new pwiz::analysis::MSDataCache);
        analyzers.push_back(cache);
        //printf("cache created\n");

        pwiz::analysis::RegionSlice::Config conf(" mz=[" + boost::lexical_cast<string > (minmz) + "," + boost::lexical_cast<string > (maxmz) + "] rt=[" + boost::lexical_cast<string > (minrt) + "," + boost::lexical_cast<string > (maxrt) + "]");
        //conf.mzRange = make_pair(minmz, maxmz);
        //conf.rtRange = make_pair(minrt, maxrt);
        //conf.dumpRegionData = false;
        pwiz::analysis::MSDataAnalyzerPtr regionSlice(new pwiz::analysis::RegionSlice(*cache, conf));
        analyzers.push_back(regionSlice);

        //printf("Region Slice created\n");


        pwiz::analysis::MSDataAnalyzer::DataInfo dataInfo(msdata);
        //dataInfo.sourceFilename = boost::filesystem::path(filename).leaf();
        dataInfo.outputDirectory = ".";
        dataInfo.log = log;
        //printf("Data info created\n");

        pwiz::analysis::MSDataAnalyzerDriver driver(analyzers);
        //printf("msdata analyzer created\n");
        driver.analyze(dataInfo);
        //printf("end function\n");
        //printf("cache size:%d\n", cache.size());


    } catch (exception& e) {
        if (log) *log << e.what() << "\n[MSDataAnalyzerApplication] Caught exception.\n";
    } catch (...) {
        if (log) *log << "[MSDataAnalyzerApplication] Caught unknown exception.\n";
    }

    //if (log) *log << endl;

}

void mzConverter::xicExtractorPwizImpl(const MSDataFile& msdata,
                                                     const string& filename,
                                                     double mz,
                                                     double radius) const {
    ostream* log = &std::cerr;
    try {


        //printf("msdata read\n");
        //printf("number of spectra:%d\n", msdata->run.spectrumListPtr->size());
        pwiz::analysis::MSDataAnalyzerContainer analyzers;
        boost::shared_ptr<pwiz::analysis::MSDataCache> cache(new pwiz::analysis::MSDataCache);
        analyzers.push_back(cache);
        //printf("cache created\n");

        pwiz::analysis::RegionSIC::Config conf(boost::lexical_cast<string > (mz) + " " + boost::lexical_cast<string > (radius) + " amu");

        //conf.mzRange = make_pair(minmz, maxmz);
        //conf.rtRange = make_pair(minrt, maxrt);
        //conf.dumpRegionData = false;

        pwiz::analysis::MSDataAnalyzerPtr regionSic(new pwiz::analysis::RegionSIC(*cache, conf));

        analyzers.push_back(regionSic);

        //printf("Region Slice created\n");


        pwiz::analysis::MSDataAnalyzer::DataInfo dataInfo(msdata);
        //dataInfo.sourceFilename = boost::filesystem::path(filename).leaf();
        dataInfo.outputDirectory = ".";
        dataInfo.log = log;
        //printf("Data info created\n");

        pwiz::analysis::MSDataAnalyzerDriver driver(analyzers);
        //printf("msdata analyzer created\n");
        driver.analyze(dataInfo);
        //printf("end function\n");
        //printf("cache size:%d\n", cache.size());


    } catch (exception& e) {
        if (log) *log << e.what() << "\n[MSDataAnalyzerApplication] Caught exception.\n";
    } catch (...) {
        if (log) *log << "[MSDataAnalyzerApplication] Caught unknown exception.\n";
    }

    //if (log) *log << endl;
}


void mzConverter::enumerateSpectra()  {
    printf("Retrieve data...");
    mzdbReader->readMzDB(msd, false);
    printf("Done\n");
    SpectrumList& sl = *msd.run.spectrumListPtr;
    size_t totalArrayLength = 0;
    clock_t beginTime = clock();
    std::cout << sl.size() << std::endl;
    for (size_t i = 0; i < sl.size(); ++i) {
        SpectrumPtr s = sl.spectrum(i, true);
        //cout << i << ", " << s->id << endl;
        try {
            mzScan* s_  = (mzScan*)s.get();
            //cout << s_->mz.size() << endl;
        } catch (exception& e) {
            std::cout << e.what() << std::endl;
        } catch(...) {
            std::cout << "That's too bad\n "<< std::endl;
        }

        totalArrayLength += s->defaultArrayLength;
        if (i + 1 == sl.size() || ((i + 1) % 100) == 0)
            std::cout << "Enumerating spectra: " << (i + 1) << '/' << sl.size() << " (" << totalArrayLength << " data points)\r" << std::flush;
    }
    std::cout << std::endl;
    clock_t endTime = clock();
    std::cout << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << std::endl << std::endl;
}



/*PWIZ_API_DECL void mzConverter::rtreeRequest()  {

        mzRegionExtractor extractor(_mzdb);
        printf("after region extrator constructor\n");
        srand(time(0));
        clock_t beginTime_ = clock();
        int count = 0;
        for (int i = 0; i < 100; ++i) {
            double mz = unifRand(300, 1995);
            vector<mzScan*> s;
            extractor.rTreeExtraction(mz, mz + 5., 0., 24000., 1, s);
            //printf("sizeof s:%d\n", s.size());
            for (size_t k = 0; k < s.size(); k++) {
                count += s[k]->mz.size();
                delete s[k];
            }
        }
        printf("data points found:%d\n", count);
        clock_t endTime_ = clock();
        cout << "Elapsed Time: " << ((double) endTime_ - beginTime_) / CLOCKS_PER_SEC << " sec" << endl;
    }*/

}//end namespace
