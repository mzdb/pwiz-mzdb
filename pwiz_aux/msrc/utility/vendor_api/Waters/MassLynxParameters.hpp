//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxParameters.hpp
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation
//	
//-----------------------------------------------------------------------------------------------------
#pragma once

#include "MassLynxRawDefs.h"
#include "MassLynxRawBase.hpp"

namespace Waters
{
	namespace Lib
	{
		namespace MassLynxRaw
		{
			using std::string;

			/**
			*  @brief Helper class to build parameter settings
			*/
			class MassLynxParameters
			{
			public:
				MassLynxParameters() : m_pParameters(NULL)
				{
					m_pStringHandler = new MassLynxStringHandler();
					createParameters(&m_pParameters);
				}

				MassLynxParameters(const MassLynxParameters& mlParams) : m_pParameters(NULL)
				{
					m_pStringHandler = new MassLynxStringHandler();
					createParametersFromParameters(mlParams.GetParameters(), &m_pParameters);
				}

				~MassLynxParameters()
				{
					delete m_pStringHandler;
					destroyParameters(m_pParameters);
				}

				// Overload =
				MassLynxParameters operator=(const MassLynxParameters& rhs)
				{
					destroyParameters(m_pParameters);
					createParametersFromParameters(rhs.GetParameters(), &m_pParameters);
					return *this;
				}

				template< typename T>
				MassLynxParameters& Set(const MassLynxScanItem& key, const T& value)
				{
					return Set((int)key, value);
				}

				template< typename T>
				MassLynxParameters& Set(const MassLynxHeaderItem& key, const T& value)
				{
					return Set((int)key, value);
				}

				template< typename T>
				MassLynxParameters& Set(const MassLynxSampleListItem& key, const T& value)
				{
					return Set((int)key, value);
				}

				template< typename T>
				MassLynxParameters& Set(const LockMassParameter& key, const T& value)
				{
					return Set((int)key, value);
				}

				MassLynxParameters& Set(const SmoothParameter& key, const int& value)
				{
					return Set((int)key, value);
				}

				MassLynxParameters& Set(const SmoothParameter& key, const SmoothType& value)
				{
					return Set((int)key, (int)value);
				}

				// Function parameters for raw writer
				template< typename T>
				MassLynxParameters& Set(const FunctionDefinition& key, const T& value)
				{
					return Set((int)key, (int)value);
				}

				string Get(const MassLynxHeaderItem& key) const
				{
					return Get((int)key);
				}

				string Get(const MassLynxScanItem& key) const
				{
					return Get((int)key);
				}

				vector<int> GetKeys() const
				{
					int* pKeys(NULL);
					int size(0);
					getParameterKeys(m_pParameters, &pKeys, &size);
					return vector<int>(pKeys, pKeys + size);
				}

				CMassLynxParameters GetParameters() const { return m_pParameters; }

			private:

				string Get(const int& key) const
				{
					char* pValue(NULL);
					getParameterValue(m_pParameters, key, &pValue);
					return m_pStringHandler->ToString(pValue, false);
				}

				MassLynxParameters& Set(const int& key, const char* value)
				{
					setParameterValue(m_pParameters, key, value);
					return *this;
				}

				MassLynxParameters& Set(const int& key, const string& value)
				{
					Set(key, value.c_str());
					return *this;
				}

				MassLynxParameters& Set(const int& key, const float& value)
				{
					Set(key, std::to_string(value));
					return *this;
				}

				MassLynxParameters& Set(const int& key, const double& value)
				{
					Set(key, std::to_string(value));
					return *this;
				}

				MassLynxParameters& Set(const int& key, const int& value)
				{
					Set(key, std::to_string(value));
					return *this;
				}

				MassLynxParameters& Set(const int& key, const bool& value)
				{
					Set(key, std::to_string(value));
					return *this;
				}

				MassLynxParameters& Set(const int& key, const SmoothType& value)
				{
					Set(key, (int)value);
					return *this;
				}

			private:
				CMassLynxParameters m_pParameters;
				MassLynxStringHandler* m_pStringHandler;
			};

		}   // MassLynxRaw
	}   // Lib
}   // Waters