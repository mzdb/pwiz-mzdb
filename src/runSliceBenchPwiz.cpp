#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <share.h>

#include "getopt_pp.h"

#include "mzDBConverter.h"
#include "MzDBFile.h"


using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;



int main(int argc, char* argv[]) {

    string help = "\n\nusage: runSliceBench.exe -i filepath <parameters> \ni, input: mzDB filepath\n o, ouput: format of the output: mzXML, mzML, mzData, all open formats supported by pwiz";
    string filename = "";
    double mzmin = 0, mzmax = 0;

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('r', "mzmin", mzmin);
    ops >> Option('s', "mzmax", mzmax);


    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {

        //cout << help << endl;
        exit(1);
    }

    if (filename == "") {
        //cout << help <<endl;
        exit(1);
    }

    //cout << "\nSelected values :" << endl;


    MzDBFile file(filename);
    mzRegionExtractor extractor(&file);

    clock_t beginTime_ = clock();
    size_t count = 0;
    if (mzmin != 0 && mzmax != 0) {
        //cout << "mzmin : " << mzmin << endl;
        //cout << "mzmax : " << mzmax << endl;
        vector<mzScan*> s;
        extractor.runSliceExtraction(mzmin, mzmax, 1, s);
        for (size_t k = 0; k < s.size(); k++) {
            count += s[k]->mz.size();
            delete s[k];
        }
        //cout << "\r" << "data points found: " << count << flush;
    } else {
        //cout << "mzmin : random" << endl;
        //cout << "mzmax : random" << endl;

        srand(time(0));
        for (int i = 0; i < 100; ++i) {

            double mz = unifRand(300, 1995);
            vector<mzScan*> s;
            extractor.runSliceExtraction(mz, mz+5, 1, s);

            for (size_t k = 0; k < s.size(); k++) {
                count += s[k]->mz.size();

                delete s[k];
            }
            //cout << "\r" "#" << i + 1 << ", data points found: " << count << flush;
        }
    }
    clock_t endTime_ = clock();
    cout << ((double) endTime_ - beginTime_) / CLOCKS_PER_SEC << endl;

    //printf("See ya !\n");
}

