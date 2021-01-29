//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawChromatogramReader.h
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation
//-----------------------------------------------------------------------------------------------------
#pragma once

#include "MassLynxRawBase.hpp"

namespace Waters
{
namespace Lib
{
namespace MassLynxRaw
{
	using std::string;
	using std::vector;

	class MassLynxRawChromatogramReader : public MassLynxBaseReader
	{
	public:

		MassLynxRawChromatogramReader(const string & strFullPath) : MassLynxBaseReader(strFullPath, MassLynxBaseType::CHROM) { }
		MassLynxRawChromatogramReader(const MassLynxBaseReader& massLynxRawReader) : MassLynxBaseReader(massLynxRawReader, MassLynxBaseType::CHROM) { }
		MassLynxRawChromatogramReader(const MassLynxRawChromatogramReader& massLynxRawChromatogramReader) : MassLynxBaseReader(massLynxRawChromatogramReader, MassLynxBaseType::CHROM) { }

		/**
		*  Returns the TIC chromatogram for the requested function
		*
		*   @param  nWhichFunction requested function
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadTICChromatogram(const int& nWhichFunction, vector<float> & times, vector<float> & intensities) const
		{
			// get the data..
			float* pTimes(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readTICChromatogram(GetRawReader(), nWhichFunction, &pTimes, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			ToVector(pIntensities, nSize, intensities, true);
		}

		/**
		*  Returns the BPI chromatogram for the requested function
		*
		*   @param  nWhichFunction requested function
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadBPIChromatogram(const int& nWhichFunction, vector<float> & times, vector<float> & intensities)
		{
			// get the data..
			float* pTimes(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readBPIChromatogram(GetRawReader(), nWhichFunction, &pTimes, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			ToVector(pIntensities, nSize, intensities, true);
		}

		/**
		*  Returns the mass chromatogram for the requested function and mass
		*
		*   @param  nWhichFunction requested function
		*   @param  fMass requested mass
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*	@param  fMassWindow mass window
		*	@param  bProducts
		*   @return void
		*/
		void ReadMassChromatogram(const int& nWhichFunction, const float& fMass, vector<float> & times, vector<float> & intensities, const float& fMassWindow, bool bProducts)
		{
			// get the data..
			float* pTimes(NULL); int nSize(0); float* pIntensities(NULL);
			CheckReturnCode(readMassChromatograms(GetRawReader(), nWhichFunction, &fMass, 1, &pTimes, &pIntensities, fMassWindow, bProducts, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			ToVector(pIntensities, nSize, intensities, true);
		}

		/**
		*  Returns the mass chromatogram for the requested function and vector of masses
		*
		*   @param  nWhichFunction requested function
		*   @param  fMass requested mass
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*	@param  fMassWindow mass window
		*	@param  bProducts
		*   @return void
		*/
		void ReadMultipleMassChromatograms(const int& nWhichFunction, const vector<float>& masses, vector<float>& times, vector<std::vector<float>>& intensities, const float& fMassWindow, bool bProducts = true)
		{
			// get the data..
			int nMasses(static_cast<int>(masses.size())); float* pTimes(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readMassChromatograms(GetRawReader(), nWhichFunction, &masses[0], nMasses, &pTimes, &pIntensities, fMassWindow, bProducts, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			intensities.resize(nMasses);
			for (int nIndex = nMasses - 1; nIndex > -1; --nIndex)
				ToVector(&pIntensities[nIndex*nSize], nSize, intensities[nIndex], nIndex == 0);
		}

		/**
		*  Returns the chromatogram for the requested function and MRM
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichMRM requested MRM
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadMRMChromatogram(const int& nWhichFunction, const int& nWhichMRM, std::vector<float> & times, std::vector<float> & intensities)
		{
			// get the data..
			float* pTimes(NULL); int nSize(0); float* pIntensities;
			CheckReturnCode(readMRMChromatograms(GetRawReader(), nWhichFunction, &nWhichMRM, 1, &pTimes, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			ToVector(pIntensities, nSize, intensities, true);
		}

		/**
		*  Returns the chromatogram for the requested function and vector of MRMs
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichMRMs requested MRMs
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadMRMChromatogram(const int& nWhichFunction, const vector<int> whichMRM, vector<float> & times, vector<vector<float>>& intensities)
		{
			// get the data..
			int nMRMs(static_cast<int>(whichMRM.size())); float* pTimes(NULL); float* pIntensities(NULL);	int nSize(0);
			CheckReturnCode(readMRMChromatograms(GetRawReader(), nWhichFunction, &whichMRM[0], nMRMs, &pTimes, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			intensities.resize(nMRMs);
			for (int nIndex = nMRMs - 1; nIndex > -1; --nIndex)
				ToVector(&pIntensities[nIndex*nSize], nSize, intensities[nIndex], nIndex == 0);
		}

		/**
		*  Returns the chromatogram for the requested function, precursoe and target mass
		*	Note: This function uses mass tolerance not window ( tolerance = window /2 )
		*
		*   @param  nWhichFunction requested function
		*   @param  fPrecursorMass requested mass
		*   @param  fPrecursorTolerance requested tolerance
		*   @param  fTargetMass requested target mass
		*   @param  fTargetTolerance requested target mass tollerance
		*   @param [out] times vector of times
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void ReadMassChromatogram(const int& nWhichFunction, const float& fPrecursorMass, const float& fPrecursorTolerance, const float& fMass, const float& fMassTolerance, vector<float> & times, vector<float> & intensities)
		{
			// get the data..
			float* pTimes(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readSonarMassChromatogram(GetRawReader(), nWhichFunction, fPrecursorMass, fMass, &pTimes, &pIntensities, fPrecursorTolerance * 2, fMassTolerance * 2, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times, true);
			ToVector(pIntensities, nSize, intensities, true);
		}
	};
}
}
}