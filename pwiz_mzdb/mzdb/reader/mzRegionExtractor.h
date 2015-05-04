#ifndef __MZREGIONEXTRACTOR__
#define __MZREGIONEXTRACTOR__


#include "mzUtils.hpp"


namespace mzdb {
	using namespace std;
    /**
     *implements RTREE extraction and runSlice based extraction of data points
     *WARNING: try to refactor but leads to a unknown segmentation fault...
     * Do  not need to free the mzdbfile ' database and statement pointer
     * @brief The mzRegionExtractor class
     */
    struct mzScan;
    struct MzDBFile;

	class PWIZ_API_DECL mzRegionExtractor {


        /**
         * Simple structure to describe a boundingbox
         * @brief The mzBB struct
         */
		 struct PWIZ_API_DECL mzBB {
			vector<byte> data;
			size_t size;
			int id, runSliceId, firstScanId;

            PWIZ_API_DECL mzBB(int i, vector<byte>& d, size_t s) : id(i), data(d), size(s) {}
		};

	private:

        static const char* sqlRunSlice, *sqlRTree, *sqlMSnRTree;
		bool isNoLoss;
        MzDBFile& _mzdb;

		map<int, int> msLevelByID;
		map<int, float> rtByID;
		map<int, DataEncoding> dataEncodings;

        /**
         *Fill meta informations about spectra, useful to reconstruct spectra
         *given bounding box
         * @brief fillInternalMaps
         */
        void fillInternalMaps();


	public:

        /**
         *Constructor
         * @brief mzRegionExtractor
         * @param filename: name of the database file
         */
        PWIZ_API_DECL mzRegionExtractor(MzDBFile&);


        /**
         *perform a rtree extraction given map coordinates and msLevel
         *@brief rTreeExtraction
         *@param minmz: minimum mass over charge
         *@param maxmz: maximum mass over charge
         *@param minrt: minimum retention time in seconds
         *@param maxrt: maximum retenstion time in seconds
         *@param msLevel: ms level to perform extraction, usually set to '1'
         *@param results: vector containing extracted scans
         */
        PWIZ_API_DECL void rTreeExtraction(double minmz, double maxmz, double minrt, double maxrt, int msLevel, vector<mzScan*> &results);

        /**
         *perform a runSlice extraciton given min and max mass over charge
         *the sql request is slightly different of the one used in the rtree
         *extraction and especially optimized for runSlice extraction
         *@brief runSliceExtraction
         *@param minmz: minimum mass over charge on the lcms map
         *@param maxmz: maximim mass over charge on the lcms map
         *@param msLevel: msLevel on which we want to perform the extraction
         *@param results: vector containing resulting scan
         */
        PWIZ_API_DECL void runSliceExtraction(double minmz, double maxmz, int msLevel, vector<mzScan*>& results);

	};
} //end namespace
#endif
