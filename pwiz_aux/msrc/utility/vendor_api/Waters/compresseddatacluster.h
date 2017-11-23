/* 
*******************************************************************

FILENAME:	compresseddatacluster.h

PURPOSE:	Definition compressed data cluster 

COMMENTS:	
						
AUTHOR: 	Keith Richardson
			Copyright (C) 2006 Micromass Ltd - All Rights Reserved

*********************************************************************
*/

#ifndef COMPRESSED_DATA_CLUSTER
#define COMPRESSED_DATA_CLUSTER

#include "cdtdefs.h"

class CompressedDataClusterImp;

/// Class providing access to compressed raw data.
class CDT_API CompressedDataCluster {
public:
	/** Create an empty data cluster.
	*/
	CompressedDataCluster();

	/** Create a data cluster using the supplied raw file.  This overloaded version opens the first function.
	* @param pcRawFileName path to raw data folder to open
	*/
	CompressedDataCluster(const char * pcRawFileName);

	/** Create a data cluster using the supplied raw file and function number.
	* @param pcRawFileName path to raw data folder to open
	* @param nFuncNum function number to open (zero indexed)
	*/
	CompressedDataCluster(const char * pcRawFileName, const int nFuncNum);

	/** Initialise the data cluster with a new raw file, erasing the current contents.  
		This overloaded version opens the first function.
		@param pcRawFileName path to raw data folder to open
	*/
	void Initialise(const char * pcRawFileName);

	/** Initialise the cluster with a new raw file, erasing the current contents.
		@param pcRawFileName path to raw data folder to open
		@param nFuncNum function number to open (zero indexed)
	*/
	void Initialise(const char * pcRawFileName, const int nFuncNum);

	/** Check data cluster initialisation status.
		@return {\bftrue} for valid data cluster, {\bffalse} for empty or invalid data cluster
	*/
	bool isInitialised() const;

	/** Get result of initialisation of data cluster.
		@return CDC result code
	*/
	CDC_RESULT getInitialiseResult() const;

	/** Destructor.
	*/
	virtual ~CompressedDataCluster();

	/** Get the length of the mass axis.  This size should be used to preallocate data arrays.
	*   @param nMassAxisLength the length of the mass axis
	*   @return CDC result code
	*/
	CDC_RESULT getMassAxisLength( int & nMassAxisLength ) const;

	/** Get the mass axis.
		@param pfMass array to hold the mass axis.  This must be allocated with length at least
		as large as the value obtained using {@link getMassAxisLength getMassAxisLength}
		@return CDC result code
	*/
	CDC_RESULT getMassAxis( float * pfMass ) const;

	/** Get number of scans in compressed block.
		@param nScansInBlock number of scans in a block
		@return CDC result code
	*/
	CDC_RESULT getScansInBlock( int & nScansInBlock ) const; 

	/** Get total number of compressed blocks.
		@param nNumberOfBlocks total number of compressed blocks
		@return CDC result code
	*/
	CDC_RESULT getNumberOfBlocks( int & nNumberOfBlocks ) const;

	/** Load a specified block of data (masses and intensities).
		@param nBlockToRead index of block of data to load (zero indexed)
		@return CDC result code
	*/
	CDC_RESULT loadDataBlock( int nBlockToRead );

	/** Read a single scan from a loaded block.  If the block is not loaded an error code will be returned.
		The data arrays should be at least as large as the value obtained using {@link getMassAxisLength getMassAxisLength}, or the 
		behaviour is undefined.  This overloaded version provides access to masses as floats.
		@param nBlock block containing the data to read (zero indexed)
		@param nScanIdx index of scan to read.  0 < nScanIdx < nScansInBlock ( see {@link getScansInBlock getScansInBlock} ).
		@param pfMass preallocated array for masses 
		@param pfIntensity preallocated array for intensities
		@param nScanSize on return with success, the number of data in the specified scan 
		@return CDC result code
	*/
	CDC_RESULT getScan( const int nBlock, const int nScanIdx, float * pfMass, float * pfIntensity, int &nScanSize ) const;

	/** Read a single scan from a loaded block.  If the block is not loaded an error code will be returned.
		The data arrays should be at least as large as the value obtained using {@link getMassAxisLength getMassAxisLength}, or the 
		behaviour is undefined.  This overloaded version provides access to indices into the mass axis array.
		@param nBlock block containing the data to read (zero indexed)
		@param nScanIdx index of scan to read.  0 < nScanIdx < nScansInBlock ( see {@link getScansInBlock getScansInBlock} ).
		@param pnMassIdx preallocated array for mass indices 
		@param pfIntensity preallocated array for intensities
		@param nScanSize on return with success, the number of data in the specified scan 
		@return CDC result code
	*/
	CDC_RESULT getScan( const int nBlock, const int nScanIdx, int * pnMassIdx, float * pfIntensity, int &nScanSize ) const;

	/** Read a single scan from a loaded block with flags.  If the block is not loaded an error code will be returned.
		The data arrays should be at least as large as the value obtained using {@link getMassAxisLength getMassAxisLength}, or the 
		behaviour is undefined.  This overloaded version provides access to indices into the mass axis array.
		@param nBlock block containing the data to read (zero indexed)
		@param nScanIdx index of scan to read.  0 < nScanIdx < nScansInBlock ( see {@link getScansInBlock getScansInBlock} ).
		@param pnMassIdx preallocated array for mass indices 
		@param pfIntensity preallocated array for intensities
		@param pnFlags preallocated array for flags 
		@param nScanSize on return with success, the number of data in the specified scan 
		@return CDC result code
	*/
	CDC_RESULT getScan( const int nBlock, const int nScanIdx, int * pnMassIdx, float * pfIntensity, int * pnFlags, int &nScanSize ) const;

	/** Get retention time of a scan in seconds.
		@param nBlock required block
		@param nScanIdx scan index.  0 < nScanIdx < nScansInBlock ( see {@link getScansInBlock getScansInBlock} ).
		@param fTime on return with success, the RT of the specified scan
		@return CDC result code
	*/
	CDC_RESULT getRT( const int nBlock, const int nScanIdx, float &fTime ) const;

	/** Get the scan error flag for the specified scan.
		@param nBlock required block
		@param nScanIdx scan index.  0 < nScanIdx < nScansInBlock ( see {@link getScansInBlock getScansInBlock} ).
		@param nScanError on return with success, the scan error flag for the specified scan
		@return CDC result code
	*/
	CDC_RESULT getScanError( const int nBlock, const int nScanIdx, int& nScanError ) const;

	/** Get the name of the raw file. The size of the buffer should
		be {\bf_MAX_PATH} otherwise the behaviour is undefined.
	*/
	CDC_RESULT getRawFilePath( char cFileName[] ) const;

	/** Translate an error code into a string.
	    @param nCode error code to translate
		@return result string
	*/
	static const char * resultCodeToString( CDC_RESULT nCode );

private:

	// All calls are forwarded to an implementation object.
	CompressedDataClusterImp * m_pImp;

	// Private copy constructor and assignment operators.
	CompressedDataCluster( const CompressedDataCluster & ){};
	const CompressedDataCluster & operator=( const CompressedDataCluster & ){ return *this; }
};

#endif


