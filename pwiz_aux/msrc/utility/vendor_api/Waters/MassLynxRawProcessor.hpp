//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawProcessor.h
// DATE:			July 2018
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
	class MassLynxRawProcessor : public MassLynxBaseProcessor
	{
	public:
		MassLynxRawProcessor() : MassLynxBaseProcessor(MassLynxBaseType::CENTROID) {}

		/**
		Creates a new centroid raw file based on the current raw data file
		Source raw file must contain continuum data
		Exception will not be thrown, use GetLastError to retrieve error code
		@param strOutputPathName Path of new raw file
		@return true if successful
		*/
		bool Centroid(const string& strOutputPathName) const
		{
			return CheckReturnCode(centroidRaw(GetProcessor(), strOutputPathName.c_str()), false);
		}
	};

	/**
	Class that processes dda data returning MS1 and MS2 scans
	MS1 scan is the survey scan, MS2 is the associated fragment scan
	*/
	class MassLynxDDAProcessor : public MassLynxBaseProcessor
	{
	public:
		MassLynxDDAProcessor() : MassLynxBaseProcessor(MassLynxBaseType::DDA) {}

		/**
		Returns vectors of retention times and associated set masses
		@param [out] RTs scan retention time
		@param [out] setmasses
		@return void
		*/
		void GetIndex(vector<float>& RTs, vector<float>& setmasses)
		{
			// get the data..
			float* pRTs(NULL); float* pMasses(NULL); int nSize(0);
			CheckReturnCode(ddaRaw(GetProcessor(), &pRTs, &pMasses, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pRTs, nSize, RTs, true);
			ToVector(pMasses, nSize, setmasses, true);
		}

		/**
		Returns mass and intensities of requested index
		@param nWhichFunction requested function
		@param scan type
		@param [out] masses
		@param [out] intensities
		@return true if successfull
		*/
		bool GetScan(const int& nWhichIndex, const MassLynxScanType& mlScanType, vector<float>& masses, vector<float>& intensities)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
			bool bSuccess = CheckReturnCode(ddaGetScan(GetProcessor(), nWhichIndex, mlScanType, &pMasses, &pIntensities, &nSize), false);

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);

			return bSuccess;
		}

		/**
		Get the next available scan in the processor
		@param [out] fRT scan retention time
		@param [out] mlScanType scan type
		@param [out] fSetMass scan set mass
		@param [out] masses
		@param [out] intensities
		@return true if scan available
		*/
		bool GetNextScan(float& fRT, MassLynxScanType& mlScanType, float& fSetMass, vector<float>& masses, vector<float>& intensities) const
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0); bool bNext(false);
			CheckReturnCode(ddaGetNextScan(GetProcessor(), &fRT, &mlScanType, &fSetMass, &pMasses, &pIntensities, &nSize, &bNext), false);

			// fill the vector
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);

			return bNext;
		}


		/**
		Reset the next scan counter
		@return void
		*/
		void Reset() const
		{
			// reset the scan iterator
			CheckReturnCode(ddaResetScan(GetProcessor()));
		}
	};

	class MassLynxMSeProcessor : public MassLynxBaseProcessor
	{
	public:
		MassLynxMSeProcessor() : MassLynxBaseProcessor(MassLynxBaseType::MSE) {}

		/**
		Get the next available scan in the processor
		@param [out] fRT scan retention time
		@param [out] mlScanType scan type
		@param [out] masses
		@param [out] intensities
		@return true if scan available
		*/
		bool GetNextScan(float& fRT, MassLynxScanType& mlScanType, vector<float>& masses, vector<float>& intensities) const
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0); bool bNext(false);
			CheckReturnCode(mseGetNextScan(GetProcessor(), &fRT, &mlScanType, &pMasses, &pIntensities, &nSize, &bNext));

			// fill the vector and deallocate the memory
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);

			return bNext;
		}

		/**
		Reset the next scan counter
		@return void
		*/
		void Reset() const
		{
			// reset the scan iterator
			CheckReturnCode(mseResetScan(GetProcessor()));
		}
	};
}
}
}

