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
#include <share.h>
#include <iostream>
#include <fstream>

#include "mzdb/lib/getopt_pp/include/getopt_pp.h"
#include "mzdb/writer/mzDBWriter.hpp"
#include "mzdb/utils/MzDBFile.h"
#include "boost/algorithm/string.hpp"

#include "mzdb/lib/glog/glog/logging.h"

using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

/**
 * @param string filename
 * @return
 */
int deleteIfExists(const string &filename) {

    if (ifstream(filename.c_str())) {
        //file exist
        cout << "Found file with the same name" << endl;
        cout << "Delete...";
        if (remove(filename.c_str()) != 0) {
            LOG(ERROR) << "Error trying to delete file, exiting...May the file is opened elsewhere ?";
            exit(EXIT_FAILURE);
        } else
            cout << "Done" << endl;
    }
    return 0;
}

#ifdef _WIN32 
bool alreadyOpened(const string& filename) {

    FILE* stream;
    if ((stream = _fsopen(filename.c_str(), "at", _SH_DENYNO)) == NULL)
        return true;
    fclose(stream);
    return false;
}
#endif


inline bool exists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

void parseRange(string& range, DataMode mode, map<int, DataMode>& results, vector<int>& modifiedIndex, string& help) {
    if (range != "") {
        if (range.size() > 3) {
            LOG(ERROR) << "Error in parsing command line";
            cout << help << endl;
            exit(EXIT_FAILURE);
        }
        if (range.size() == 1) {
            int p;
            try {
                p = boost::lexical_cast<int>(range);
            } catch (boost::bad_lexical_cast &) {
                LOG(ERROR) << "Error, index must be an integer";
                cout << help << endl;
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
                cout << help << endl;
                exit(EXIT_FAILURE);
            }
            if (p1 && p2 && p1 <= p2) {
                for (int i = p1; i <= p2; ++i) {
                    results[i] = mode;
                    modifiedIndex.push_back(i);
                }
            } else {
                cout << help << "\n";
                LOG(ERROR) << "Error parameter 2 greater than parameter 1";
                exit(EXIT_FAILURE);
            }
        }
    }
}




int main(int argc, char* argv[]) {

    google::InitGoogleLogging("logger");
    google::SetStderrLogging(0);

    string filename = "", centroid = "", profile = "", fitted = "", namefile = "", serialization = "xml";
    bool compress = false;
    float bbWidth = 15, bbWidthMSn = 0; //15
    float bbHeight = 5, bbHeightMSn = 10000; //5
    bool noLoss = false; //32 bits intensity encoding by default
    int peakPickingAlgo = 0, nscans = 0, nbCycles = 3;

    map<int, DataMode> results;
    map<int, string> dataModes;

    //init results by default a mS1 is fitted others are centroided
    results[1] = FITTED;
    dataModes[1] = "PROFILE";
    dataModes[20] = "FITTED";
    dataModes[12] = "CENTROID";
    for (size_t i = 2; i <= MAX_MS; ++i) {
        results[i] = CENTROID;
    }

    string help = "\n\nusage: raw2mzDB.exe -input filename <parameters>"
                  "\n\nOptions:\n\n"
                  "\t-i, --input : specify the input rawfile path"
                  "\t-o, --output : specify the output filename (must be an absolute path)"
                  "\t-c, --centroid : centroidization, eg: -c 1 (centroidization msLevel 1) or -c 1-5 (centroidization msLevel 1 to msLevel 5) \n"
                  "\t-p, --profile : idem but for profile mode \n"
                  "\t-f, --fitted : idem buf for fitted mode \n"
                  "\t-T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s\n"
                  "\t-t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s\n:"
                  "\t-M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da \n"
                  "\t-m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da \n"
                  "\t--bufferSize : low value (min 2) will enforce the program to use less memory (max 50), default: 3"
                  "\t--no_loss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)\n "
                  "\t--nscans : nb scans to convert into the mzDB file (max: number of scans in the rawfile)"
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
    ops >> Option("bufferSize", nbCycles);
    if (ops >> OptionPresent('h', "help"))
        cout << help << endl;
    //ops >> Option('C', "compress", compression)
    if (ops.options_remain()) {
        LOG(ERROR) <<"Oops! Unexpected options. Refer to help";
        cout << help << endl;
        return 1;
    }

    if (! exists(filename)) {
        LOG(ERROR) << "Filepath does not exist.Exiting.";
        return 1;
    }

    if (!filename.size()) {
        LOG(ERROR) << "empty raw filename ! Exiting...";
        cout << help << endl;
        return 1;
    }

#ifdef WIN32
    if (alreadyOpened(filename)) {
        LOG(ERROR) << "File already open in another application.\nPlease close it to perform conversion.";
        return 1;
    }
#endif

    int res = deleteIfExists(filename + ".mzDB");
    if (res == 1)
        return 1;

    vector<int> modifiedIndex;

    parseRange(profile, PROFILE, results, modifiedIndex, help);
    parseRange(fitted, FITTED, results, modifiedIndex, help);
    parseRange(centroid, CENTROID, results, modifiedIndex, help);

    //check stuffs
    cout << "\nWhat I understood :" << endl;
    cout << "Treating: " << filename << endl;
    map<int, DataMode>::iterator it;
    for (it = results.begin(); it != results.end(); ++it) {
        cout << "ms " << it->first << " => selected Mode: " << dataModes[(int) it->second] << endl;
    }
    cout << endl;
#ifdef DEBUG
    cout << bbHeight<<", " << bbHeightMSn<< ", " <<bbWidth<< ", " << bbWidthMSn <<endl;
#endif

    MzDBFile f(filename, bbHeight, bbHeightMSn, bbWidth, bbWidthMSn);
    try {
        mzDBWriter writer(f, results, compress);
        mzPeakFinderUtils::PeakPickerParams p;
        if (noLoss) {
            LOG(INFO) << "no loss detected";
            writer.writeMzRTreeDB<double, double, double, double>(noLoss, namefile, nscans, nbCycles, p);
        } else {
            LOG(INFO) << "mz64 int32 detected";
            writer.writeMzRTreeDB<double, float, float, float>(noLoss, namefile, nscans, nbCycles, p);
        }
        LOG(INFO) << "check run slices numbers";
        writer.checkAndFixRunSliceNumberAnId();

    } catch (exception& e) {
        cout << e.what() << endl;
    }

    printf("See ya !\n");

    google::ShutdownGoogleLogging();

    return 0;
}
