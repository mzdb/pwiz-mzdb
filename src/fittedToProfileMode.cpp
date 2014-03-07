#include "mzdb/fitted2Profile/fittedToProfile.h"
#include "mzdb/lib/getopt_pp/include/getopt_pp.h"
#include "mzdb/reader/mzDBReader.h"
#include "pwiz_tools/common/FullReaderList.hpp"

#include "pwiz/data/msdata/MSDataFile.hpp"

using namespace std;
using namespace GetOpt;
using namespace pwiz::msdata;

int main(int argc, char* argv[]) {
    string filename = "", output = "", rawFilename;
    GetOpt_pp ops(argc, argv);
    ops >> Option('i', "input", filename);
    ops >> Option('o', "output", output);
    ops >> Option('r', "origin", rawFilename);
	
	if (filename == "") {
		printf("Empty input... Exiting\n");
		exit(0);
	}
    if (output == "")
        output = filename + ".mzML";

    MSDataPtr in(new MSData);
    mzdb::MzDBFile file(filename);
    mzdb::mzDBReader reader(&file);
    printf("Read mzDB file...");
    reader.readMzRTreeDB(*in); //want a raw pointer
    printf("Done\n");

    printf("Read rawFile...");
    FullReaderList readers;
    vector<MSDataPtr> msdList;
    try {
        readers.read(rawFilename, msdList);
    } catch (exception& e) {
        cout << e.what() << endl;
    }
    printf("Done\n");

    try {
        printf("Start the conversion...");
        mzdb::mzDBMSDataTomzMLMSData(&file, in, msdList[0]);
        //printf("Done\n");
    } catch (exception& e) {
        cout << e.what() << endl;
    }

    try {
        MSDataFile::WriteConfig conf;
        conf.indexed = false;
        conf.format = MSDataFile::Format_mzXML;

        if (! file.isNoLoss()) {
            //printf("conversion options: mz 64 bits, intensity 32 bits mzML\n");
            conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
            conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
            conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_32;

        }
        else {
            //printf("conversion options: mz 64 bits, intensity 64 bits mzML\n");
            conf.binaryDataEncoderConfig.precision = BinaryDataEncoder::Precision_64;
            conf.binaryDataEncoderConfig.precisionOverrides[MS_m_z_array] = BinaryDataEncoder::Precision_64;
            conf.binaryDataEncoderConfig.precisionOverrides[MS_intensity_array] = BinaryDataEncoder::Precision_64;
        }
        printf("\nWriting...");
        MSDataFile::write(*in, reader.filename() + ".mzXML", conf, new pwiz::util::IterationListenerRegistry()); //if you do not specify any configuration, mzML by default
        printf("Done\n");
    } catch (exception &e) {
        cout << e.what() << endl;
    }
    return 0;
}
