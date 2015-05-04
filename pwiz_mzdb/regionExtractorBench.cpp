#include <algorithm>
#include <cstdio>
#include <stdio.h>
#include <share.h>

#include "getopt_pp.h"
#include "mzdb/reader/mzDBReader.h"


using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

inline static double unifRand(double a, double b) {
    return (b - a)*(rand() / double(RAND_MAX)) + a;
}

int main(int argc, char* argv[]) {

    string help = "\n\nusage: rtreeBench.exe -i filepath <parameters> \ni, input: mzDB filepath\n o, ouput: format of the output: mzXML, mzML, mzData, all open formats supported by pwiz";
    string filename = "";
    double rtOffset = 60;
    double rtmin = 300;

    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);


    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {
        cout << help << endl;
        exit(1);
    }

    if (filename == "") {
        cout << help <<endl;
        exit(1);
    }

	try {
		MzDBFile file(filename);
        mzDBReader reader(file);

		clock_t beginTime_ = clock();
		size_t count = 0;
		double minmz = 300;
		int i = 0;
		while (i  < 100) {
			double rtmax = rtmin + rtOffset;
			vector<mzScan*> s;
            reader.extractRegion(minmz, minmz+5, rtmin, rtmax, 1, s);

			for (size_t k = 0; k < s.size(); k++) {
				count += s[k]->mz.size();
				delete s[k];
			}
			
			minmz += 5;
			rtmin += rtOffset;
			++i;
		}
		
		clock_t endTime_ = clock();
		cout << ((double) endTime_ - beginTime_) / CLOCKS_PER_SEC << endl;
	} catch (exception& e) {
		cout << e.what() << endl;
	}

    printf("See ya !\n");
}
