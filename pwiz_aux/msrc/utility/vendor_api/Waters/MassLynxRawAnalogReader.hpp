//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawAnalogReader.hpp
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
	using std::string;
	using std::vector;

	/**
	* Allows access to raw analog data and chanel information
	*
	* If an error is encountered then an exception will be throw, see exception codes and messages.
	*
	* Creating and using an analog reader
	* @snippet MassLynxRawExample.cpp create_analog_reader
	*/

	class MassLynxRawAnalogReader : public MassLynxBaseReader
	{
	public:

		MassLynxRawAnalogReader(const string& strFullPath) : MassLynxBaseReader(strFullPath, MassLynxBaseType::ANALOG) { }
		MassLynxRawAnalogReader(const MassLynxBaseReader& massLynxRawReader) : MassLynxBaseReader(massLynxRawReader, MassLynxBaseType::ANALOG) {}
		MassLynxRawAnalogReader(const MassLynxRawAnalogReader& massLynxRawAnalogReader) : MassLynxBaseReader(massLynxRawAnalogReader, MassLynxBaseType::ANALOG) {}

		/**
		*  Returns the number of channels
		*   @return int number of channels
		*/
		int GetChannelCount() const
		{
			int nChannels(0);
			CheckReturnCode(getChannelCount(GetRawReader(), &nChannels));
			return nChannels;
		}

		/**
		*  Returns the channel description
		*
		*   @param  nWhichChannel requested channel
		*
		*   @return string channel description
		*/
		string GetChannelDescription(const int& nWhichChannel) const
		{
			char* chValue(NULL);
			CheckReturnCode(getChannelDesciption(GetRawReader(), nWhichChannel, &chValue));
			return ToString(chValue);
		}

		/**
		*  Returns the channel units
		*
		*   @param  nWhichChannel requested channel
		*
		*   @return string channel units
		*/
		string GetChannelUnits(const int& nWhichChannel) const
		{
			char* chValue(NULL);
			CheckReturnCode(getChannelUnits(GetRawReader(), nWhichChannel, &chValue));
			return ToString(chValue);
		}

		/**
		*  Reads the times and intensities for the requested channel
		*
		*   @param  nWhichChannel requested channel
		*   @param [out] times  vector of times
		*   @param [out] intensities  vector of intensities
		*   @return void
		*/
		void ReadChannel(const int& nWhichChannel, vector<float>& times, vector<float>& intensities) const
		{
			// get the data..
			const float* pTimes(NULL); const float* pIntensities(NULL); int nSize(0);
			CheckReturnCode(readChannel(GetRawReader(), nWhichChannel, &pTimes, &pIntensities, &nSize));

			// fill the vector and deallocate the memory
			ToVector(pTimes, nSize, times);
			ToVector(pIntensities, nSize, intensities);
		}
	};
}
}
}

