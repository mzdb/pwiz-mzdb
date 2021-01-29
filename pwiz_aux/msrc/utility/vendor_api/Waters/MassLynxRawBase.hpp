//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxBaseReader.hpp
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation
//
// COMMENTS:		This header contains the delclaration of the MassLynxBaseReader
//					class.
//					
//-----------------------------------------------------------------------------------------------------

/*! \mainpage MassLynx SDK
*
* MassLynx Raw SDK is a set of assemblies and associated headers to allow access to MassLynx Raw data independently of the Masslynx application. Functionality includes extracting scan information, reading spectra, extracting chromatograms, TIC, BPi and mass, and lock mass correction.

Supported languages are c++, c# .Net, Java

Two main types of classes are available: Reader Classes and Processor Classes

<b>Reader Classes</b>

Reader class can be used to extract data and information from MassLynx Raw data. There are two methods to create a Reader class, from a fully specified file path or from an existing MassLynx reader object.
It is generally more efficient to create a reader class from an existing reader object, an example of this would to create a MassLynxInfo from a path to initially get some details on the raw data, eg number of scans in a function, and then subsequently create a Scan reader using the existing Info reader to read the scan data.

<b>Processor Classes</b>

Processor class allows processing to be done against MassLynx of raw data files. The processing parameters set in the processor class are applied to the chosen raw data file. For example applying lock mass correction to a list of raw data files, set the LockMass processing parameters and raw file and then process. If further massLynx raw data files need to be processed, reset the faw file and process again. The existing process arameters will be used t process the new file. In this way a list of raw data files can be process by initially setting the parameters and changing the target data file.

<b>Parameter Class</b>

Helper function to set parameters in a JSON format. Used in conjunction with processing classes

<b>Writer Class</b>

Used to create MassLynx Raw data files from mass and intensity data.



*
*/

/**
*  \cond HIDDEN_SYMBOLS
*  Doxygen ignore this class
*/

#pragma once

#include <string>
#include <vector>
#include "MassLynxRawDefs.h"


///<sumary>
///Check a MassLynxRaw result code
///<\summary>

namespace Waters
{
namespace Lib
{
namespace MassLynxRaw
{
	using std::string;
	using std::vector;

	//class MassLynxBaseProcessor;
	static void __stdcall processor_callback(void* pObject, const int& percent);

	// exception handler
	class MassLynxRawException : public std::exception
	{
	public:
		MassLynxRawException(const int& nCode, const string& message) : m_nCode(nCode), m_strMessage(message) {}
		virtual const char* what() const throw() { return m_strMessage.c_str(); }

		int code() const { return m_nCode; }

	private:
		std::string m_strMessage;
		int m_nCode;
	};

	// string handler
	class MassLynxStringHandler
	{
	public:
		string ToString(const char* chString, const bool& bRelease) const
		{
			if (NULL == chString)
				return "";

			string strValue(chString);
			if (bRelease)
			{
				releaseMemory((void*&)chString);
				chString = NULL;
			}

			return strValue;
		}
	};

	// vector handler
	class MassLynxVectorHandler
	{
	public:
		// return a vector of items by reference
		template< typename T>
		int ToVector(const T* pArray, const int& nSize, vector<T>& templateVector, const bool& bRelease = false) const
		{
			// do we have data?
			if (pArray == NULL)
			{
				templateVector.clear();
				return 0;
			}

			// something wrong so just clean up
			if (nSize < 1)
			{
				if (bRelease)
					releaseMemory((void*)pArray);

				templateVector.clear();
				return 0;
			}

			templateVector.resize(nSize);
			memcpy(&templateVector[0], pArray, nSize * sizeof(T));

			if (bRelease)
				releaseMemory((void*)pArray);

			return 0;
		}

		// return a vector of items
		template< typename T>
		vector<T> ToVector(const T* pArray, const int& nSize, const bool& bRelease = false) const
		{
			vector<T>templateVector(nSize);
			ToVector(pArray, nSize, templateVector, bRelease);
			return templateVector;
		}

		// split a string and return a vector
		vector<string> ToVector(const string &s, char delim) const
		{
			// check if empty - return empty vector 
			if (s.empty())
				return vector<string>(0);

			auto pos = s.find(delim);

			// check if just one item
			if (string::npos == pos)
				return vector<string>{s};

			size_t i = 0;
			vector<string> elems;
			while (pos != string::npos) {
				elems.push_back(s.substr(i, pos - i));
				i = ++pos;
				pos = s.find(delim, pos);

				if (pos == string::npos)
					elems.push_back(s.substr(i, s.length()));
			}

			return elems;
		}
	};

	// handle the checking of the return codes
	class MassLynxCodeHandler
	{
	public:
		MassLynxCodeHandler() : m_nCode(0) {
			m_pStringHandler = new MassLynxStringHandler;
		}

		virtual ~MassLynxCodeHandler() { delete m_pStringHandler; }

		bool CheckReturnCode(const int& nCode, const bool& bThrow = true) const
		{
			m_nCode = nCode;
			if (0 == nCode)
				return true;

			// three option true, false, throw exception
			//	if (0 < nCode)
			if (bThrow)
				throw MassLynxRawException(GetLastCode(), GetLastMessage());

			// get last error
			return false;
		}

		virtual string GetLastMessage() const
		{
			char* pErrorMessage(NULL);
			getErrorMessage(GetLastCode(), &pErrorMessage);
			return ToString(pErrorMessage, true);
		}

		int GetLastCode() const
		{
			return m_nCode;
		}
	protected:
		string ToString(const char* chString, const bool& bRelease) const
		{
			return m_pStringHandler->ToString(chString, bRelease);
		}


	private:
		mutable int m_nCode;
		MassLynxStringHandler* m_pStringHandler;
	};

	// Base reader class
	class MassLynxBaseReader
	{
	protected:
		MassLynxBaseReader(const string & strFullPath, MassLynxBaseType nType) : m_pRawReader(NULL), m_pCodeHandler(NULL), m_pStringHandler(NULL), m_pVectorHandler(NULL)
		{
			m_pCodeHandler = new MassLynxCodeHandler();
			if ( !CheckReturnCode(createRawReaderFromPath(strFullPath.c_str(), &m_pRawReader, nType), false))
			{
				// need to throw exception
				int nCode(GetLastCode());
				string message(GetLastMessage());
				delete m_pCodeHandler;

				throw MassLynxRawException(nCode, message);
			}

			m_pStringHandler = new MassLynxStringHandler();
			m_pVectorHandler = new MassLynxVectorHandler();
		}

		MassLynxBaseReader(const MassLynxBaseReader& massLynxBaseReader, MassLynxBaseType nType) : m_pRawReader(NULL), m_pCodeHandler(NULL), m_pStringHandler(NULL), m_pVectorHandler(NULL)
		{
			m_pCodeHandler = new MassLynxCodeHandler();
			if (!CheckReturnCode(createRawReaderFromReader(massLynxBaseReader.GetRawReader(), &m_pRawReader, nType),false))
			{
				// need to throw exception
				int nCode(GetLastCode());
				string message(GetLastMessage());
				delete m_pCodeHandler;

				throw MassLynxRawException(nCode, message);
			}

			m_pStringHandler = new MassLynxStringHandler();
			m_pVectorHandler = new MassLynxVectorHandler();
		}

		virtual ~MassLynxBaseReader()
		{
			delete m_pCodeHandler;
			delete m_pStringHandler;
			delete m_pVectorHandler;

			destroyRawReader(m_pRawReader);
			m_pRawReader = NULL;
		}


	public:
		CMassLynxBaseReader const GetRawReader() const { return m_pRawReader; }

		bool CheckReturnCode(const int& nCode, const bool& bThrow = true) const
		{
			return m_pCodeHandler->CheckReturnCode(nCode, bThrow);
		}

		string GetLastMessage() const
		{
			return m_pCodeHandler->GetLastMessage();
		}

		int GetLastCode() const
		{
			return m_pCodeHandler->GetLastCode();
		}

		string ToString(const char* chString) const
		{
			return m_pStringHandler->ToString(chString, false);
		}

		template< typename T>
		vector<T> ToVector(const T* pArray, const int& nSize, const bool& bRelease = false) const
		{
			return m_pVectorHandler->ToVector(pArray, nSize, bRelease);
		}

		template< typename T>
		int ToVector(const T* pArray, const int& nSize, vector<T>& templateVector, const bool& bRelease = false) const
		{
			return m_pVectorHandler->ToVector(pArray, nSize, templateVector, bRelease);
		}

		// split a string
		vector<string> ToVector(const string &s, char delim) const
		{
			return m_pVectorHandler->ToVector(s, delim);
		}

	private:
		CMassLynxBaseReader m_pRawReader;
		MassLynxCodeHandler* m_pCodeHandler;
		MassLynxStringHandler* m_pStringHandler;
		MassLynxVectorHandler* m_pVectorHandler;
	};

	// handle the return codes for the processors
	class MassLynxProcessorCodeHandler : public MassLynxCodeHandler
	{
	public:
		MassLynxProcessorCodeHandler(CMassLynxBaseProcessor pBaseProcessor) : MassLynxCodeHandler(), m_pBaseProcessor(pBaseProcessor) {}
		virtual ~MassLynxProcessorCodeHandler() {}

		virtual string GetLastMessage() const
		{
			char* pErrorMessage(NULL);
			getProcessorMessage(m_pBaseProcessor, GetLastCode(), &pErrorMessage);
			return ToString(pErrorMessage, NULL == m_pBaseProcessor);
		}

	private:
		CMassLynxBaseProcessor m_pBaseProcessor;
	};

	// base processor class
	class MassLynxBaseProcessor
	{
	protected:
		MassLynxBaseProcessor(const MassLynxBaseType& nType) : m_pBaseProcessor(NULL), m_pCodeHandler(NULL), m_pStringHandler(NULL), m_pVectorHandler(NULL)
		{
			const int nCode = createRawProcessor(&m_pBaseProcessor, nType, processor_callback, this);
			m_pCodeHandler = new MassLynxProcessorCodeHandler(m_pBaseProcessor);
			if (!CheckReturnCode(nCode, false))
			{
				// need to throw exception
				int nCode(GetLastCode());
				string message(GetLastMessage());
				delete m_pCodeHandler;

				throw MassLynxRawException(nCode, message);
			}

			m_pStringHandler = new MassLynxStringHandler();
			m_pVectorHandler = new MassLynxVectorHandler();

		}

		virtual ~MassLynxBaseProcessor()
		{
			destroyRawProcessor(m_pBaseProcessor);
			delete m_pCodeHandler;
			delete m_pStringHandler;
			delete m_pVectorHandler;
		}

		bool CheckReturnCode(const int& nCode, const bool& bThrow = true) const
		{
			return m_pCodeHandler->CheckReturnCode(nCode, bThrow);
		}

		template< typename T>
		int ToVector(const T* pArray, const int& nSize, vector<T>& templateVector, const bool& bRelease = false) const
		{
			return m_pVectorHandler->ToVector(pArray, nSize, templateVector, bRelease);
		}

	public:

		string GetLastMessage() const
		{
			return m_pCodeHandler->GetLastMessage();
		}

		int GetLastCode() const
		{
			return m_pCodeHandler->GetLastCode();
		}

		void SetRawData(const MassLynxBaseReader& mlRawReader)
		{
			CheckReturnCode(setRawReader(GetProcessor(), mlRawReader.GetRawReader()));
		}

		void SetRawData(const string& strFullPath)
		{
			CheckReturnCode(setRawPath(GetProcessor(), strFullPath.c_str()));
		}

		void SetRawReader(const MassLynxBaseReader& mlRawReader)
		{
			CheckReturnCode(setRawReader(GetProcessor(), mlRawReader.GetRawReader()));
		}

		void SetRawPath(const string& strFullPath)
		{
			CheckReturnCode(setRawPath(GetProcessor(), strFullPath.c_str()));
		}

		virtual void Progress(const int& percent) {}

	protected:
		CMassLynxBaseProcessor const GetProcessor() const { return m_pBaseProcessor; }

		string ToString(const char* chString) const
		{
			return m_pStringHandler->ToString(chString, false);
		}

	private:
		CMassLynxBaseProcessor m_pBaseProcessor;
		MassLynxProcessorCodeHandler* m_pCodeHandler;
		MassLynxStringHandler* m_pStringHandler;
		MassLynxVectorHandler* m_pVectorHandler;
	};

	static void __stdcall processor_callback(void* pObject, const int& percent)
	{
		// call the virtual update fn
		MassLynxBaseProcessor* self = (MassLynxBaseProcessor*)pObject;
		self->Progress(percent);
	}
}
}
}

/**
*  \endcond
*/