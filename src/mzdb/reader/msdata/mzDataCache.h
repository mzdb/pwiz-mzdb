#ifndef __MZDATACACHE__
#define __MZDATACACHE__

#include <stdio.h>

#include "pwiz/data/msdata/MSData.hpp"

#include "../../lib/kyoto_cabinet/kchashdb.h"
#include "../../utils/mzUtils.hpp"

namespace mzdb {
	using namespace std;

    class mzDataCache {

		enum OpenPolicy {
			OCREATE,
			OWRITER,
			OREADER
		};

		enum CompressorPolicy {
			NO_COMPRESSOR,
			LZO_COMPRESSOR,
			LZMA_COMPRESSOR,
            ZLIB_COMPRESSOR
		};

        struct CacheConfig {
			OpenPolicy openPolicy;
			CompressorPolicy compressorPolicy;
		};

		kyotocabinet::HashDB db;
		const string filename;



	public:

         mzDataCache();

         mzDataCache(string& f);

         ~mzDataCache();

         inline void open() {db.open("-", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);}
         inline void clear() {db.clear();}
         inline size_t count() {return db.count();}

         void addKeyValue(const string& s, pwiz::msdata::SpectrumPtr& spec);

         void getValue(const string& s, pwiz::msdata::SpectrumPtr& spec);
	};

}
#endif
