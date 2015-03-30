#include <iostream>

#include "mzdb/lib/getopt_pp/include/getopt_pp.h"
#include "mzdb/converter/mzDBConverter.h"
#include "mzdb/utils/MzDBFile.h"

using namespace std;
using namespace mzdb;
using namespace GetOpt;
using namespace pwiz::msdata;

int main(int argc, char* argv[]) {
    
    string help = "\n\nusage: mzDB2mzML.exe -i filepath <parameters> \n\
                  i, input: mzDB filepath\n \
                  o, ouput: format of the output: mzXML, mzML, mzData, all open formats supported by pwiz";
    
    string filename = "", format = "mzML";
    bool noLoss = false;
        
    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('o', "output", format);
    ops >> OptionPresent("no_loss", noLoss);
    
    
    if (ops.options_remain() || ops >> OptionPresent('h', "help")) {
        //printf("Oops! Unexpected options. Refer to help\n");
        cout << help << endl;
    }
    
    if (filename == "") {
        cout << help <<endl;
        exit(1);
    }

    MzDBFile f(filename);
    try {
        mzConverter converter(&f);
        //printf("after creation mzCOnverter\n");
        converter.enumerateSpectra();
        //converter.convert(noLoss, mzConverter::MZ_XML);
    } catch (exception& e) {
        cout << "Exception:\n\t->" << e.what() <<endl;
    }

    //converter.enumerateSpectra();
    //converter.rtreeRequest();	
    printf("See ya !\n");
}


