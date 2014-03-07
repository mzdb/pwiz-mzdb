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

    string help = "\n\nusage: iterationBench.exe -i filepath <parameters> \ni, input: mzDB filepath\n";

    string filename = "", format = "mzML";
    //bool noLoss = false;

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);

    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {
        //printf("Oops! Unexpected options. Refer to help\n");
        cout << help << endl;
    }

    if (filename == "") {
        cout << help <<endl;
        exit(1);
    }

    MzDBFile file(filename);
    try {
        mzConverter converter(format, &file);
        converter.enumerateSpectra();
    } catch (exception &e) {
        cout <<"Exception caught" << e.what() << endl;
    }
}

