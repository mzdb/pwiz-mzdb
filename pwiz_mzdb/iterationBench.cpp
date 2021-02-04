#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <share.h>

#include "getopt_pp.h"
#include "mzdb/reader/mzDBReader.h"
#include "MzDBFile.h"

using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

int main(int argc, char* argv[]) {

    string help = "\n\nusage: iterationBench.exe -i filepath\n";
    string filename = "";

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);

    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {
        cout << help << endl;
    }

    if (filename == "") {
        cout << help <<endl;
        exit(1);
    }

    MzDBFile file(filename);
    try {
        mzDBReader reader(file);
        reader.enumerateSpectra();
    } catch (exception &e) {
       std::cerr << "Exception caught" << e.what();//LOG(ERROR)
    } catch(...) {
		std::cerr << "Unknown exception happened";//LOG(ERROR)
    }
}

