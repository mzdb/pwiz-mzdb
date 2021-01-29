//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawInfo.h
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation
//
// COMMENTTS:		This header contains the declaration of the MassLynxRawInfo
//					class.
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

	/**
	* Allows access to raw data information and scan statistics
	*
	* If an error is encountered then an exception will be throw, see exception codes and messages.

	* Creating and using an info reader
	* @snippet MassLynxRawExample.cpp info_function_details
	* Read header items
	* @snippet MassLynxRawExample.cpp info_read_header
	* Read scan stats items
	* @snippet MassLynxRawExample.cpp info_read_stats
	*/


	class MassLynxRawInfo : public MassLynxBaseReader
	{
	public:
		MassLynxRawInfo(const string & strFullPath) : MassLynxBaseReader(strFullPath, MassLynxBaseType::INFO) {}
		MassLynxRawInfo(const MassLynxBaseReader& rawReader) : MassLynxBaseReader(rawReader, MassLynxBaseType::INFO) {}
		MassLynxRawInfo(const MassLynxRawInfo& rawReader) : MassLynxBaseReader(rawReader, MassLynxBaseType::INFO) {}

		/**
		*  Returns the number of ms functions in the Raw data.
		*
		*   @return number of functions
		*/
		int GetFunctionCount() const
		{
			unsigned int nFunction(0);
			CheckReturnCode(getFunctionCount(GetRawReader(), &nFunction));
			return nFunction;
		}

		/**
		*  Returns the description of the requested function type.
		*
		*   @param  functionType - requested MassLynxFunctionType
		*
		*   @return function type description
		*/
		string GetFunctionTypeString(const MassLynxFunctionType& functionType) const
		{
			char* chFunctionType(NULL);
			CheckReturnCode(getFunctionTypeString(GetRawReader(), functionType, &chFunctionType));
			return ToString(chFunctionType);
		}

		/**
		*  Returns the acqusition type.
		*
		*   @return acquisition type
		*/
		MassLynxAcquisitionType GetAcquisitionType() const
		{
			MassLynxAcquisitionType acquisitionType(MassLynxAcquisitionType::UNINITIALISED);
			CheckReturnCode(getAcquisitionType(GetRawReader(), &acquisitionType));
			return acquisitionType;
		}

		/**
		*  Returns the requested function type.
		*
		*   @param  nWhichFunction requested function
		*
		*   @return function type
		*/
		MassLynxFunctionType GetFunctionType(const int& nWhichFunction) const
		{
			MassLynxFunctionType functionType(MassLynxFunctionType::UNINITIALISED);
			CheckReturnCode(getFunctionType(GetRawReader(), nWhichFunction, &functionType));
			return functionType;
		}

		/**
		*  Returns the requested ion mode.
		*
		*   @param  nWhichFunction requested function
		*
		*   @return ion mode
		*/
		MassLynxIonMode GetIonMode(const int& nWhichFunction) const
		{
			MassLynxIonMode ionMode(MassLynxIonMode::UNINITIALISED);
			CheckReturnCode(getIonMode(GetRawReader(), nWhichFunction, &ionMode));
			return ionMode;
		}

		/**
		*  Returns the requested ion mode description.
		*
		*   @param  ionMode MassLynxIonMode
		*
		*   @return ion mode description
		*/
		string GetIonModeString(const MassLynxIonMode& ionMode) const
		{
			char* chIonMode(NULL);
			CheckReturnCode(getIonModeString(GetRawReader(), ionMode, &chIonMode));
			return ToString(chIonMode);
		}

		/**
		*  Returns the retention time of a scan.
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*
		*   @return retention time
		*/
		float GetRetentionTime(const int& nWhichFunction, const int& nWhichScan) const
		{
			float fRT(0.0f);
			CheckReturnCode(getRetentionTime(GetRawReader(), nWhichFunction, nWhichScan, &fRT));
			return fRT;
		}

		/**
		*  Returns the drift time of a scan.
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichDrift requested scan
		*
		*   @return retention time
		*/
		float GetDriftTime(const int& nWhichFunction, const int& nWhichDrift) const
		{
			float fDT(0.0f);
			CheckReturnCode(getDriftTime(GetRawReader(), nWhichFunction, nWhichDrift, &fDT));
			return fDT;
		}

		/**
		*  Returns the Drift Time calcuated using the drift time calibration
		*  Collisional Cross Section calibration must be present
		*  Exception will be thrown on error
		*
		*   @param  ccs requested collisional cross section
		*   @param  mass
		*   @param  charge
		*
		*   @return DriftTime
		*/
		float GetDriftTime(const float& ccs, const float& mass, const int& charge) const
		{
			float dt(nanf(""));
			CheckReturnCode(getDriftTime_CCS(GetRawReader(), ccs, mass, charge, &dt));
			return dt;
		}

		/**
		* Returns the Drift Time calcuated using the drift time calibration
		* Collisional Cross Section calibration must be present
		* Exception will not be thrown, use GetLastError to retrieve error code
		*
		*  @param  ccs requested collisional cross section
		*  @param  mass
		*  @param  charge
		*  @param [out] drift time
		*
		*  @return true if successful

		*/
		bool TryGetDriftTime(const float& ccs, const float& mass, const int& charge, float & dt) const
		{
			return CheckReturnCode(getDriftTime_CCS(GetRawReader(), ccs, mass, charge, &dt), false);
		}

		/**
		*  Returns the CollisionalCrossSection
		*  Collisional Cross Section calibration must be present
		*  Exception will be thrown on error
		*
		*   @param  driftTime requested drift time
		*   @param  mass
		*   @param  charge
		*
		*   @return CollisionalCrossSection
		*/
		float GetCollisionalCrossSection(const float& driftTime, const float& mass, const int& charge) const
		{
			float fCCS(nanf(""));
			CheckReturnCode(getCollisionalCrossSection(GetRawReader(), driftTime, mass, charge, &fCCS));
			return fCCS;
		}

		/**
		*  Returns the CollisionalCrossSection
		*  Collisional Cross Section calibration must be present
		*  Exception will not be thrown, use GetLastError to retrieve error code
		*
		*  @param  driftTime requested drift time
		*  @param  mass
		*  @param  charge
		*  @param [out] fCCS collisional cross section
		*
		*  @return true if successful
		*/

		bool TryGetCollisionalCrossSection(const float& driftTime, const float& mass, const int& charge, float& fCCS) const
		{
			return CheckReturnCode(getCollisionalCrossSection(GetRawReader(), driftTime, mass, charge, &fCCS), false);
		}

		/**
		*  Returns the acquisition mass range of the requested function.
		*
		*   @param  nWhichFunction requested function
		*   @param [out] startMass start mass of the function
		*   @param [out] endMass end mass of the function
		*
		*   @return void
		*/
		void GetAcquisitionMassRange(const int& nWhichFunction, float& startMass, float& endMass) const
		{
			CheckReturnCode(getAcquisitionMassRange(GetRawReader(), nWhichFunction, 0, &startMass, &endMass));
		}

		/**
		*  Returns the acquisition mass range of the requested function.
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichMRM requested mrm transition
		*   @param [out] startMass start mass of the function
		*   @param [out] endMass end mass of the function
		*
		*   @return void
		*/
		void GetAcquisitionMassRange(const int& nWhichFunction, const int& nWhichMRM, float& startMass, float& endMass) const
		{
			CheckReturnCode(getAcquisitionMassRange(GetRawReader(), nWhichFunction, nWhichMRM, &startMass, &endMass));
		}

		/**
		*  Returns the acquisition time range of the requested function.
		*
		*   @param  nWhichFunction requested function
		*   @param [out] startTime start time of the function
		*   @param [out] endTime end time of the function
		*
		*   @return void
		*/
		//void GetAcquisitionTimeRange(const int& nWhichFunction, float & startTime, float & endTime) const
		//{
		//	CheckReturnCode(getAcquisitionTimeRange(GetRawReader(), nWhichFunction, &startTime, &endTime));
		//}

		void GetAcquisitionTimeRange(const int& nWhichFunction, float& startTime, float& endTime) const
		{
			CheckReturnCode(getAcquisitionTimeRange(GetRawReader(), nWhichFunction, &startTime, &endTime));
		}

		/**
		*   Returns the number of scans in the requested function
		*
		*   @param  nWhichFunction requested function
		*
		*   @return number of scans
		*/
		int GetScansInFunction(const int& nWhichFunction) const
		{
			unsigned int nScans(0);
			CheckReturnCode(getScanCount(GetRawReader(), nWhichFunction, &nScans));
			return nScans;
		}

		bool TryGetScansInFunction(const int& nWhichFunction, unsigned int& nScans) const
		{
			return CheckReturnCode(getScanCount(GetRawReader(), nWhichFunction, &nScans), false);
		}

		/**
		*   Returns the number of drift scans in the requested function
		*
		*   @param  nWhichFunction requested function
		*
		*   @return number of scans
		*/
		int GetDriftScanCount(const int& nWhichFunction) const
		{
			unsigned int nScans(0);
			CheckReturnCode(getDriftScanCount(GetRawReader(), nWhichFunction, &nScans));
			return nScans;
		}

		/**
		*  Returns the number of MRM transitions in the requested function
		*  Function must be of type MRM
		*  Exception will be thrown on error
		*
		*  @param  nWhichFunction requested function
		*
		*  @return number of MRM transitions
		*/
		int GetMRMCount(const int& nWhichFunction) const
		{
			int nMRMs(0);
			CheckReturnCode(getMRMCount(GetRawReader(), nWhichFunction, &nMRMs));
			return nMRMs;
		}

		/**
		*  Returns the number of MRM transitions in the requested function
		*  Exception will not be thrown, use GetLastError to retrieve error code
		*
		*
		*  @param  nWhichFunction requested function
		*  @param [out] number of MRM transitions
		*
		*  @return true if successful
		*/
		bool TryGetMRMCount(const int& nWhichFunction, int& nMRMs) const
		{
			return CheckReturnCode(getMRMCount(GetRawReader(), nWhichFunction, &nMRMs), false);
		}
		
		/**
		*   Returns the type of data in the requested function
		*
		*   @param  nWhichFunction requested function
		*
		*   @return true if data is continuum
		*/
		bool IsContinuum(const int& nWhichFunction) const
		{
			bool bContinuum;
			CheckReturnCode(isContinuum(GetRawReader(), nWhichFunction, bContinuum));
			return bContinuum;
		}

		/**
		*  Returns a vector containing the MassLynxScanItem ids of the available items.
		*
		*   @param  nWhichFunction requested function
		*
		*   @return string scan item value
		*/
		vector<MassLynxScanItem> GetAvailableScanItems(const int& nWhichFunction) const
		{
			MassLynxParameters params = GetScanItems(nWhichFunction);

			// convert to a vector
			vector<int> keys = params.GetKeys();
			vector<MassLynxScanItem> items;
			for (size_t index(0); index < keys.size(); ++index)
				items.push_back((MassLynxScanItem)keys[index]);

			return items;

			//int nSize(0); MassLynxScanItem* pItems(NULL);
			//CheckReturnCode(getItemsInFunction(GetRawReader(), nWhichFunction, &pItems, &nSize));
			//return ToVector(pItems, nSize, true);
		}

		MassLynxParameters GetScanItems(const int& nWhichFunction) const
		{
			MassLynxParameters parameters;
			CheckReturnCode(getScanItemsInFunction(GetRawReader(), nWhichFunction, parameters.GetParameters()));
			return parameters;
		}

		/**
		*  Returns the scan item value for the requested function and scan.
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*	@param  nWhichItem requested item
		*
		*   @return string scan item value
		*/
		string GetScanItem(const int& nWhichFunction, const int& nWhichScan, const MassLynxScanItem& nWhichItem) const
		{
			return GetScanItem(nWhichFunction, nWhichScan, vector<MassLynxScanItem>(1, nWhichItem))[0];
		}

		/**
		*  Returns the scan item value for the requested function and scan.
		*
		*   @param  nWhichFunction requested function
		*   @param  nWhichScan requested scan
		*	@param  nWhichItem requested items
		*
		*   @return vector of scan item values
		*/
		vector<string> GetScanItem(const int& nWhichFunction, const int& nWhichScan, const vector<MassLynxScanItem>& whichItems) const
		{
			const int nItems(static_cast<int>(whichItems.size()));
			if (1 > nItems)
				return vector<string>(0);

			char* pValues(NULL);
			char chDelimiter;
			CheckReturnCode(getScanItems(GetRawReader(), nWhichFunction, nWhichScan, &whichItems[0], &pValues, nItems, &chDelimiter));
			return ToVector(ToString(pValues), chDelimiter);
		}

		MassLynxParameters GetScanItemValue(const int& nWhichFunction, const int& nWhichScan, const vector<MassLynxScanItem>& whichItems) const
		{
			MassLynxParameters parameters;
			getScanItemValue(GetRawReader(), nWhichFunction, nWhichScan, &whichItems[0], static_cast<int>(whichItems.size()), parameters.GetParameters());
			return parameters;
		}



		/**
		*  Returns the requested scan item description.
		*
		*   @param  whichItem MassLynxScanItem
		*
		*   @return scan item description
		*/
		string GetScanItemString(const MassLynxScanItem& whichItem) const
		{
			return GetScanItemString(vector<MassLynxScanItem>(1, whichItem))[0];
		}

		/**
		*  Returns the requested scan item description.
		*
		*   @param  whichItems vector of MassLynxScanItems
		*
		*   @return vector or scan item descriptions
		*/
		vector<string> GetScanItemString(const vector<MassLynxScanItem>& whichItems) const
		{
			int nItems(static_cast<int>(whichItems.size())); vector<string> names(nItems);
			if (1 > nItems)
				return vector<string>(0);

			char* pNames(NULL);
			char chDelimiter;
			CheckReturnCode(getScanItemNames(GetRawReader(), &whichItems[0], &pNames, nItems, &chDelimiter));

			return ToVector(ToString(pNames), chDelimiter);
		}

		MassLynxParameters GetScanItemName(const vector<MassLynxScanItem>& whichItems) const
		{
			MassLynxParameters parameters;
			CheckReturnCode(getScanItemName(GetRawReader(), &whichItems[0], static_cast<int>(whichItems.size()), parameters.GetParameters()));
			return parameters;
		}
		

		///**
		//*  \deprecated Use GetHeaderItem(const MassLynxHeaderItem& nWhichItem)
		//*/
		//[[deprecated("Replaced by GetHeaderItem(const MassLynxHeaderItem& nWhichItem)")]]
		//string GetHeaderItem(const string& strWhichTag) const
		//{
		//	char* chValue(NULL);
		//	CheckReturnCode(getHeaderItem(GetRawReader(), strWhichTag.c_str(), &chValue));
		//	return ToString(chValue);
		//}

		///**
		//*  \deprecated Use GetHeaderItem(const vector<MassLynxHeaderItem>& whichItems)
		//*/
		//[[deprecated("Replaced by GetHeaderItem(const vector<MassLynxHeaderItem>& whichItems)")]]
		//vector<string> GetHeaderItem(const vector<string>& whichTags) const
		//{
		//	vector<string> headerItem(whichTags.size());
		//	for (size_t nIndex = 0; nIndex < whichTags.size(); ++nIndex)
		//		headerItem[nIndex] = GetHeaderItem(whichTags[nIndex]);

		//	return headerItem;
		//}

		/**
		*   Returns the value of the requested header item
		*
		*   @param  whichItem enum of requested item
		*
		*   @return string - value of item
		*/
		string GetHeaderItem(const MassLynxHeaderItem& whichItem) const
		{
			return GetHeaderItem(vector<MassLynxHeaderItem>(1, whichItem))[0];
		}

		/**
		*   Returns the vector of the requested header items
		*
		*   @param  vector<MassLynxHeaderItem> vector of requested items
		*
		*   @return vector<string> - value of items
		*/
		vector<string> GetHeaderItem(const vector<MassLynxHeaderItem>& whichItems) const
		{
			const int nItems(static_cast<int>(whichItems.size()));
			if (1 > nItems)
				return vector<string>(0);

			char* pValues(NULL);
			char chDelimiter;
			CheckReturnCode(getHeaderItems(GetRawReader(), &whichItems[0], &pValues, nItems, &chDelimiter));
			return ToVector(ToString(pValues), chDelimiter);
		}

		MassLynxParameters GetHeaderItemValue(const vector<MassLynxHeaderItem>& whichItems) const
		{
			MassLynxParameters parameters;
			CheckReturnCode(getHeaderItemValue(GetRawReader(), &whichItems[0], static_cast<int>(whichItems.size()), parameters.GetParameters()));
			return parameters;
		}

		// needs a test
		MassLynxParameters GetHeaderItemValue(const MassLynxHeaderItem& whichItem) const
		{
			MassLynxParameters parameters;
			CheckReturnCode(getHeaderItemValue(GetRawReader(), &whichItem, 1, parameters.GetParameters()));
			return parameters;
		}

		/**
		*   Determines if the raw data has post acquisition lock mass correction applied.
		*
		*   @return false if post acqustion lock mass correction is not applied.
		*/
		bool IsLockMassCorrected() const
		{
			bool bCorrected;
			CheckReturnCode(isLockMassCorrected(GetRawReader(), &bCorrected));
			return bCorrected;
		}

		/**
		*   Determines if post acquisition lock mass correction can be applied.
		*
		*   @return false if the raw data can not be lock mass corrected.
		*/
		bool CanLockMassCorrect() const
		{
			bool bCanCorrect;
			CheckReturnCode(canLockMassCorrect(GetRawReader(), &bCanCorrect));
			return bCanCorrect;
		}
	};

	namespace Extended
	{
		class MassLynxRawInfo : public Waters::Lib::MassLynxRaw::MassLynxRawInfo
		{
		public:

			MassLynxRawInfo(const string & strFullPathName) : Waters::Lib::MassLynxRaw::MassLynxRawInfo(strFullPathName) {}
			MassLynxRawInfo(const MassLynxRawInfo& massLynxInfo) : Waters::Lib::MassLynxRaw::MassLynxRawInfo(massLynxInfo) {}
			MassLynxRawInfo(const MassLynxBaseReader& massLynxRawReader) : Waters::Lib::MassLynxRaw::MassLynxRawInfo(massLynxRawReader) {}

			bool GetSonarRange(const int& nWhichFunction, const float& preCursorMass, const float& preCursorTolerance, int& nStartIndex, int& nEndIndex) const
			{
				return CheckReturnCode(getIndexRange(GetRawReader(), nWhichFunction, preCursorMass, preCursorTolerance * 2, &nStartIndex, &nEndIndex));
			}
		};

	}
}
}
}

