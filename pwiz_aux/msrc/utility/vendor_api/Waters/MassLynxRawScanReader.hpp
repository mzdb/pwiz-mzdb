//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawScanReader.h
// DATE:			Nov 2018
// COPYRIGHT(C):	Waters Corporation
//
//-----------------------------------------------------------------------------------------------------
#pragma once

#include "MassLynxRawBase.hpp"

namespace Waters
{
namespace Lib
{
namespace MassLynxRaw
{
	using std::vector;

	class MassLynxRawScanReader : public MassLynxBaseReader
	{
	public:
		MassLynxRawScanReader(const string & strFullPath) : MassLynxBaseReader(strFullPath, MassLynxBaseType::SCAN) {}
		MassLynxRawScanReader(const MassLynxRawScanReader& massLynxScanReader) : MassLynxBaseReader(massLynxScanReader, MassLynxBaseType::SCAN) {}
		MassLynxRawScanReader(const MassLynxBaseReader& massLynxRawReader) : MassLynxBaseReader(massLynxRawReader, MassLynxBaseType::SCAN) {}

		/** @example MassLynxRawExample.cpp
		* This is an example of how to use the Example_Test class.
		* More details about this example.
		**/


		/**
		*  Reads the masses and intensities for the requested function and scan
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*   @param [out] masses vector of masses
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadScan(const int& nWhichFunction, const int& nWhichScan, vector<float> & masses, vector<float> & intensities)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readScan(GetRawReader(), nWhichFunction, nWhichScan, &pMasses, &pIntensities, &nSize));

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);
		}

		/**
		*  Reads the masses, intensities and flags for the requested function and scan
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*   @param [out] masses vector of masses
		*   @param [out] intensities vector of intensities
		*   @param [out] flags vector of flags
		*
		*   @return void
		*/
		void ReadScan(const int& nWhichFunction, const int& nWhichScan, vector<float> & masses, vector<float> & intensities, vector<char> & flags)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); char* pFlags(NULL); int nSize(0);
			CheckReturnCode(readScanFlags(GetRawReader(), nWhichFunction, nWhichScan, &pMasses, &pIntensities, &pFlags, &nSize));

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);
			ToVector(pFlags, nSize, flags);
		}

		void ReadScan(const int& nWhichFunction, const int& nWhichScan, vector<float> & masses, vector<float> & intensities, vector<float> & productMasses)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); float* pProductMasses(NULL); int nSize(0); int nProductSize(0);
			CheckReturnCode(readDaughterScan(GetRawReader(), nWhichFunction, nWhichScan, &pMasses, &pIntensities, &pProductMasses, &nSize, &nProductSize));

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);
			ToVector(pProductMasses, nProductSize, productMasses);
		}

		/**
		*  Reads the masses and intensities for the requested function, scan and drift
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*   @param  nWhichDrift requested drift
		*   @param [out] masses vector of masses
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/

		void ReadScan(const int& nWhichFunction, const int& nWhichScan, const int& nWhichDrift, vector<float> & masses, vector<float> & intensities)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readDriftScan(GetRawReader(), nWhichFunction, nWhichScan, nWhichDrift, &pMasses, &pIntensities, &nSize));

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);
		}

		//void ExtractScan(const int& nWhichFunction, const int& nStartScan, const int& nEndScan, const int& nWhichDrift, vector<int> & masses, vector<float> & intensities)
		//{
		//	// get the data..
		//	int* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
		//	CheckReturnCode(extractScan(GetRawReader(), nWhichFunction, nStartScan, nEndScan, nWhichDrift, &pMasses, &pIntensities, &nSize));

		//	// fill the vector
		//	ToVector(pMasses, nSize, masses);
		//	ToVector(pIntensities, nSize, intensities);
		//}
	};

	namespace Extended
	{
		class MassLynxRawScanReader : public Waters::Lib::MassLynxRaw::MassLynxRawScanReader
		{
		public:
			using Waters::Lib::MassLynxRaw::MassLynxRawScanReader::ReadScan;

			MassLynxRawScanReader(const string & strFullPathName) : Waters::Lib::MassLynxRaw::MassLynxRawScanReader(strFullPathName) {}
			MassLynxRawScanReader(const MassLynxRawScanReader& massLynxScanReader) : Waters::Lib::MassLynxRaw::MassLynxRawScanReader(massLynxScanReader) {}
			MassLynxRawScanReader(const MassLynxBaseReader& massLynxRawReader) : Waters::Lib::MassLynxRaw::MassLynxRawScanReader(massLynxRawReader) {}

			void ReadScan(const int& nWhichFunction, const int& nWhichScan, const int& nWhichDrift, vector<int>& masses, vector<float>& intensities)
			{
				// get the data..
				int* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
				CheckReturnCode(readDriftScanIndex(GetRawReader(), nWhichFunction, nWhichScan, nWhichDrift, &pMasses, &pIntensities, &nSize));

				// fill the vector
				ToVector(pMasses, nSize, masses);
				ToVector(pIntensities, nSize, intensities);
			}

			void ReadScan(const int& nWhichFunction, const int& nWhichScan, const int& nWhichDrift, vector<int>& masses, vector<float>& intensities, vector<char>& flags)
			{
				// get the data..
				int* pMasses(NULL); float* pIntensities(NULL); char* pFlags(NULL); int nSize(0);
				CheckReturnCode(readDriftScanFlagsIndex(GetRawReader(), nWhichFunction, nWhichScan, nWhichDrift, &pMasses, &pIntensities, &pFlags, &nSize));

				// fill the vector
				ToVector(pMasses, nSize, masses);
				ToVector(pIntensities, nSize, intensities);
				ToVector(pFlags, nSize, flags);
			}

			void GetMassScale(const int& nWhichFunction, const int& nWhichScan, vector<float>&masses)
			{
				// get the data
				int nSize(0); float* pMasses(NULL);
				CheckReturnCode(getDriftMassScale(GetRawReader(), nWhichFunction, nWhichScan, &pMasses, &nSize));

				// fill the vector
				ToVector(pMasses, nSize, masses);
			}
		};
	}
}   // MassLynxRaw
}   // Lib
}   // Waters

