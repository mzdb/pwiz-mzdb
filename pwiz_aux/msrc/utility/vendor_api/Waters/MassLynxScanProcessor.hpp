//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxScanProcessor.h
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation
//
//-----------------------------------------------------------------------------------------------------
#pragma once

#include "MassLynxRawBase.hpp"
#include "MassLynxParameters.hpp"

namespace Waters
{
namespace Lib
{
namespace MassLynxRaw
{
	using std::string;
	using std::vector;

	class MassLynxScanProcessor : public MassLynxBaseProcessor
	{
	public:
		MassLynxScanProcessor() : MassLynxBaseProcessor(MassLynxBaseType::SCAN) {}

	public:

		MassLynxScanProcessor& Combine(const int& nWhichFunction, const int* pScans, const int& nScans)
		{
			CheckReturnCode(combineScan(GetProcessor(), nWhichFunction, pScans, nScans));
			return *this;
		}

		// load single scan
		MassLynxScanProcessor& Load(const int& nWhichFunction, const int& nWhichScan)
		{
			CheckReturnCode(combineScan(GetProcessor(), nWhichFunction, &nWhichScan, 1));
			return *this;
		}

		// return the processed scan
		void MassLynxScanProcessor::GetScan(vector<float>& masses, vector<float>& intensities) const
		{
			// get the data..
			float* pMasses(NULL);
			float* pIntensities(NULL);
			int nSize(0);
			CheckReturnCode(getScan(GetProcessor(), &pMasses, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pMasses, nSize, masses);
			ToVector(pIntensities, nSize, intensities);
		}

		MassLynxScanProcessor& MassLynxScanProcessor::SetThresholdParameters(const MassLynxParameters& thresholdParameters)
		{
			// send JSON string
			CheckReturnCode(setThresholdParameter(GetProcessor(), thresholdParameters.GetParameters()));
			return *this;
		}

		MassLynxScanProcessor& MassLynxScanProcessor::Threshold(const MassLynxParameters& thresholdParameters)
		{
			SetThresholdParameters(thresholdParameters);
			return Threshold();
		}

		MassLynxScanProcessor& MassLynxScanProcessor::Threshold()
		{
			CheckReturnCode(thresholdScan(GetProcessor()));
			return *this;
		}



		//string MassLynxScanProcessor::GetThresholdParameters() const
		//{
		//	char* pParameters(NULL);
		//	CheckReturnCode(getThresholdParameter(GetProcessor(), &pParameters));
		//	return ToString(pParameters);
		//}

		//MassLynxScanProcessor& SetSmoothParameters(const std::string& smoothParameters)
		//{
		//	// send JSON string
		//	CheckReturnCode(setSmoothParameter(GetProcessor(), smoothParameters.c_str()));
		//	return *this;
		//}

		MassLynxScanProcessor& SetSmoothParameters(const MassLynxParameters& smoothParameters)
		{
			// send JSON string
			CheckReturnCode(setSmoothParameter(GetProcessor(), smoothParameters.GetParameters()));
			return *this;
		}



		//string GetSmoothParameters() const
		//{
		//	char* pParameters(NULL);
		//	CheckReturnCode(getSmoothParameter(GetProcessor(), &pParameters));
		//	return ToString(pParameters);
		//}

		//MassLynxScanProcessor& Smooth(const std::string& smoothParameters)
		//{
		//	SetSmoothParameters(smoothParameters);
		//	return Smooth();
		//}

		MassLynxScanProcessor& Smooth(const MassLynxParameters& smoothParameters)
		{
			SetSmoothParameters(smoothParameters);
			return Smooth();
		}

		MassLynxScanProcessor& Smooth()
		{
			CheckReturnCode(smoothScan(GetProcessor()));
			return *this;
		}

		MassLynxScanProcessor& Centroid()
		{
			CheckReturnCode(centroidScan(GetProcessor()));
			return *this;
		}
	};
}   // MassLynxRaw
}   // Lib
}   // Waters



