#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <share.h>

#include "getopt_pp.h"
#include "mzDBConverter.h"


using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

inline static double unifRand(double a, double b) {
    return (b - a)*(rand() / double(RAND_MAX)) + a;
}

int main(int argc, char* argv[]) {

    string help = "\n\nusage: regionExtractorPwiz.exe -i filepath <parameters> \ni, input: mz5 filepath\n o, ouput: format of the output: mzXML, mzML, mzData, all open formats supported by pwiz";
    string filename = "";
    double rtOffset = 60;

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('t', "rtOffset", rtOffset);

    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {

        //cout << help << endl;
        exit(1);
    }

    if (filename == "") {
        cout << help <<endl;
        exit(1);
    }

    MSDataFile msdatafile(filename);
    mzConverter conv("hola", 0);

    clock_t beginTime_ = clock();

    double minmz = 300;
    double rtmin = 0;
    int i = 0;
    while (i  < 100) {
        double rtmax = rtmin + rtOffset;
        conv.regionExtractorPwizImpl(msdatafile, filename, mz, mz+5, rtmin, rtmax);
        minmz += 5;
        rtmin += rtOffset;
        ++i;
    }


    clock_t endTime_ = clock();
    cout << ((double) endTime_ - beginTime_) / CLOCKS_PER_SEC << endl;

    //printf("See ya !\n");
}

