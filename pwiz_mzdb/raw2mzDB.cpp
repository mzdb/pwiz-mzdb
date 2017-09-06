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
#include "mzdb/writer/version.h"

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
        if (remove(filename.c_str()) != 0) {
            LOG(ERROR) << "File " << filename << " already exists and is locked, exiting...";
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

void parseResolutions(string& resolutionAsString, map<int, double>& resolutions, const string& help) {
    if(resolutionAsString != "" && toLower(resolutionAsString) != "auto") { // if auto, resolutions will be computer later (after opening raw file)
        // resolutionAsString must be like "40000-30000"
        vector<string> splitted;
        boost::split(splitted, resolutionAsString, boost::is_any_of("-"));
        try {
            for(int i = 0; i < splitted.size(); i++) {
                resolutions[i+1] = boost::lexical_cast<double>(splitted[i]);
            }
        } catch (boost::bad_lexical_cast &) {
            LOG(ERROR) << "Error, resolution values must be integers";
            std::cout << help << endl;
            exit(EXIT_FAILURE);
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
    // returns something like: 2017-01-04 13:08:59.105156500 +000
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

string getBuildName(string date) {
    // return something like: raw2mzDB_<soft_version_str>_build<yyyymmdd>
    std::stringstream name;
    name << "raw2mzDB_" << SOFT_VERSION_STR << "_build" << date.substr(0,4) << date.substr(5,2) << date.substr(8,2);
    return name.str();
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
            // MS_Bruker_Agilent_YEP_format generates a runtime error !
            } else if(fileExists(inputFileName + "/Analysis.yep", false, help)) {
                // inputFileName = inputFileName + "/Analysis.yep"; // Bruker file format
                LOG(ERROR) << "Bruker YEP format is not supported. Exiting.";
                exit(EXIT_FAILURE);
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
        outputFileName = outputFileName.substr(0, outputFileName.length() - 13); // 13 instead of 12 because we want to remove the slash/antislash character too
    }
    // if user gave a ".mzDB.tmp" extension, remove it because it will be added anyway
    if(std::regex_match(toLower(outputFileName), std::regex(".*.mzdb.tmp$"))) {
        outputFileName = outputFileName.substr(0, outputFileName.length() - 9);
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
    // also delete the mzDB.tmp file that could only exists if the previous run has failed
    if (deleteIfExists(outputFileName + ".tmp") == 1) { exit(EXIT_FAILURE); }
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
    //float bbWidth = 15, bbWidthMSn = 0; //15
    //float bbHeight = 5, bbHeightMSn = 10000; //5
    float bbWidth_default_dda = 15, bbWidthMSn_default_dda = 0, bbHeight_default_dda = 5, bbHeightMSn_default_dda = 10000;
    float bbWidth_default_dia = 60, bbWidthMSn_default_dia = 75, bbHeight_default_dia = 5, bbHeightMSn_default_dia = 20;
    float bbWidth = -1, bbWidthMSn = -1, bbHeight = -1, bbHeightMSn = -1;
    
    // MS resolution (only used for AB Sciex centroid computation so far)
    string resolutionStr = "";
    map<int, double> resolutions;

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
                  "\t-o, --output : specify the output filename\n"
                  "\t-c, --centroid : centroidization, eg: -c 1 (centroidization msLevel 1) or -c 1-5 (centroidization msLevel 1 to msLevel 5) \n"
                  "\t-p, --profile : idem but for profile mode \n"
                  "\t-f, --fitted : idem buf for fitted mode \n"
                  //"\t--ppm : ppm parameter for peak detection algorithm of WIFF files, default: 20\n"
                  "\t-T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s for DDA, 60s for DIA\n"
                  "\t-t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s for DDA, 75s for DIA\n"
                  "\t-M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da for DDA and DIA\n"
                  "\t-m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da for DDA, 20Da for DIA\n"
                  "\t-a, --acquisition : dda, dia or auto (converter will try to determine if the analysis is DIA or DDA), default: auto\n"
                  "\t-r, --resolution : resolution per msLevel or auto to compute resolution, eg: 40000-30000 for specific MS1 and MS2 resolutions, default is auto\n"
                  //"\t--bufferSize : low value (min 2) will enforce the program to use less memory (max 50), default: 3\n"
                  // "\t--max_nb_threads : maximum nb_threads to use, default: nb processors on the machine\n"
                  "\t--noLoss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)\n "
                  "\t--cycles : only convert the selected range of cycles, eg: 1-10 (first ten cycles) or 10- (from cycle 10 to the end) ; using this option will disable progress information\n"
                  "\t-s, --safeMode : use centroid mode if the requested mode is not available\n"
                  "\t--log : console, file or both (log file will be put in the same directory as the output file), default: console\n"
                  "\t-v, --version: display version information\n"
                  "\t-h --help : show help";

    const string buildDate = getBuildDate(argv[0]);
    std::stringstream version;
    // append: SQLite version, proteowizard release, proteowizard msdata 
    version << "\nVersion: " << getBuildName(buildDate)
            << "\n\n- Build date: '" << buildDate << "'"
            << "\n- Software version: " << SOFT_VERSION_STR
            << "\n- SQLite schema version: " << SCHEMA_VERSION_STR
            << "\n- SQLite version: " << SQLITE_VERSION
            << "\n- ProteoWizard release: " << pwiz::Version::str() << " (" << pwiz::Version::LastModified() << ")"
            << "\n- ProteoWizard MSData: " << pwiz::msdata::Version::str() << " (" << pwiz::msdata::Version::LastModified() << ")\n";

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('c', "centroid", centroid);
    ops >> Option('p', "profile", profile);
    ops >> Option('f', "fitted", fitted);
    ops >> Option('T', "bbTimeWidth", bbWidth);
    ops >> Option('M', "bbMzWidth", bbHeight);
    ops >> Option('t', "bbTimeWidthMSn", bbWidthMSn);
    ops >> Option('m', "bbMzWidthMSn", bbHeightMSn);
    ops >> Option('r', "resolution", resolutionStr);
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
    if (ops >> OptionPresent('v', "version")) {
        std::cout << version.str() << std::endl;
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

    // TODO allow the user to define another log file name, and just rename the log file into this name at the end
    // where should logs be written
    string logFile = "";
    if(boost::to_upper_copy(logToWhat) == "FILE" || boost::to_upper_copy(logToWhat) == "BOTH") {
        // write logs to a file in the same directory as the output file
        // Default log file format is based on this (cf. glog/logging.h.in:260)
        // Unless otherwise specified, logs will be written to the filename
        // "<program name>.<hostname>.<user name>.log.<severity level>.", followed
        // by the date, time, and pid (you can't prevent the date, time, and pid
        // from being in the filename).
        FLAGS_logtostderr = 0;
        // write to file and to console
        if(boost::to_upper_copy(logToWhat) == "BOTH") {
            FLAGS_alsologtostderr = 1;
        }
        // log file has the name of the output file and is stored in the output directory
        fs::path output(outputFileName);
        logFile = fs::absolute(output).parent_path().string() + "/" + output.stem().string(); // stem() returns the name of the file without its path and extension
        google::SetLogDestination(google::GLOG_INFO, logFile.c_str()); // first parameter means that all levels from FATAL to INFO will be put in the given file
        google::SetLogFilenameExtension("-"); // date, time, and pid will be appended to the extension (and this cannot be modified in the glog library)
    } else {
        // write logs to error output (GLOG does not write on standard output, but why ???)
        FLAGS_logtostderr = 1;
    }

    ////--- overriding default encoding to `fitted` for DIA mslevel 2
    //if(boost::to_upper_copy(acquisitionMode) == "DIA") {
    //    //dataModeByMsLevel[2] = FITTED;
    //
    //    if (bbHeightMSn != 10000 || bbWidthMSn != 0) {
    //        LOG(WARNING) << "Warning: 'bbMzWidthMSn' and/or 'bbRtWidthMSn' when 'dia' is set will be ignored !";
    //    }
    //
    //    // create bounding box of the same dimensions than MS1
    //    bbHeightMSn = bbHeight;
    //    bbWidthMSn = bbWidth;
    //
    //    //dataModeByMsLevel[3] = FITTED;
    //}

    //todo use set instead
    vector<int> modifiedIndex;

    parseRange(profile, PROFILE, dataModeByMsLevel, modifiedIndex, help);
    parseRange(fitted, FITTED, dataModeByMsLevel, modifiedIndex, help);
    parseRange(centroid, CENTROID, dataModeByMsLevel, modifiedIndex, help);
    
    parseResolutions(resolutionStr, resolutions, help);

    pair<int, int> cycleRange = parseCycleRange(cycleRangeStr);

    //--- Starting launching code
    // create a mzDBFile
    /// outputFileName must end with ".tmp" here (this ".tmp" will be removed at the end of the process)
    std::string outputFileNameTmp = outputFileName + ".tmp";
    MzDBFile f(filename, outputFileNameTmp, bbHeight, bbHeightMSn, bbWidth, bbWidthMSn, noLoss);

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

    mzDBWriter writer(f, msData, originFileFormat, dataModeByMsLevel, buildDate, resolutions, compress, safeMode);
    
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
    // set default values unless value is specified
    if(writer.getSwathAcquisition()) {
        if(bbWidth == -1) f.bbWidth = bbWidth_default_dia;
        if(bbWidthMSn == -1) f.bbWidthMsn = bbWidthMSn_default_dia;
        if(bbHeight == -1) f.bbHeight = bbHeight_default_dia; // abu use dda
        if(bbHeightMSn == -1) f.bbHeightMsn = bbHeightMSn_default_dia; // abu use dda
    } else {
        if(bbWidth == -1) f.bbWidth = bbWidth_default_dda;
        if(bbWidthMSn == -1) f.bbWidthMsn = bbWidthMSn_default_dda;
        if(bbHeight == -1) f.bbHeight = bbHeight_default_dda;
        if(bbHeightMSn == -1) f.bbHeightMsn = bbHeightMSn_default_dda;
    }

    //---print gathered informations
    std::stringstream summary;
    summary << "\n\n************ Summary of the parameters ************\n\n";
    summary << "Treating file: " << filename << " (" << (writer.getSwathAcquisition() ? "DIA" : "DDA") << " " << pwiz::msdata::cvTermInfo(originFileFormat).name << ")\n";
    //summary << "Acquisition mode : " << (writer.getSwathAcquisition() ? "DIA" : "DDA") << "\n";
    for (auto it = dataModeByMsLevel.begin(); it != dataModeByMsLevel.end(); ++it) {
        summary << "MS " << it->first << "\tSelected Mode: " << modeToString(it->second) << "\n";
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
    summary << "MS 1\t" << "m/z: " << f.bbHeight << " Da, retention time: " << f.bbWidth << "sec\n";
    summary << "MS n\t" << "m/z: " << f.bbHeightMsn << " Da, retention time: " << f.bbWidthMsn << "sec\n";

    summary << "\n";
    LOG(INFO) << summary.str();
    //end parsing commandline

    //---create parameters for peak picking
    mzPeakFinderUtils::PeakPickerParams p;
    // overriding ppm
    //p.ppm = ppm;

    clock_t beginTime = clock();

    try {
        if (noLoss) {
            //LOG(INFO) << "No-loss mode encoding: all ms Mz-64, all ms Int-64";
            writer.writeNoLossMzDB(outputFileNameTmp, cycleRange, nbCycles, p);
        } else {
            if (false) { // If msInstrument only good at ms1
                //LOG(INFO) << "ms1 Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzMs1Hi(outputFileNameTmp, cycleRange, nbCycles, p);
            } else {
                //LOG(INFO) << "all ms Mz-64, all ms Int-32 encoding";
                writer.writeMzDBMzHi(outputFileNameTmp, cycleRange, nbCycles, p);
            }
        }
        
        LOG(INFO) << "Checking run slices numbers";
        // check if run slice numbers are sorted according to ms_level and begin_mz
        writer.checkAndFixRunSliceNumberAnId();
        // close mzDB sqlite handler
        //writer.closeMzDbFile();
        // rename temp file into mzDB
        fs::path tempOutputPath(outputFileNameTmp);
        fs::path finalOutputPath(outputFileName);
        boost::filesystem::rename(tempOutputPath, finalOutputPath);
    } catch (exception& e) {
        LOG(ERROR) << e.what();
    } catch(...) {
        LOG(ERROR) << "Unknown error. Unrecoverable. Exiting...";
        exit(EXIT_FAILURE);
    }

    clock_t endTime = clock();
    LOG(INFO) << "Elapsed Time: " << ((double) endTime - beginTime) / CLOCKS_PER_SEC << " sec" << endl;
    
    /*
     * Piece of code written to rename the log file (if any)
     * The log file format is "<output_filename>-<yyyymmdd>-<hhmmss>.<pid>" and it's not convenient for Windows users so I just add ".txt"
     */ 
    try {
        if(FLAGS_logtostderr == 0) {
            google::SetLogDestination(google::GLOG_INFO, ""); // write logs to another file to unlock the current log file
            fs::path log(logFile);
            string pid = boost::lexical_cast<std::string>(GetCurrentProcessId());
            string searchTerm = log.filename().string()+".*"+pid;
            const std::regex filter(searchTerm.c_str());
            boost::filesystem::directory_iterator end_itr;
            for(boost::filesystem::directory_iterator i(log.parent_path()); i != end_itr; ++i) {
                // Skip if not a file
                if( !boost::filesystem::is_regular_file( i->status() ) ) continue;
                // rename if file matches the regex
                fs::path fd(i->path());
                if(std::regex_match(fd.filename().string().c_str(), filter)) {
                    fs::path renamedLogFile(fd.string() + ".txt");
                    //printf("Renaming log file '%s' to '%s'\n", i->path().string().c_str(), renamedLogFile.string().c_str());
                    boost::filesystem::rename(i->path(), renamedLogFile);
                }
            }
        }
    } catch(exception& e) {
        printf(e.what());
        printf("\n");
    } catch(...) {
        printf("Unknown error\n");
    }

    return 0;
}
