//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxLockmassProcessor.h
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
	//class MassLynxParameters;
	/**
	* Applies post acquisition lock mass correction to MassLynx data
	*
	* Lock mass correction can only be applied under certain conditions, data that has been lock mass corrected during acqusition can not be changed. To prevent unnecessary recalculation, comparison between the supplied parameters and the existing lock mass values is made. If these values are different then lock mass correction will be applied. If the 'force' parameter is set to true then lock mass correction will always be applied.
	*
	* If the lock mass algorithm has changed this will also force a recalculation of the lock mass correction.
	*
	* If an error is encountered then an exception will be throw, see exception codes and messages.

	* Inherit update
	* @snippet MassLynxRawExample.cpp LockmassProcessor_inherit
	* Apply post acquisition lockmass correction
	* @snippet MassLynxRawExample.cpp lockmass_apply
	* Remove post acquisition lockmass correction
	* @snippet MassLynxRawExample.cpp lockmass_remove
	*/

	class MassLynxLockMassProcessor : public MassLynxBaseProcessor
	{
	public:
		MassLynxLockMassProcessor() : MassLynxBaseProcessor(MassLynxBaseType::LOCKMASS) {}

		/**
		*   Removes post acquisition lock mass correction.
		*
		*   @return void
		*/
		void RemoveLockMassCorrection()
		{
			CheckReturnCode(removeLockMassCorrection(GetProcessor()));
		}

		/**
		*   Determines if the raw data has post acquisition lock mass correction applied.
		*
		*   @return false if post acqustion lock mass correction is not applied.
		*/
		bool IsLockMassCorrected() const
		{
			int isCorrected(0);
			CheckReturnCode(LMP_isLockMassCorrected(GetProcessor(), &isCorrected));
			return isCorrected;
		}

		/**
		*   Determines if post acquisition lock mass correction can be applied.
		*
		*   @return false if the raw data can not be lock mass corrected.
		*/
		bool CanLockMassCorrect() const
		{
			int canCorrect(0);
			CheckReturnCode(LMP_canLockMassCorrect(GetProcessor(), &canCorrect));
			return canCorrect;
		}

		/**
		*   Returns the currenty applied lock mass parameters
		*
		*   @param  mass is the value of the lock mass
		*   @param  tolerance is the tolerance
		*
		*   @return void
		*/
		void GetLockMassValues(float& mass, float& tolerance) const
		{
			CheckReturnCode(getLockMassValues(GetProcessor(), &mass, &tolerance));
		}

		/**
		*   Returns the currenty applied lockmass correction gain for a requested retention time
		*
		*   @param  retentiontime
		*
		*   @return gain lock mass gain
		*/
		float GetLockMassCorrection(const float& retentionTime) const
		{
			float gain(0.0f);
			CheckReturnCode(getLockMassCorrection(GetProcessor(), retentionTime, &gain));
			return gain;
		}

		/**
		*  \deprecated Use GetLockMassCorrection(const float& retentionTime) const
		*/
		void GetLockMassCorrection(const float& retentionTime, float& gain) const
		{
			CheckReturnCode(getLockMassCorrection(GetProcessor(), retentionTime, &gain));
		}

		bool TryGetLockMassCorrection(const float& retentionTime, float& gain) const
		{
			return CheckReturnCode(getLockMassCorrection(GetProcessor(), retentionTime, &gain), false);
		}


		/**
		*  \deprecated Use  SetParameters(const MassLynxParameters& lockMassParameters)
		*/
		//[[deprecated("Replaced by  SetParameters(const MassLynxParameters& lockMassParameters)")]]
		//void SetParameters(const string& lockMassParameters)
		//{
		//	CheckReturnCode(setLockMassParameters_dep(GetProcessor(), lockMassParameters.c_str()));
		//}

		/**
		*   Sets the parameters into the lock mass processor. 
		*	Parameters are supplied in a key value pair in the MassLynxParameter class
		*
		*	Key	Description
		*	LockMassParameter::MASS			Value of the lock mass
		*	LockMassParameter::TOLERANCE	Tolerance applied to the given lock mass used to find lock mass in data
		*	LockMassParameter::FORCE		Boolean value used to force the lock mass
		*
		*   @param  MassLynxParameters class
		*
		*   @return void
		*/
		void SetParameters(const MassLynxParameters& lockMassParameters)
		{
			CheckReturnCode(setLockMassParameters(GetProcessor(), lockMassParameters.GetParameters()));
		}

		/**
		*  Returns JSON formatted string of parameters set in the processor
		*
		*   @return string lock mass parameters
		*/
		string GetParameters() const
		{
			char* pParameters(NULL);
			CheckReturnCode(getLockMassParameter(GetProcessor(), &pParameters));
			return ToString(pParameters);
		}

		/**
		*  Performs lock mass correction with the supplied parameters
		*
		*   @return bool true if sucessfully lock mass correction is successful
		*/
		//bool LockMassCorrect(const string& lockMassParameters)
		//{
		//	SetParameters(lockMassParameters);
		//	return LockMassCorrect();
		//}

		bool LockMassCorrect(const MassLynxParameters& lockMassParameters)
		{
			SetParameters(lockMassParameters);
			return LockMassCorrect();
		}

		/**
		*  Performs lock mass correction using set parameters
		*
		*   @return bool true if sucessfully lock mass correction is successful
		*/
		bool LockMassCorrect()
		{
			bool bCorrected(false);
			CheckReturnCode(lockMassCorrect(GetProcessor(), &bCorrected));
			return bCorrected;
		}

		/**
		*  Returns the combined and centrioded spectrum of all lock mass scans
		*  Can be used to identify the lock mass
		*
		*   @param [out] masses vector of masses
		*   @param [out] intensities vector of intensities
		*
		*   @return void
		*/
		void GetCandidates(vector<float>& masses, vector<float>& intensities)
		{
			// get the data..
			float* pMasses(NULL); float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(getLockMassCandidates(GetProcessor(), &pMasses, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pMasses, nSize, masses, true);
			ToVector(pIntensities, nSize, intensities, true);
		}
	};
}   // MassLynxRaw
}   // Lib
}   // Waters
