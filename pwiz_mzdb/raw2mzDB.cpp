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

#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <share.h>
#include <eh.h>
#endif

#include "boost/algorithm/string.hpp"

#include "mzdb/lib/getopt_pp/include/getopt_pp.h"
#include "mzdb/writer/mzdb_writer.hpp"
#include "mzdb/utils/MzDBFile.h"

#include "mzdb/utils/glog/logging.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

#ifdef _WIN32
void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp ){
    throw runtime_error("SEH exception.");
}
#endif


/// Warning, calling this function will delete a previous mzdb file.
int deleteIfExists(const string &filename) {

    if (std::ifstream(filename.c_str())) {
        //file exist
        printf("\n");
        LOG(WARNING) << "Found file with the same name";
        LOG(WARNING) << "Delete...";
        if (remove(filename.c_str()) != 0) {
            LOG(ERROR) << "Error trying to delete file, exiting...May the file is opened elsewhere ?";
            exit(EXIT_FAILURE);
        } else
            LOG(INFO) << "Done";
    }
    return 0;
}

/// windows specific
#ifdef _WIN32 
bool alreadyOpened(const string& filename) {

    FILE* stream;
    if ((stream = _fsopen(filename.c_str(), "at", _SH_DENYNO)) == NULL)
        return true;
    fclose(stream);
    return false;
}
#endif

/// TODO: why using a different than above, could be more portable (ifstream)
inline bool exists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

/// Parse command line, range (1-, 1-3)
void parseRange(string& range, DataMode mode,
                         map<int, DataMode>& results,
                         vector<int>& modifiedIndex, const string& help) {
    if (range != "") {
        if (range.size() > 3) {
            LOG(ERROR) << "Error in parsing command line";
            std::cout << help << endl;
            exit(EXIT_FAILURE);
        }
        if (range.size() == 1) {
            int p;
            try {
                p = boost::lexical_cast<int>(range);
            } catch (boost::bad_lexical_cast &) {
                LOG(ERROR) << "Error, index must be an integer";
                std::cout << help << endl;
                exit(EXIT_FAILURE);
            }
            results[p] = mode;
            modifiedIndex.push_back(p);

        }
        if (range.size() == 3) {
            vector<string> splitted;
            boost::split(splitted, range, boost::is_any_of("-"));
            int p1 = 0, p2 = 0;
            try {
                p1 = boost::lexical_cast<int>(splitted[0]);
                p2 = boost::lexical_cast<int>(splitted[1]);
            } catch (boost::bad_lexical_cast &) {
                LOG(ERROR) << "Error, the two indexes must be integers";
                std::cout << help << endl;
                exit(EXIT_FAILURE);
            }
            if (p1 && p2 && p1 <= p2) {
                for (int i = p1; i <= p2; ++i) {
                    results[i] = mode;
                    modifiedIndex.push_back(i);
                }
            } else {
                std::cout << help << "\n";
                LOG(ERROR) << "Error parameter 2 greater than parameter 1";
                exit(EXIT_FAILURE);
            }
        }
    }
}

/// Starting point !
int main(int argc, char* argv[]) {
    //#ifdef _WIN32
    //    //set se translator
    //    LOG(INFO) << "Setting custom translator for SEH exception.";
    //    _set_se_translator(trans_func);
    //#endif

    string filename = "", centroid = "", profile = "", fitted = "", namefile = "", serialization = "xml";
    bool compress = false;

    //---bounding boxes sizes defaults
    float bbWidth = 15, bbWidthMSn = 0; //15
    float bbHeight = 5, bbHeightMSn = 10000; //5

    //---32 bits intensity encoding by default
    bool noLoss = false;

    //---force data storage as dia
    bool dia=false;

    int peakPickingAlgo = 0, nscans = 0, nbCycles = 3, maxNbThreads = 1;


    // filling several helping maps
    map<int, DataMode> dataModeByMsLevel;
    map<int, string> dataModes;

    //init results by default a Ms1 is fitted others are centroided
    //keys of dataMode are the sizes of structures in bytes,  so
    //this not really portable
    dataModeByMsLevel[1] = FITTED;
    dataModes[1] = "PROFILE";
    dataModes[20] = "FITTED";
    dataModes[12] = "CENTROID";

    for (size_t i = 2; i <= MAX_MS; ++i) {
        dataModeByMsLevel[i] = CENTROID;
    }

    const string help = "\n\nusage: raw2mzDB.exe --input filename <parameters>"
                  "\n\nOptions:\n\n"
                  "\t-i, --input : specify the input rawfile path\n"
                  "\t-o, --output : specify the output filename (must be an absolute path)\n"
                  "\t-c, --centroid : centroidization, eg: -c 1 (centroidization msLevel 1) or -c 1-5 (centroidization msLevel 1 to msLevel 5) \n"
                  "\t-p, --profile : idem but for profile mode \n"
                  "\t-f, --fitted : idem buf for fitted mode \n"
                  "\t-T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s\n"
                  "\t-t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s\n"
                  "\t-M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da \n"
                  "\t-m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da \n"
                  "\t--dia : will force the conversion in DIA mode\n"
                  //"\t--bufferSize : low value (min 2) will enforce the program to use less memory (max 50), default: 3\n"
                  // "\t--max_nb_threads : maximum nb_threads to use, default: nb processors on the machine\n"
                  "\t--no_loss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)\n "
                  "\t--nscans : nb scans to convert into the mzDB file (max: number of scans in the rawfile)\n"
                  "\t-h --help : show help";


    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('c', "centroid", centroid);
    ops >> Option('p', "profile", profile);
    ops >> Option('f', "fitted", fitted);
    ops >> Option('T', "bbTimeWidth", bbWidth);
    ops >> Option('M', "bbMzWidth", bbHeight);
    ops >> Option('t', "bbTimeWidthMSn", bbWidthMSn);
    ops >> Option('m', "bbMzWidthMSn", bbHeightMSn);
    //ops >> Option('s', "serialize", serialization);
    ops >> Option('o', "output", namefile);
    ops >> Option('n', "nscans", nscans);
    ops >> Option("no_loss", noLoss);
    ops >> Option("dia", dia);
    //ops >> Option("bufferSize", nbCycles);
    //ops >> Option("max_nb_threads", maxNbThreads);

    if (ops >> OptionPresent('h', "help")) {
        std::cout << help << std::endl;
        exit(EXIT_SUCCESS);
    }
    //ops >> Option('C', "compress", compression)
    if (ops.options_remain()) {
        LOG(ERROR) <<"Oops! Unexpected options. Refer to help";
        std::cout << help << std::endl;
        exit(EXIT_FAILURE);
    }

    if (! exists(filename)) {
        LOG(ERROR) << "Filepath does not exist.Exiting.";
        std::cout << help << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!filename.size()) {
        LOG(ERROR) << "empty raw filename ! Exiting...";
        std::cout << help << std::endl;
        exit(EXIT_FAILURE);
    }

#ifdef WIN32
    if (alreadyOpened(filename)) {
        LOG(ERROR) << "File already open in another application.\nPlease close it to perform conversion.";
        exit(EXIT_FAILURE);
    }
#endif

    int res = deleteIfExists(filename + ".mzDB");
    if (res == 1) {
        //should never pass here
      exit(EXIT_FAILURE);
    }

    vector<int> modifiedIndex;

    parseRange(profile, PROFILE, dataModeByMsLevel, modifiedIndex, help);
    parseRange(fitted, FITTED, dataModeByMsLevel, modifiedIndex, help);
    parseRange(centroid, CENTROID, dataModeByMsLevel, modifiedIndex, help);

    //---print gathered informations
    std::cout << "\nWhat I understood :\n";
    std::cout << "Treating: " << filename << "\n";
    for (auto it = dataModeByMsLevel.begin(); it != dataModeByMsLevel.end(); ++it) {
        std::cout << "ms " << it->first << " => selected Mode: " << dataModes[(int) it->second] << "\n";
    }
    std::cout << "\n";
    //end parsing commandline


    //--- Sarting launching code
    //create a mzDBFile
    MzDBFile f(filename, bbHeight, bbHeightMSn, bbWidth, bbWidthMSn, noLoss);

    //---pwiz file detection
    ReaderPtr readers(new FullReaderList);
    vector<MSDataPtr> msdList;

    try {
        ( (FullReaderList*) readers.get() )->read(f.name, msdList);
    } catch(exception& e) {
        LOG(ERROR) << e.what() << endl;
        LOG(FATAL) << "This a fatal error. Exiting..." << endl;
        exit(EXIT_FAILURE);
    } catch(...) {
        LOG(FATAL) << "Unknown fatal exception. Exiting...";
        exit(EXIT_FAILURE);
    }

    auto& msData = msdList[0];
    auto originFileFormat = pwiz::msdata::identifyFileFormat( readers, f.name );

    mzDBWriter writer(f, msData, originFileFormat, dataModeByMsLevel, compress);

    //---insert metadata
    try {
        writer.checkMetaData();
    } catch (exception& e) {
        LOG(ERROR) << "Error checking metadata: ";
        LOG(ERROR) << "\t->" << e.what();
    } catch(...) {
        LOG(ERROR) << "unknown fatal exception. Exiting...";
    }

    //---check swath mode
    if (dia)
        writer.setSwathAcquisition(true);
    else {
        try {
            writer.isSwathAcquisition();
        } catch (exception& e) {
            LOG(ERROR) << "Error checking DDA/SWATH Mode: ";
            LOG(ERROR) << "\t->" << e.what();
        } catch(...) {
            LOG(ERROR) << "Unknown error checking DDA/SWATH Mode. Default to DDA...";
        }
    }

    //---overwrite swath mode if needed
//    if (dia) {
//        if (! writer.getSwathAcquisition()) {
//            LOG(INFO) << "Overwriting acquisition to swath mode...";
//            writer.setSwathAcquisition(true);
//        }
//    }

    //---create parameters for peak picking
    mzPeakFinderUtils::PeakPickerParams p;

    clock_t beginTime = clock();

    try {
        if (noLoss) {
            //LOG(INFO) << "No-loss mode encoding: all ms Mz-64, all ms Int-64";
            writer.writeNoLossMzDB(namefile, nscans, nbCycles, p);
        } else {
            if (false) { // If msInstrument only good at ms1
                //LOG(INFO) << "ms1 Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzMs1Hi(namefile, nscans, nbCycles, p);
            } else {
                //LOG(INFO) << "all ms Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzHi(namefile, nscans, nbCycles, p);
            }
        }

        LOG(INFO) << "Checking run slices numbers";
        writer.checkAndFixRunSliceNumberAnId();

    } catch (exception& e) {
        LOG(ERROR) << e.what();
    } catch(...) {
        LOG(ERROR) << "Unknown error. Unrecoverable. Exiting...";
        exit(EXIT_FAILURE);
    }

   clock_t endTime = clock();
   LOG(INFO) << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << endl;
    return 0;
}
