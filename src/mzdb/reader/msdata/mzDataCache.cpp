#include "mzDataCache.h"


namespace mzdb {
	using namespace std;
    using namespace pwiz::msdata;

     mzDataCache::mzDataCache() {
			if (!db.tune_compressor(new kyotocabinet::LZOCompressor<kyotocabinet::LZO::RAW > ()))
				printf("Can not set a compressor...Don't know why !\n");

			if (!db.tune_options(kyotocabinet::HashDB::TCOMPRESS)) {
				printf("Can not compress each record don't why...\n");
			}
		}

     mzDataCache::mzDataCache(string& f) : filename(f) {}//unicity guaranteed

     mzDataCache::~mzDataCache() {
			db.close();
			printf("Deleting temporary files...");
			if (remove("-") != 0)
				printf("Error, can not delete file...\n");
			else
				printf("Done\n");
		}

      void mzDataCache::addKeyValue(const string& s, SpectrumPtr& spec) {
			vector<double>& mz = spec->getMZArray()->data;
			vector<double>& intensity = spec->getIntensityArray()->data;
			db.set((s + "mz").c_str(), (s + "mz").size() * sizeof (char), (char*) &mz[0], mz.size() * sizeof (double));
			db.set((s + "int").c_str(), (s + "int").size() * sizeof (char), (char*) &intensity[0], intensity.size() * sizeof (double));
		}

     void mzDataCache::getValue(const string& s, SpectrumPtr& spec) {
			//printf("hola\n");
			size_t* size = (size_t*) malloc(sizeof (size_t));
			size_t* size_ = (size_t*) malloc(sizeof (size_t));
			double* mz = (double*) (db.get((s + "mz").c_str(), (s + "mz").size() * sizeof (char), size));
			double* intensity = (double*) (db.get((s + "int").c_str(), (s + "int").size() * sizeof (char), size_));

			vector<double> mzData(mz, mz + (*size) / sizeof (double));
			vector<double> intData(intensity, intensity + (*size_) / sizeof (double));
			free(size);
			free(size_);
			delete [] mz;
			delete [] intensity;
            //printf("data get size: %d", mzData.size());
			spec->setMZIntensityArrays(mzData, intData, CVID_Unknown);
		}
}
