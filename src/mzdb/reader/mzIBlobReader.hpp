#ifndef __MZ_BLOB_READER__
#define __MZ_BLOB_READER__

#include "../utils/mzUtils.hpp"
#include "msdata/mzScan.hpp"

namespace mzdb {
    namespace IBlobReader {

		using namespace std;

        template<typename T>
        inline T get_2(size_t index, byte* buffer, size_t size) {
			if(index + sizeof(T) <= size)
				return *((T*) & buffer[index]);
			return 0;
        }
		
		/** scanInfos id, startPos, nbPeaks, peakEncoding, DataMode (asInt), structSize*/
         int buildMapPositions(byte* buf,
                               size_t size,
                               map<int, vector<int> >& scansInfos,
                               map<int, DataEncoding>& dataEncodings);

		
         void readData(byte* buf,
                       size_t size,
                       int idx,
                       mzScan* s,
                       map<int, vector<int> >& scansInfos);
		
	}//end namespace blobreader
}//end namespace mzdb
#endif
