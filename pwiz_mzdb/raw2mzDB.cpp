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
 * @file raw2mzDB.cpp
 * @brief Start file with the main function
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

// this is needed especially on Windows because windows.h already defines an ERROR security level
// so this command needs to be called before windows.h and glog/logging.h
#define GLOG_NO_ABBREVIATED_SEVERITIES

#ifdef _WIN32
#include <share.h>
#include <eh.h>
#endif

#include "boost/algorithm/string.hpp"

#include "mzdb/lib/getopt_pp/include/getopt_pp.h"
#include "mzdb/writer/mzdb_writer.hpp"
#include "mzdb/utils/MzDBFile.h"

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#endif

using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;
namespace fs = boost::filesystem;

//#ifdef _WIN32
//void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp ){
//    LOG(ERROR) << "about to throw a converted exception";
//    throw runtime_error("SEH exception.");
//}
//#endif

/*
 * Piece of code provided by Microsoft MSDN: https://msdn.microsoft.com/en-us/library/ms682621(VS.85).aspx
 * it lists the dlls currently loaded
 * it has been added when Andrea Kalaitzakis and Veronique Dupierris asked for a way to check on the fly if all required dlls are loaded
 * in order to tell the end user which dll is missing (or even better, which software should be installed)
 */
//int PrintModules(DWORD processID) {
//    HMODULE hMods[1024];
//    HANDLE hProcess;
//    DWORD cbNeeded;
//
//    // Print the process identifier.
//    printf("\nProcess ID: %u\n", processID);
//    // Get a handle to the process.
//    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );
//    if (NULL == hProcess) return 1;
//
//    // Get a list of all the modules in this process.
//    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
//        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
//            TCHAR szModName[MAX_PATH];
//            // Get the full path to the module's file.
//            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
//                // Print the module name and handle value.
//                _tprintf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
//            }
//        }
//    }
//    // Release the handle to the process.
//    CloseHandle(hProcess);
//    return 0;
//}

/// Warning, calling this function will delete a previous mzdb file.
int deleteIfExists(const string &filename) {

    if (std::ifstream(filename.c_str())) {
        //file exist
        //printf("\n");
        LOG(WARNING) << "";
        LOG(WARNING) << "Found file with the same name";
        LOG(WARNING) << "Delete...";
        if (remove(filename.c_str()) != 0) {
            LOG(ERROR) << "Error trying to delete file, exiting...May the file is opened elsewhere ?";
            exit(EXIT_FAILURE);
        } else {
            LOG(INFO) << "Done";
        }
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

pair<int, int> parseCycleRange(string& range) {
    size_t min = 0, max = 0;
    std::smatch sm;
    std::regex e("^([0-9]*)-([0-9]*)$");
    std::regex_match (range, sm, e);
    if (sm.size() == 3) { // sm contains { range, item 1, item 2 }
        try {
            if(sm[1] != "") min = stoi(sm[1]);
            if(sm[2] != "") max = stoi(sm[2]);
            if(min < max || max == 0) {
                // TODO what if min == 1 ? does it mean first or second ?
                return make_pair<int, int>(min, max);
            } else {
                LOG(ERROR) << "Error, max index (" << max << ") cannot be lower than min index (" << min << ")";
            }
        } catch (...) {} // should never come here because regex has already checked that values where integers
    }
    return make_pair<int, int>(0,0);
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
        else if (range.size() == 1) {
            // example: -p 1
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
        else if (range.size() == 3) {
            // example: -p 1-2
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
                LOG(ERROR) << "Error index 2 greater than index 1";
                exit(EXIT_FAILURE);
            }
        }
        // TODO remove this functionnality because MAX_MS is not known yet
        else if (range.size() == 2) {
            int MAX_MS = 2; // ABU added just to make it work
            // example: -p 1-
            vector<string> splitted;
            boost::split(splitted, range, boost::is_any_of("-"));
            int p1 = 0;
            try {
                p1 = boost::lexical_cast<int>(splitted[0]);
                if (p1 > MAX_MS) {
                    LOG(ERROR) << "Error,  index 1, " << p1 <<" bigger than index 2" << MAX_MS;
                    exit(EXIT_FAILURE);
                }
                for (int i = p1; i <= MAX_MS; ++i) {
                    results[i] = mode;
                    modifiedIndex.push_back(i);
                }

            } catch(boost::bad_lexical_cast &) {
                LOG(ERROR) << "Error, index must be integer";
                exit(EXIT_FAILURE);
            }
        }
    }
}

string getExecutableDirectory(string argv0) {
    //boost::filesystem::path argv0AsPath = argv0;
    //boost::filesystem::path exePath = boost::filesystem::system_complete(argv0AsPath);
    fs::path argv0AsPath = argv0;
    fs::path exePath = fs::system_complete(argv0AsPath);
    return exePath.parent_path().string();
}

string getBuildDate(string argv0) {
    string exePath = getExecutableDirectory(argv0);
    string filename = exePath + "/build.txt";
    std::ifstream fin( filename.c_str() );
    string date;
    if(fin) {
        getline(fin, date); // read only first line
    } else {
        // try to get raw2mzDB.exe creation date ? No, date may change for some reason and it would be a false information
        date = "";
    }
    return date;
}

std::string toLower(std::string input) {
    std::locale locale;
    std::string output = "";
    for (std::string::size_type i = 0; i < input.length(); i++)
        output += std::tolower(input[i], locale);
    return output;
}

bool fileExists(std::string inputFileName, bool quitIfFileDoesNotExist, const string& help) {
    // transform the path into a boost file object and test if it exists
    fs::path bFileName(inputFileName);
    if(!fs::exists(bFileName)) {
        // file does not exist
        if(quitIfFileDoesNotExist) {
            LOG(ERROR) << "File '" << inputFileName << "' does not exist. Exiting.";
            std::cout << help << std::endl;
            exit(EXIT_FAILURE);
        } else return false; // file does not exists but it's just informational
    } else return true; // file exists
}

void checkInputFile(std::string &inputFileName, const string& help) {
    // check that file exists, quit if not
    fileExists(inputFileName, true, help);
    // declare variables for the tests
    std::string lc_inputFileName = toLower(inputFileName);
    fs::path bFileName(inputFileName);
    // if it's a .d folder, check that the associated analysis.baf file also exists and point to it
    if(fs::is_directory(bFileName)) {
        if(std::regex_match(lc_inputFileName, std::regex(".*\\.d$"))) {
            // TODO what about Agilent files ?
            if(fileExists(inputFileName + "/analysis.baf", false, help)) {
                inputFileName = inputFileName + "/analysis.baf"; // Bruker file format
            //} else if(fileExists(inputFileName + "/Analysis.yep", false, help)) {
            //    inputFileName = inputFileName + "/Analysis.yep"; // Bruker file format
            }
        //} else if(std::regex_match(lc_inputFileName, std::regex(".*\\.raw$"))) {
        //    // else if Waters raw folder ?
        //} else {
        }
    } else {
        // if it's a wiff file, check that the associated wiff.scan file also exists
        if(std::regex_match(lc_inputFileName, std::regex(".*\\.wiff$"))) {
            fileExists(inputFileName + ".scan", true, help);
        } else if(std::regex_match(lc_inputFileName, std::regex(".*\\.wiff.scan$"))) {
            // if it's a wiff.scan file, check that the associated wiff file also exists and point to it
            inputFileName = inputFileName.substr(0, inputFileName.length() - 5);
        }
    }
    // final check with the potentially modified file name
    fileExists(inputFileName, true, help);
}

void checkOutputFile(std::string &outputFileName, std::string &inputFileName) {
    // if no output file name has been given, use input file name
    if (outputFileName == "") outputFileName = inputFileName;
    // if input data is a Bruker .d directory, then only keep .d directory name
    if(std::regex_match(toLower(outputFileName), std::regex(".*analysis.baf$"))) {
        outputFileName = outputFileName.substr(0, outputFileName.length() - 13);
    }
    // add the mzDB extension if needed
    if(!std::regex_match(toLower(outputFileName), std::regex(".*\\.mzdb$"))) {
        outputFileName = outputFileName + ".mzDB";
    }
    // if a file with the same name already exist, delete it !
    int res = deleteIfExists(outputFileName);
    if (res == 1) {
        //should never pass here
        exit(EXIT_FAILURE);
    }
}

/// Starting point !
int main(int argc, char* argv[]) {
    
    // list of loaded dlls
    //PrintModules(GetCurrentProcessId());

    google::InitGoogleLogging(argv[0]); // set this on to allow logging options (such as writting logs to a file)
    //FLAGS_logbufsecs = 0; // DEBUG ONLY !! use this to print log messages instantly, it will probably slow everything... (does not seem to work...)

//    #ifdef _WIN32
//        //set se translator
//        LOG(INFO) << "Setting custom translator for SEH exception.";
//        _set_se_translator(trans_func);
//    #endif
    string filename = "", centroid = "", profile = "", fitted = "", outputFileName = "", serialization = "xml", cycleRangeStr="";
    string logToWhat = "";
    bool compress = false;
    //double ppm = 20.0;


    //---bounding boxes sizes defaults
    float bbWidth = 15, bbWidthMSn = 0; //15
    float bbHeight = 5, bbHeightMSn = 10000; //5

    //---32 bits intensity encoding by default
    bool noLoss = false;

    //---force data storage as dia
    string acquisitionMode = "AUTO";

    bool safeMode=false;

    int peakPickingAlgo = 0, nscans = 0, nbCycles = 3, maxNbThreads = 1;


    // filling several helping maps
    map<int, DataMode> dataModeByMsLevel;

    //init results by default a Ms1 is fitted others are centroided
    //keys of dataMode are the sizes of structures in bytes,  so
    //this not really portable
    dataModeByMsLevel[1] = FITTED;

    int MAX_MS = 2; // ABU added this just to make it work
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
                  //"\t--ppm : ppm parameter for peak detection algorithm of WIFF files, default: 20\n"
                  "\t-T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s\n"
                  "\t-t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s\n"
                  "\t-M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da \n"
                  "\t-m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da \n"
                  "\t-a, --acquisition : dda, dia or auto (converter will try to determine if the analysis is DIA or DDA), default: auto\n"
                  //"\t--bufferSize : low value (min 2) will enforce the program to use less memory (max 50), default: 3\n"
                  // "\t--max_nb_threads : maximum nb_threads to use, default: nb processors on the machine\n"
                  "\t--noLoss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)\n "
                  "\t--cycles : only convert the selected range of cycles, eg: 1-10 (first ten cycles) or 10- (from cycle 10 to the end) ; using this option will disable progress information\n"
                  "\t-s, --safeMode : use centroid mode if the requested mode is not available\n"
                  "\t--log : console, file or both (log file will be put in the same directory as the output file), default: console\n"
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
    ops >> Option('o', "output", outputFileName);
    ops >> Option("log", logToWhat);
    ops >> Option('n', "cycles", cycleRangeStr);
    ops >> OptionPresent("noLoss", noLoss);
    ops >> Option('a', "acquisition", acquisitionMode);
    ops >> OptionPresent('s', "safeMode", safeMode);
    
    //std::cout << "dia:" << dia << std::endl;
    //ops >> Option("ppm", ppm);
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
    
    // check input file name, change it if necessary (ie. XXX.wiff.scan file given instead of XXX.wiff file, or XXX.d folder instead of XXX.d/analysis.baf)
    checkInputFile(filename, help);

    if (!filename.size()) {
        LOG(ERROR) << "empty raw filename ! Exiting...";
        std::cout << help << std::endl;
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    if (alreadyOpened(filename)) {
        LOG(ERROR) << "File already open in another application.\nPlease close it to perform conversion.";
        exit(EXIT_FAILURE);
    }
#endif

    // check output file name, use input file name if missing, make sure to have the right extension, delete former file if any
    checkOutputFile(outputFileName, filename);

    // where should logs be written
    if(boost::to_upper_copy(logToWhat) == "FILE" || boost::to_upper_copy(logToWhat) == "BOTH") {
        // write logs to a file in the same directory as the output file
        // Default log file format is based on this (cf. glog/logging.h.in:270)
        // Unless otherwise specified, logs will be written to the filename
        // "<program name>.<hostname>.<user name>.log.<severity level>.", followed
        // by the date, time, and pid (you can't prevent the date, time, and pid
        // from being in the filename).
        FLAGS_logtostderr = 0;
        // write to file and to console
        if(boost::to_upper_copy(logToWhat) == "BOTH") {
            FLAGS_alsologtostderr = 1;
        }
        // log file has the name of the input file, but is stored in the output directory
        fs::path output(outputFileName);
        string logFile = fs::absolute(output).parent_path().string() + "/" + filename;
        google::SetLogDestination(google::GLOG_INFO, logFile.c_str()); // first parameter means that all levels from FATAL to INFO will be put in the given file
        google::SetLogFilenameExtension(".log."); // date, time, and pid will be appended to the extension (and this cannot be modified in the glog library)
    } else {
        // write logs to standard and error outputs
        FLAGS_logtostderr = 1;
    }

    //--- overriding default encoding to `fitted` for DIA mslevel 2
    if(boost::to_upper_copy(acquisitionMode) == "DIA") {
        dataModeByMsLevel[2] = FITTED;

        if (bbHeightMSn != 10000 || bbWidthMSn != 0) {
            LOG(WARNING) << "Warning: 'bbMzWidthMSn' and/or 'bbRtWidthMSn' when 'dia' is set will be ignored !";
        }

        // create bounding box of the same dimensions than MS1
        bbHeightMSn = bbHeight;
        bbWidthMSn = bbWidth;

        //dataModeByMsLevel[3] = FITTED;
    }

    //todo use set instead
    vector<int> modifiedIndex;

    parseRange(profile, PROFILE, dataModeByMsLevel, modifiedIndex, help);
    parseRange(fitted, FITTED, dataModeByMsLevel, modifiedIndex, help);
    parseRange(centroid, CENTROID, dataModeByMsLevel, modifiedIndex, help);

    pair<int, int> cycleRange = parseCycleRange(cycleRangeStr);

    //---print gathered informations
    std::stringstream summary;
    summary << "\n\n************ Summary of the parameters ************\n\n";
    summary << "Treating file: " << filename << "\n";
    for (auto it = dataModeByMsLevel.begin(); it != dataModeByMsLevel.end(); ++it) {
        summary << "ms " << it->first << " => selected Mode: " << modeToString(it->second) << "\n";
    }
    
    if(cycleRange.first > 1 || cycleRange.second != 0) {
        summary << "Converting cycles from ";
        if(cycleRange.first == 0) summary << "first"; else summary << "#" << cycleRange.first;
        summary << " to ";
        if(cycleRange.second == 0) summary << "last"; else summary << "#" << cycleRange.second;
        summary << "\n";
    }
    summary << "\n";

    summary << "Bounding box dimensions:\n";
    summary << "MS 1\t" << "m/z: " << bbHeight << " Da, retention time: " << bbWidth << "sec\n";
    summary << "MS n\t" << "m/z: " << bbHeightMSn << " Da, retention time: " << bbWidthMSn << "sec\n";

    summary << "\n";
    LOG(INFO) << summary.str();
    //end parsing commandline

    //--- Starting launching code
    //create a mzDBFile
    MzDBFile f(filename, outputFileName, bbHeight, bbHeightMSn, bbWidth, bbWidthMSn, noLoss);

    //---pwiz file detection
    ReaderPtr readers(new FullReaderList);
    vector<MSDataPtr> msdList;

    try {
        ((FullReaderList*) readers.get())->read(f.name, msdList);
    } catch(exception& e) {
        LOG(ERROR) << e.what() << endl;
        LOG(FATAL) << "This a fatal error. Exiting..." << endl;
        exit(EXIT_FAILURE);
    } catch(...) {
        LOG(FATAL) << "Unknown fatal exception. Exiting...";
        exit(EXIT_FAILURE);
    }

    auto& msData = msdList[0];
    auto originFileFormat = pwiz::msdata::identifyFileFormat(readers, f.name);

    // TODO add a check on the content of the raw file
    // make sure right now that dataModeByMsLevel is correct
    // that way, the user will know what he will get
    // it will take some time due to the reading of the raw file

    mzDBWriter writer(f, msData, originFileFormat, dataModeByMsLevel, getBuildDate(argv[0]), compress, safeMode);
    
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
    if(boost::to_upper_copy(acquisitionMode) == "DIA") {
        writer.setSwathAcquisition(true);
    } else if(boost::to_upper_copy(acquisitionMode) == "DDA") {
        writer.setSwathAcquisition(false);
    } else {
        try {
            writer.isSwathAcquisition();
        } catch (exception& e) {
            LOG(ERROR) << "Error checking DDA/SWATH Mode: ";
            LOG(ERROR) << "\t->" << e.what();
        } catch(...) {
            LOG(ERROR) << "Unknown error checking DDA/SWATH Mode. Default to DDA...";
        }
    }

    //---create parameters for peak picking
    mzPeakFinderUtils::PeakPickerParams p;
    // overriding ppm
    //p.ppm = ppm;

    clock_t beginTime = clock();

    try {
        if (noLoss) {
            //LOG(INFO) << "No-loss mode encoding: all ms Mz-64, all ms Int-64";
            writer.writeNoLossMzDB(outputFileName, cycleRange, nbCycles, p);
        } else {
            if (false) { // If msInstrument only good at ms1
                //LOG(INFO) << "ms1 Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzMs1Hi(outputFileName, cycleRange, nbCycles, p);
            } else {
                //LOG(INFO) << "all ms Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzHi(outputFileName, cycleRange, nbCycles, p);
            }
        }
        
        LOG(INFO) << "Checking run slices numbers";
        // check if run slice numbers are sorted according to ms_level and begin_mz
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
