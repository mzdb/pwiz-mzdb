#include "fittedToProfile.h"
#include "mzSpectrumListCache.h"

#define MAX_POINT 4.0
#define INT_RELATIVE_THRESH 0.001
#define HEIGHT_DIV_SIGMA_FACT 3.3972872
#define SIX_DIV_SIGMA_FACT 2.5479654
#define FOUR_DIV_SIGMA_FACT 1.6986436
#define TWO_DIV_SIGMA_FACT 0.8493218

namespace mzdb {

using namespace std;
using namespace pwiz::msdata;



void mzDBMSDataTomzMLMSData(MzDBFile* f, MSDataPtr mzdb, MSDataPtr raw) {

    //out = mzdb; //to catch all metadata
    ofstream fileHandle("ErrorPercentage.txt", ios::out | ios::trunc);
    //ofstream fileHandle_2("ErrorNbPoints.txt", ios::out | ios::trunc);
    FILE* fileHandle_2 = fopen("ErrorNbPoints.txt", "w");
    FILE* fileHandle_3 = fopen("ymax.txt", "w");


    if ( ! fileHandle || ! fileHandle_2)
        printf("Failed to create ratiosPercentage file\n");

    mzDataCache* cache = new mzDataCache;
    cache->open();

    int lastPercent = 0;

    //BinnedSpectrum::getMzDataInterval(300.,  2001, 0.);
    //printf("\ninterval vector size : %d\n", BinnedSpectrum::mzData.size());

    SpectrumListPtr spec = mzdb->run.spectrumListPtr;
    SpectrumListPtr rawSpec = raw->run.spectrumListPtr;

    for (size_t i =  0; i < spec->size(); ++i) {
        SpectrumPtr ptr = spec->spectrum(i, true);
        SpectrumPtr rawSpectrum = rawSpec->spectrum(i, true);

         //if( i != 1 ) { continue; }

        mzScan* scan = (mzScan*)(ptr.get());

        if (! scan) {
            printf("Empty pointer\n");
            exit(0);
        }
        vector<double>& mz = scan->mz;
        vector<double>& intens = scan->intensities;
        vector<float>& lwhm = scan->lwhm;
        vector<float>& rwhm = scan->rwhm;



        /*printf("%d\n", mz.size());
        for (size_t j = 0; j < mz.size(); ++j) {
            printf("%f\n", mz[j]);
        }
        printf("\n");*/

        if ( scan->encoding.mode == FITTED) {
            BinnedSpectrum binnedSpectrum(rawSpectrum);
            //printf("size of mzdata:%d\n", binnedSpectrum.mzData.size());
            vector<double> intData(binnedSpectrum.mzData.size(), 0);

            for (size_t j=0; j < mz.size(); ++j) {
                double y_max = intens[j];
                double x_zero = mz[j];

                fprintf( fileHandle_3, "%.5f\t%f\n", x_zero,y_max);
                //printf("max value: %f\n", y_max);

                // add the apex
                size_t index = binnedSpectrum.getLeftIndex(x_zero);
                //get the limit of ppm
                double ppmLimit = 1.1 * sqrt(x_zero);

                double maxDistFromTheApex = (x_zero * (ppmLimit / 2)) / 1e6;
                double mzLeftLimit = x_zero - maxDistFromTheApex;
                double mzRightLimit = x_zero + maxDistFromTheApex;


                //binnedSpectrum.putFromIndex(index, y_max);
                //intData[index] +=  mzMath::y(binnedSpectrum.mzData[index], x_zero,  y_max, sigmal_squared);// y_max;

                double lwhm_j = static_cast<double>(lwhm[j]);
                double rwhm_j = static_cast<double>(rwhm[j]);
                //generate x value;
                //printf("%f, %f, %f, %f\n", mz[j], intens[j], lwhm[j], rwhm[j]);

                //if ( lwhm_j ) {
                double sigmal = (2.0 * lwhm_j) / SIGMA_FACTOR;
                //99% of the value 6 sigma goes to 3/4 sigmas up and down
                double sigmal_x6 =  sigmal * 2;
                double min_x_left  = x_zero - sigmal_x6;
                size_t index_left = index;//(index - 1 >= 0) ? index - 1 : 0;
                double sigmal_squared = sigmal * sigmal;
                double x_zero_pl = binnedSpectrum.mzData[index_left];//binnedSpectrum.getPairFromIndex(index_left).first;//
                double y_vall =  mzMath::y(x_zero_pl, x_zero,  y_max, sigmal_squared);
                bool makeBounds = false;
                if (y_vall < 6e3)
                    makeBounds = true;

                //visitedLeftIndexes.push_back(index);

                //size_t counter = 0;
                while (  x_zero_pl > min_x_left ) {

                    if (x_zero_pl < mzLeftLimit && makeBounds)
                        break;
                    //binnedSpectrum.putFromIndex(index_left, y_vall);
                    intData[index_left] += y_vall;
                    /* calculate values for next step */
                    index_left--;//-= 1; //= index_left - 1 >= 0 ? index_left - 1 : 0;
                    x_zero_pl = binnedSpectrum.mzData[index_left];//binnedSpectrum.getPairFromIndex(index_left).first;//
                    y_vall =  mzMath::y(x_zero_pl, x_zero,  y_max, sigmal_squared);
                    //counter++;
                }


                //do not padd with zero if bin already exists
                //binnedSpectrum.getOrInitIntensityFromIndex(index_left);
                //do nothing, the tested pair is initialized to zero
                // during the test
                /*} else {
                    binnedSpectrum.getOrInitIntensityFromIndex(index - 1);
                }*/

                //if ( rwhm_j ) {
                double sigma = (2.0 * rwhm_j) / SIGMA_FACTOR;
                double sigma_x6 = sigma * 2; //99% of the value
                double min_x_right = x_zero + sigma_x6;
                size_t index_right = index + 1 ;
                double sigma_squared = sigma * sigma;
                double x_zero_p = binnedSpectrum.mzData[index_right];//binnedSpectrum.getPairFromIndex(index_right).first;//
                double y_val =   mzMath::y(x_zero_p, x_zero, y_max, sigma_squared);
                //size_t counter = 0;
                //printf("%f, %f, %d, %d\n", x_zero_p, min_x_right, index_right, binnedSpectrum.binnedSpectrum.size());


                while ( x_zero_p < min_x_right) {
                    if (x_zero_p > mzRightLimit && makeBounds)
                        break;
                    intData[index_right] += y_val;
                    index_right++;
                    x_zero_p = binnedSpectrum.mzData[index_right];//binnedSpectrum.getPairFromIndex(index_right).first;//
                    y_val =  mzMath::y(x_zero_p, x_zero, y_max, sigma_squared);
                    //counter++;
                }

                //do not padd with zero if bin already exists
                //actually it is never supposed to happen since mz is increasing
                //so automatically create an entry and set it to 0
                //in some case (i suppose...) a previous peak could be larger
                //that is a reason to not reset the intensity to 0 too
                //if (! binnedSpectrum.getIntensityFromIndex(index_right))
                //binnedSpectrum.getOrInitIntensityFromIndex(index_right);
                /*} else {
                    binnedSpectrum.getOrInitIntensityFromIndex(index + 1);
                }*/
            }

            //pair<vector<double>, vector<double> > data = binnedSpectrum.getData();
            //printf("%d, %d\n", intData.size(), binnedSpectrum.mzData.size());
            ptr->setMZIntensityArrays(binnedSpectrum.mzData, intData,  CVID_Unknown);

            //here binning
            /*---comparing data intensities*/
            //printf("binnedSpectrumSize:%d, rawSize:%d\n", data.first.size(), rawSpectrum->getMZArray()->data.size());

            //bin
            //int nbBins = (int) (1700.0 / 0.1);
            map<int, vector<pair<double, double> > > rawIntensitiesSumByBinIndex, mzdbIntensitiesSumByIndex;
            vector<MZIntensityPair> rawMzIntensities;
            rawSpectrum->getMZIntensityPairs(rawMzIntensities);
            for (size_t j = 0; j < rawMzIntensities.size(); ++j) {
                const MZIntensityPair& pair = rawMzIntensities[j];
                int idx = (int) (pair.mz / 1);
                rawIntensitiesSumByBinIndex[idx].push_back(make_pair(pair.mz, pair.intensity));
            }

            for (size_t j= 0; j < intData.size(); ++j) {
                const double& mz = binnedSpectrum.mzData[j];
                const double& intensity = intData[j];
                int idx = (int) (mz / 1);
                mzdbIntensitiesSumByIndex[idx].push_back(make_pair(mz, intensity));
            }

            //iterate on raw map
            double counter = 0;
            double sum = 0;
            for (auto it = rawIntensitiesSumByBinIndex.begin(); it != rawIntensitiesSumByBinIndex.end(); ++it) {
                const int& idx = it->first;
                const vector<pair<double, double> >& vec = it->second;
                double integratedIntensity = 0;
                if (! vec.empty()) {
                    integrate(vec, integratedIntensity);
                }
                if (integratedIntensity) {
                    if (mzdbIntensitiesSumByIndex.find(idx) != mzdbIntensitiesSumByIndex.end()) {
                        const vector<pair<double, double> >& mzdbPoints = mzdbIntensitiesSumByIndex[idx];
                        //printf("v1 %f, %f\n", intensity, mzdbIntensity);
                        double mzDBIntegratedIntensity = 0;
                        integrate(mzdbPoints, mzDBIntegratedIntensity);
                        double error = (integratedIntensity - mzDBIntegratedIntensity) / integratedIntensity;
                        sum += error;
                        counter ++;
                    }
                }
            }
           //printf("%f, %f\n", sum, counter);
            /*if (scan->idMzDB == 5524) {
                printf("Scan 5524:%d, reconstructed profile:%d, raw profile:%d\n", scan->mz.size(), data.first.size(), rawSpectrum->getMZArray()->data.size());
            }*/
            fileHandle << (sum / counter) * 100.0 << endl;

            if (rawSpectrum->index == 1) {
                auto& rawData = rawSpectrum->getIntensityArray()->data;

                for (size_t kk = 0; kk < binnedSpectrum.mzData.size(); ++kk) {
                    //fileHandle_2 << binnedSpectrum.mzData[kk] << "\t" << rawData[kk] << "\t" << intData[kk] << endl;
                    fprintf(fileHandle_2, "%.4f\t%f\t%f\n", binnedSpectrum.mzData[kk] ,rawData[kk] , intData[kk] );
                }
            }


            //fileHandle_2 << ( (rawNonZeroCount - mzdbNonZeroCount) / rawNonZeroCount ) * 100 << endl;

        } else {
            ptr->setMZIntensityArrays(mz, intens, CVID_Unknown);
        }
        cache->addKeyValue(boost::lexical_cast<string>(i), ptr);

        //clear data
        //clearScanData(scan);

        int newPercent = (int) (((float) (i) / spec->size() * 100));
        if (newPercent != lastPercent) {
            printProgBar(newPercent);
            lastPercent = newPercent;
        }



    }//end iteration mzdb spectra;
    printProgBar(100);
    fileHandle.close(); fclose(fileHandle_2);/*---do not forget to close the fileHandle*/
    fclose(fileHandle_3);
    SpectrumListPtr spectrumListCached(new mzSpectrumListCache(f, mzdb.get(), cache));
    mzdb->run.spectrumListPtr = spectrumListCached;
    ChromatogramListPtr chromListPtr(new mzEmptyChromatogram);
    mzdb->run.chromatogramListPtr = chromListPtr;
}

void clearScanData(mzScan* scan) {
    scan->mz.clear();
    scan->intensities.clear();
    scan->lwhm.clear();
    scan->rwhm.clear();
}


void integrate(const vector<pair<double, double> >& v, double& integratedIntensity) {
    double intensityArea = 0;
    double prevPeakMz= 0;
    double prevPeakIntensity = 0;
    double intensitySum = 0;

    for( size_t j=0; j < v.size(); ++j ) {

        const pair<double, double>& peak = v[j];
        intensitySum += peak.second;

        double curPeakMz = peak.first;

          if( j > 0 ) {
            double deltaMz = curPeakMz - prevPeakMz;
            intensityArea += (peak.second + prevPeakIntensity ) * deltaMz / 2;
          }

          prevPeakMz = curPeakMz;
          prevPeakIntensity = peak.second;
        }

        if( intensityArea == 0 )
          intensityArea = intensitySum;

        //intensity = intensitySum;
        integratedIntensity = intensityArea;
}


}//end namespace
