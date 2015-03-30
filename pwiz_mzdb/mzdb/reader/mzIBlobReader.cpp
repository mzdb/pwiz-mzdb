#include "mzIBlobReader.hpp"
//#include <iostream>

namespace mzdb {
namespace IBlobReader {

using namespace std;

int buildMapPositions(byte* buf,
                            int size,
                            map<int, vector<int> >& scansInfos,
                            map<int, DataEncoding>& dataEncodings) {

    int count = 1;
    int i = 0;
    while (i < size) {

        int startPos = i;

        int id = get_2<int>(i, buf, size);
        i += 4; //skip id;


        int nPeaks = get_2<int>(i, buf, size); //nbPeaks
        i += 4;//skip nbPeaks


        DataEncoding de;

        try {
            de = dataEncodings.at(id);
        } catch (exception& e){
            cout << e.what() << endl;
            cout <<  "Can not catch data encoding for scan id:" << id <<"\n";
        }

        int pe = (int) de.mode;//peakEncoding;
        int structSize = pe;

//        if (de.mode == FITTED) {
//            structSize += 2 * sizeof(float);
//        }


        scansInfos[count].push_back(id);
        scansInfos[count].push_back(startPos);
        scansInfos[count].push_back(nPeaks);
        scansInfos[count].push_back(pe);
        scansInfos[count].push_back((int) de.mode);
        scansInfos[count].push_back(structSize);

        i += nPeaks * structSize; //skip nbPeaks * size of one peak
        count++;
    }
    return count - 1;
}

void readData(byte* buf,
                    int size,
                    int idx,
                    mzScan* s,
                    map<int, vector<int> >& scansInfos) {

    //unpackinformation
    int startPos = scansInfos[idx][1];
    int nPeaks = scansInfos[idx][2];
    int pe = scansInfos[idx][3];
    int modeAsInt = scansInfos[idx][4];
    int structSize  = scansInfos[idx][5];

    bool isFitted = (modeAsInt == (int) FITTED);

    //                /if (isFitted) cout << "fitted, "<< (int) pe << ", "<< structSize <<endl;

    int nbBytes = nPeaks * structSize; //(int)mode;

    vector<double>& mz = s->mz;
    vector<double>& intensities = s->intensities;
    vector<float>& lwhm = s->lwhm;
    vector<float>& rwhm = s->rwhm;

    switch (pe) {
    case (int)NO_LOSS_PEAK: {
        //printf("NoLossPeaks\n");
        for (int i = startPos + 8; i < startPos + 8 + nbBytes; i += structSize) {
            mz.push_back(get_2<double> ( i, buf, size ));
            intensities.push_back(get_2<double> ( i + 8, buf, size ));
            if (isFitted){
                lwhm.push_back(get_2<float>( i + 16, buf, size ));
                rwhm.push_back(get_2<float>( i + 20, buf, size ));
            }
        }
        break;
    }
    case 20:  { //(int)HIGH_RES_PEAK: {
        //printf("HighResPeaks\n");
        for (int i = startPos +  8; i < startPos + 8 + nbBytes; i += structSize) {
            mz.push_back(get_2<double> ( i, buf, size ));
            intensities.push_back(static_cast<double>(get_2<float> ( i + 8, buf, size )));
            if (isFitted){
                lwhm.push_back(get_2<float>( i + 12, buf, size ));
                rwhm.push_back(get_2<float>( i + 16, buf, size ));
            }
        }
        break;
    }
    case 12: { //(int)LOW_RES_PEAK: {
        //printf("LowResPeaks\n");
        for (int i = startPos + 8; i < startPos + 8 + nbBytes; i += structSize) {
            mz.push_back(static_cast<double>(get_2<float> ( i, buf, size )));
            intensities.push_back(static_cast<double>(get_2<float> ( i + 4, buf, size)));
            if (isFitted){
                lwhm.push_back(get_2<float>( i + 8, buf, size ));
                rwhm.push_back(get_2<float>( i + 12, buf, size ));
            }
        }
        break;
    }
    }//end switch*/
}
}

}
