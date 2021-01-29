//-----------------------------------------------------------------------------------------------------
// FILE:			MassLynxRawDefs.h
// DATE:			July 2018
// COPYRIGHT(C):	Waters Corporation 
//					
//-----------------------------------------------------------------------------------------------------

#pragma once

#ifdef _WIN32
#define MLYNX_RAW_API __declspec(dllexport)
#else
#define MLYNX_RAW_API
#define __stdcall
#endif

typedef void* CMassLynxAcquisition;
typedef void* CMassLynxParameters;
typedef void* CMassLynxBaseReader;
typedef void* CMassLynxBaseProcessor;
typedef void* CMassLynxRawWriter;
typedef void* CMassLynxSampleList;
typedef void(__stdcall *ProgressCallBack)(void* pObject, const int& percent);

enum class MassLynxBaseType { SCAN = 1, INFO = 2, CHROM = 3, ANALOG = 4, LOCKMASS = 5, CENTROID = 6, DDA = 7, MSE = 8 };

const unsigned int ION_MODE_BASE = 100;
enum class MassLynxIonMode {
	EI_POS = ION_MODE_BASE, EI_NEG = EI_POS + 1, CI_POS = EI_POS + 2, CI_NEG = EI_POS + 3, FB_POS = EI_POS + 4, FB_NEG = EI_POS + 5, TS_POS = EI_POS + 6, TS_NEG = EI_POS + 7, ES_POS = EI_POS + 8, ES_NEG = EI_POS + 9,
	AI_POS = EI_POS + 10, AI_NEG = EI_POS + 11, LD_POS = EI_POS + 12, LD_NEG = EI_POS + 13, UNINITIALISED = EI_POS + 99
};

const unsigned int FUNCTION_TYPE_BASE = 200;
enum class MassLynxFunctionType {
	MS = FUNCTION_TYPE_BASE, SIR = 1 + MS, DLY = 2 + MS, CAT = 3 + MS, OFF = 4 + MS, PAR = 5 + MS, DAU = 6 + MS, NL = 7 + MS, NG = 8 + MS, MRM = 9 + MS, Q1F = 10 + MS, MS2 = 11 + MS, DAD = 12 + MS,
	TOF = 13 + MS, PSD = 14 + MS, TOFS = 15 + MS, TOFD = 16 + MS, MTOF = 17 + MS, TOFM = 18 + MS, TOFP = 19 + MS, ASVS = 20 + MS, ASMS = 21 + MS, ASVSIR = 22 + MS, ASMSIR = 23 + MS, QUADD = 24 + MS, ASBE = 25 + MS,
	ASB2E = 26 + MS, ASCNL = 27 + MS, ASMIKES = 28 + MS, ASMRM = 29 + MS, ASNRMS = 30 + MS, ASMRMQ = 31 + MS, UNINITIALISED = MS + 99
};

const unsigned int HEADER_ITEM_BASE = 299;
enum class MassLynxHeaderItem {
	VERSION = HEADER_ITEM_BASE + 1, ACQUIRED_NAME = 1 + VERSION, ACQUIRED_DATE = 2 + VERSION, ACQUIRED_TIME = 3 + VERSION, JOB_CODE = 4 + VERSION, TASK_CODE = 5 + VERSION, USER_NAME = 6 + VERSION,
	INSTRUMENT = 7 + VERSION, CONDITIONS = 8 + VERSION, LAB_NAME = 9 + VERSION, SAMPLE_DESCRIPTION = 10 + VERSION, SOLVENT_DELAY = 11 + VERSION, SUBMITTER = 12 + VERSION, SAMPLE_ID = 13 + VERSION,
	BOTTLE_NUMBER = 14 + VERSION, ANALOG_CH1_OFFSET = 15 + VERSION, ANALOG_CH2_OFFSET = 16 + VERSION, ANALOG_CH3_OFFSET = 17 + VERSION, ANALOG_CH4_OFFSET = 18 + VERSION, CAL_MS1_STATIC = 19 + VERSION,
	CAL_MS2_STATIC = 20 + VERSION, CAL_MS1_STATIC_PARAMS = 21 + VERSION, CAL_MS1_DYNAMIC_PARAMS = 22 + VERSION, CAL_MS2_STATIC_PARAMS = 23 + VERSION, CAL_MS2_DYNAMIC_PARAMS = 24 + VERSION,
	CAL_MS1_FAST_PARAMS = 25 + VERSION, CAL_MS2_FAST_PARAMS = 26 + VERSION, CAL_TIME = 27 + VERSION, CAL_DATE = 28 + VERSION, CAL_TEMPERATURE = 29 + VERSION, INLET_METHOD = 30 + VERSION,
	SPARE1 = 31 + VERSION, SPARE2 = 32 + VERSION, SPARE3 = 33 + VERSION, SPARE4 = 34 + VERSION, SPARE5 = 35 + VERSION
};

const unsigned int SCAN_ITEM_BASE = 400;
enum class MassLynxScanItem {
	LINEAR_DETECTOR_VOLTAGE = SCAN_ITEM_BASE + 1, LINEAR_SENSITIVITY = LINEAR_DETECTOR_VOLTAGE + 1, REFLECTRON_LENS_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 2, REFLECTRON_DETECTOR_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 3, REFLECTRON_SENSITIVITY = LINEAR_DETECTOR_VOLTAGE + 4,
	LASER_REPETITION_RATE = LINEAR_DETECTOR_VOLTAGE + 5, COURSE_LASER_CONTROL = LINEAR_DETECTOR_VOLTAGE + 6, FINE_LASER_CONTROL = LINEAR_DETECTOR_VOLTAGE + 7, LASERAIM_XPOS = LINEAR_DETECTOR_VOLTAGE + 8, LASERAIM_YPOS = LINEAR_DETECTOR_VOLTAGE + 9, NUM_SHOTS_SUMMED = LINEAR_DETECTOR_VOLTAGE + 10, NUM_SHOTS_PERFORMED = LINEAR_DETECTOR_VOLTAGE + 11,
	SEGMENT_NUMBER = LINEAR_DETECTOR_VOLTAGE + 12, LCMP_TFM_WELL = LINEAR_DETECTOR_VOLTAGE + 13, SEGMENT_TYPE = LINEAR_DETECTOR_VOLTAGE + 14, SOURCE_REGION1 = LINEAR_DETECTOR_VOLTAGE + 15, SOURCE_REGION2 = LINEAR_DETECTOR_VOLTAGE + 16, REFLECTRON_FIELD_LENGTH = LINEAR_DETECTOR_VOLTAGE + 17, REFLECTRON_LENGTH = LINEAR_DETECTOR_VOLTAGE + 18,
	REFLECTRON_VOLT = LINEAR_DETECTOR_VOLTAGE + 19, SAMPLE_PLATE_VOLT = LINEAR_DETECTOR_VOLTAGE + 20, REFLECTRON_FIELD_LENGTH_ALT = LINEAR_DETECTOR_VOLTAGE + 21, REFLECTRON_LENGTH_ALT = LINEAR_DETECTOR_VOLTAGE + 22, PSD_STEP_MAJOR = LINEAR_DETECTOR_VOLTAGE + 23, PSD_STEP_MINOR = LINEAR_DETECTOR_VOLTAGE + 24, PSD_FACTOR_1 = LINEAR_DETECTOR_VOLTAGE + 25,
	NEEDLE = LINEAR_DETECTOR_VOLTAGE + 49, COUNTER_ELECTRODE_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 50, SAMPLING_CONE_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 51, SKIMMER_LENS = LINEAR_DETECTOR_VOLTAGE + 52, SKIMMER = LINEAR_DETECTOR_VOLTAGE + 53, PROBE_TEMPERATURE = LINEAR_DETECTOR_VOLTAGE + 54, SOURCE_TEMPERATURE = LINEAR_DETECTOR_VOLTAGE + 55, RF_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 56,
	SOURCE_APERTURE = LINEAR_DETECTOR_VOLTAGE + 57, SOURCE_CODE = LINEAR_DETECTOR_VOLTAGE + 58, LM_RESOLUTION = LINEAR_DETECTOR_VOLTAGE + 59, HM_RESOLUTION = LINEAR_DETECTOR_VOLTAGE + 60, COLLISION_ENERGY = LINEAR_DETECTOR_VOLTAGE + 61, ION_ENERGY = LINEAR_DETECTOR_VOLTAGE + 62, MULTIPLIER1 = LINEAR_DETECTOR_VOLTAGE + 63, MULTIPLIER2 = LINEAR_DETECTOR_VOLTAGE + 64, TRANSPORTDC = LINEAR_DETECTOR_VOLTAGE + 65,
	TOF_APERTURE = LINEAR_DETECTOR_VOLTAGE + 66, ACC_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 67, STEERING = LINEAR_DETECTOR_VOLTAGE + 68, FOCUS = LINEAR_DETECTOR_VOLTAGE + 69, ENTRANCE = LINEAR_DETECTOR_VOLTAGE + 70, GUARD = LINEAR_DETECTOR_VOLTAGE + 71, TOF = LINEAR_DETECTOR_VOLTAGE + 72, REFLECTRON = LINEAR_DETECTOR_VOLTAGE + 73, COLLISION_RF = LINEAR_DETECTOR_VOLTAGE + 74, TRANSPORT_RF = LINEAR_DETECTOR_VOLTAGE + 75, SET_MASS = LINEAR_DETECTOR_VOLTAGE + 76,
	COLLISION_ENERGY2 = LINEAR_DETECTOR_VOLTAGE + 77, SET_MASS_CALL_SUPPORTED = LINEAR_DETECTOR_VOLTAGE + 78, SET_MASS_CALIBRATED = LINEAR_DETECTOR_VOLTAGE + 79, SONAR_ENABLED = LINEAR_DETECTOR_VOLTAGE + 80, QUAD_START_MASS = LINEAR_DETECTOR_VOLTAGE + 81, QUAD_STOP_MASS = LINEAR_DETECTOR_VOLTAGE + 82, QUAD_PEAK_WIDTH = LINEAR_DETECTOR_VOLTAGE + 83, REFERENCE_SCAN = LINEAR_DETECTOR_VOLTAGE + 99, USE_LOCKMASS_CORRECTION = LINEAR_DETECTOR_VOLTAGE + 100,
	LOCKMASS_CORRECTION = LINEAR_DETECTOR_VOLTAGE + 101, USETEMP_CORRECTION = LINEAR_DETECTOR_VOLTAGE + 102, TEMP_CORRECTION = LINEAR_DETECTOR_VOLTAGE + 103, TEMP_COEFFICIENT = LINEAR_DETECTOR_VOLTAGE + 104, FAIMS_COMPENSATION_VOLTAGE = LINEAR_DETECTOR_VOLTAGE + 105, TIC_TRACE_A = LINEAR_DETECTOR_VOLTAGE + 106, TIC_TRACE_B = LINEAR_DETECTOR_VOLTAGE + 107,
	RAW_EE_CV = LINEAR_DETECTOR_VOLTAGE + 108, RAW_EE_CE = LINEAR_DETECTOR_VOLTAGE + 110, ACCURATE_MASS = LINEAR_DETECTOR_VOLTAGE + 111, ACCURATE_MASS_FLAGS = LINEAR_DETECTOR_VOLTAGE + 112, SCAN_ERROR_FLAG = LINEAR_DETECTOR_VOLTAGE + 113, DRE_TRANSMISSION = LINEAR_DETECTOR_VOLTAGE + 114, SCAN_PUSH_COUNT = LINEAR_DETECTOR_VOLTAGE + 115,
	RAW_STAT_SWAVE_NORMALISATION_FACTOR = LINEAR_DETECTOR_VOLTAGE + 116, MIN_DRIFT_TIME_CHANNEL = LINEAR_DETECTOR_VOLTAGE + 121, MAX_DRIFT_TIME_CHANNEL = LINEAR_DETECTOR_VOLTAGE + 122, TOTAL_ION_CURRENT = LINEAR_DETECTOR_VOLTAGE + 251, BASE_PEAK_MASS = LINEAR_DETECTOR_VOLTAGE + 252, BASE_PEAK_INTENSITY = LINEAR_DETECTOR_VOLTAGE + 253, PEAKS_IN_SCAN = LINEAR_DETECTOR_VOLTAGE + 254,
	UNINITIALISED = LINEAR_DETECTOR_VOLTAGE + 298
};

enum class MassLynxSampleListItem {
	FILE_NAME = 700, FILE_TEXT = FILE_NAME + 1, MS_FILE = FILE_NAME + 2, MS_TUNE_FILE = FILE_NAME + 3, INLET_FILE = FILE_NAME + 4, INLET_PRERUN = FILE_NAME + 5, INLET_POSTRUN = FILE_NAME + 6,
	INLET_SWITCH = FILE_NAME + 7, AUTO_FILE = FILE_NAME + 8, PROCESS = FILE_NAME + 9, PROCESS_PARAMS = FILE_NAME + 10, PROCESS_OPTIONS = FILE_NAME + 11, ACQU_PROCESS_FILE = FILE_NAME + 12,
	ACQU_PROCESS_PARAMS = FILE_NAME + 13, ACQU_PROCESS_OPTIONS = FILE_NAME + 14, PROCESS_ACTION = FILE_NAME + 15, SAMPLE_LOCATION = FILE_NAME + 16, SAMPLE_GROUP = FILE_NAME + 17, JOB = FILE_NAME + 18,
	TASK = FILE_NAME + 19, USER = FILE_NAME + 20, SUBMITTER = FILE_NAME + 21, CONDITIONS = FILE_NAME + 22, TYPE = FILE_NAME + 23, CONTROL = FILE_NAME + 24, ID = FILE_NAME + 25, CONC_A = FILE_NAME + 26,
	CONC_B = FILE_NAME + 27, CONC_C = FILE_NAME + 28, CONC_D = FILE_NAME + 29, CONC_E = FILE_NAME + 30, CONC_F = FILE_NAME + 31, CONC_G = FILE_NAME + 32, CONC_H = FILE_NAME + 33, CONC_I = FILE_NAME + 34,
	CONC_J = FILE_NAME + 35, CONC_K = FILE_NAME + 36, CONC_L = FILE_NAME + 37, CONC_M = FILE_NAME + 38, CONC_N = FILE_NAME + 39, CONC_O = FILE_NAME + 40, CONC_P = FILE_NAME + 41, CONC_Q = FILE_NAME + 42,
	CONC_R = FILE_NAME + 43, CONC_S = FILE_NAME + 44, CONC_T = FILE_NAME + 45, WAVELENGTH_A = FILE_NAME + 46, WAVELENGTH_B = FILE_NAME + 47, WAVELENGTH_C = FILE_NAME + 48, WAVELENGTH_D = FILE_NAME + 49,
	WAVELENGTH_E = FILE_NAME + 50, WAVELENGTH_F = FILE_NAME + 51, WAVELENGTH_G = FILE_NAME + 52, WAVELENGTH_H = FILE_NAME + 53, WAVELENGTH_I = FILE_NAME + 54, WAVELENGTH_J = FILE_NAME + 55,
	MASS_A = FILE_NAME + 56, MASS_B = FILE_NAME + 57, MASS_C = FILE_NAME + 58, MASS_D = FILE_NAME + 59, MASS_E = FILE_NAME + 60, MASS_F = FILE_NAME + 61, MASS_G = FILE_NAME + 62, MASS_H = FILE_NAME + 63,
	MASS_I = FILE_NAME + 64, MASS_J = FILE_NAME + 65, MASS_K = FILE_NAME + 66, MASS_L = FILE_NAME + 67, MASS_M = FILE_NAME + 68, MASS_N = FILE_NAME + 69, MASS_O = FILE_NAME + 70, MASS_P = FILE_NAME + 71,
	MASS_Q = FILE_NAME + 72, MASS_R = FILE_NAME + 73, MASS_S = FILE_NAME + 74, MASS_T = FILE_NAME + 75, MASS_U = FILE_NAME + 76, MASS_V = FILE_NAME + 77, MASS_W = FILE_NAME + 78, MASS_X = FILE_NAME + 79,
	MASS_Y = FILE_NAME + 80, MASS_Z = FILE_NAME + 81, MASS_AA = FILE_NAME + 82, MASS_BB = FILE_NAME + 83, MASS_CC = FILE_NAME + 84, MASS_DD = FILE_NAME + 85, FRACTION_FILE = FILE_NAME + 86,
	FRACTION_1 = FILE_NAME + 87, FRACTION_2 = FILE_NAME + 88, FRACTION_3 = FILE_NAME + 89, FRACTION_4 = FILE_NAME + 90, FRACTION_5 = FILE_NAME + 91, FRACTION_6 = FILE_NAME + 92, FRACTION_7 = FILE_NAME + 93,
	FRACTION_8 = FILE_NAME + 94, FRACTION_9 = FILE_NAME + 95, FRACTION_10 = FILE_NAME + 96, FRACTION_BOOLEAN_LOGIC = FILE_NAME + 97, FRACTION_START = FILE_NAME + 98, INJ_VOL = FILE_NAME + 99,
	STOCK_DIL = FILE_NAME + 100, USER_DIVISOR_1 = FILE_NAME + 101, USER_FACTOR_1 = FILE_NAME + 102, USER_FACTOR_2 = FILE_NAME + 103, USER_FACTOR_3 = FILE_NAME + 104, SPARE_1 = FILE_NAME + 105,
	SPARE_2 = FILE_NAME + 106, SPARE_3 = FILE_NAME + 107, SPARE_4 = FILE_NAME + 108, SPARE_5 = FILE_NAME + 109, HPLC_FILE = FILE_NAME + 110, QUAN_REF = FILE_NAME + 111, AUTO_ADDITION = FILE_NAME + 112,
	MOLFILE = FILE_NAME + 113, SUBJECTTEXT = FILE_NAME + 114, SUBJECTTIME = FILE_NAME + 115, METH_DB = FILE_NAME + 116, CURVE_DB = FILE_NAME + 117
};

enum class MassLynxScanType { MS1 = 1, MS2 = MS1 + 2, UNINITIALISED = MS1 + 9 };

enum class MassLynxAcquisitionType { DDA = 0, MSE = DDA + 1, HDDDA = DDA + 2, HDMSE = DDA + 3, SONAR = DDA + 4, UNINITIALISED = DDA + 99 };

enum class ThresholdParameters {
	VALUE = 0, THRESHTYPE = 1
};


enum class ThresholdType2 {
	ABSOLUTE_1 = 0, RELATIVE_2 = 1
};


enum class SmoothParameter {
	NUMBER = 1, WIDTH = NUMBER + 1, SMOOTHTYPE = NUMBER + 2

};
enum class SmoothType {
	MEAN = 1, MEDIAN = MEAN + 1, SAVITZKY_GOLAY = MEAN + 2
};


enum class LockMassParameter {
	MASS = 1000, TOLERANCE = MASS + 1, FORCE = MASS + 2
};

enum class FunctionDefinition {
	CONTINUUM = 1100, IONMODE = CONTINUUM + 1, FUNCTIONTYPE = CONTINUUM + 2, STARTMASS = CONTINUUM + 3, ENDMASS = CONTINUUM + 4
};

enum class AnalogParameter
{
	DESCRIPTION = 1200 + 1, UNITS = DESCRIPTION + 2, TYPE = DESCRIPTION + 3
};

enum class AnalogTraceType
{
	ANALOG = 1250, ELSD = ANALOG + 1, READBACK = ANALOG + 2
};

enum class AutoLynxStatus
{
	QUEUED = 1300, PROCESSED = QUEUED + 1, FAILED = QUEUED + 2, NOTFOUND = QUEUED + 3, UNINITIALISED = QUEUED + 9
};


#ifdef __cplusplus
extern "C" {
#endif
	// error code support
	MLYNX_RAW_API int __stdcall getErrorMessage(const int nErrorCode, char** ppErrorMessage);
	MLYNX_RAW_API int __stdcall releaseMemory(void* pMemory);

	// Base reader
	MLYNX_RAW_API int __stdcall createRawReaderFromPath(const char *path, CMassLynxBaseReader* mlRawReader, MassLynxBaseType nType);
	MLYNX_RAW_API int __stdcall createRawReaderFromReader(CMassLynxBaseReader mlSourceRawReader, CMassLynxBaseReader* mlRawReader, MassLynxBaseType nType);
	MLYNX_RAW_API int __stdcall destroyRawReader(CMassLynxBaseReader mlRawReader);

	// Information reader
	MLYNX_RAW_API int __stdcall getFunctionCount(CMassLynxBaseReader mlInfoReader, unsigned int* pFunctions);
	MLYNX_RAW_API int __stdcall getScanCount(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, unsigned int* pScans);
	MLYNX_RAW_API int __stdcall getAcquisitionMassRange(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, const int nWhichSegment, float* lowMass, float* highMass);
	MLYNX_RAW_API int __stdcall getAcquisitionTimeRange(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, float* startTime, float* endTime);
	MLYNX_RAW_API int __stdcall getFunctionType(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, MassLynxFunctionType* pFunctionType);
	MLYNX_RAW_API int __stdcall getFunctionTypeString(CMassLynxBaseReader mlInfoReader, const MassLynxFunctionType functionType, char** chFunctionType);
	MLYNX_RAW_API int __stdcall isContinuum(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, bool& bContinuum);
	MLYNX_RAW_API int __stdcall getIonMode(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, MassLynxIonMode* ionMode);
	MLYNX_RAW_API int __stdcall getIonModeString(CMassLynxBaseReader mlInfoReader, const MassLynxIonMode ionMode, char** chIonMode);
	MLYNX_RAW_API int __stdcall getRetentionTime(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, const int nWhichScan, float* fRT);
	MLYNX_RAW_API int __stdcall getDriftTime(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, const int nWhichDrift, float* fRT);
	MLYNX_RAW_API int __stdcall getDriftTime_CCS(const CMassLynxBaseReader mlInfoReader, const float ccs, const float mass, const int charge, float* driftTime);
	MLYNX_RAW_API int __stdcall getCollisionalCrossSection(const CMassLynxBaseReader mlInfoReader, const float driftTime, const float mass, const int charge, float* fCCS);
	MLYNX_RAW_API int __stdcall getDriftScanCount(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, unsigned int* pScans);
	MLYNX_RAW_API int __stdcall getMRMCount(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, int* pMRMs);
	MLYNX_RAW_API int __stdcall isLockMassCorrected(CMassLynxBaseReader mlInfoReader, bool* pIsApplied);
	MLYNX_RAW_API int __stdcall canLockMassCorrect(CMassLynxBaseReader mlInfoReader, bool* pCanApply);
	MLYNX_RAW_API int __stdcall getAcquisitionType(CMassLynxBaseReader mlInfoReader, MassLynxAcquisitionType* acquisitionType);
	MLYNX_RAW_API int __stdcall getIndexRange(CMassLynxBaseReader mlRawReader, const int nWhichFunction, const float preCursorMass, const float preCursorTolerance, int* pStartIndex, int* pEndIndex);
	MLYNX_RAW_API int __stdcall getHeaderItemValue(const CMassLynxBaseReader mlRawReader, const MassLynxHeaderItem* pItems, const int nItems, CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall getScanItemValue(const CMassLynxBaseReader mlInfoReader, const int nWhichFunction, const int nWhichScan, const MassLynxScanItem* pItems, const int nItems, CMassLynxParameters parameters);
	MLYNX_RAW_API int __stdcall getScanItemName(const CMassLynxBaseReader mlInfoReader, const MassLynxScanItem* pItems, const int nItems, CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall getScanItemsInFunction(const CMassLynxBaseReader mlInfoReader, const int nWhichFunction, CMassLynxParameters parameters);

	// deprecate these
	MLYNX_RAW_API int __stdcall getHeaderItems(CMassLynxBaseReader mlInfoReader, const MassLynxHeaderItem* pItems, char** ppItems, int nItems, char* pDelimiter);
	MLYNX_RAW_API int __stdcall getScanItems(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, const int nWhichScan, const MassLynxScanItem* pItems, char** ppInfo, int nItems, char* pDelimiter);
	MLYNX_RAW_API int __stdcall getScanItemNames(CMassLynxBaseReader mlInfoReader, const MassLynxScanItem* pItems, char** ppStatsName, int nItems, char* pDelimiter);
	MLYNX_RAW_API int __stdcall getItemsInFunction(CMassLynxBaseReader mlInfoReader, const int nWhichFunction, MassLynxScanItem** ppItems, int* pSize);
	MLYNX_RAW_API int __stdcall getHeaderItemsDelim(const CMassLynxBaseReader mlInfoReader, const MassLynxHeaderItem* pItems, char** ppItems, int nItems);

	// Scan Reader functions
	MLYNX_RAW_API int __stdcall readScan(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, float** ppMasses, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall readScanFlags(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, float** pMasses, float** pIntensities, char** pFlags, int* pSize);
	MLYNX_RAW_API int __stdcall readDriftScan(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, const int nWhichDrift, float** ppMasses, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall readDaughterScan(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, float** ppMasses, float** ppIntensities, float** ppProductMasses, int* pSize, int* pProductSize);
	MLYNX_RAW_API int __stdcall readDriftScanIndex(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, const int nWhichDrift, int** ppMasses, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall readDriftScanFlagsIndex(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, const int nWhichDrift, int** ppMasses, float** ppIntensities, char** pFlags, int* pSize);
	MLYNX_RAW_API int __stdcall getDriftMassScale(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nWhichScan, float** ppMasses, int* pSize);

	MLYNX_RAW_API int __stdcall extractScan(CMassLynxBaseReader mlRawScanreader, const int nWhichFunction, const int nStartScan, const int nEndScan, const int nWhichDrift, int** ppMasses, float** ppIntensities, int* pSize);

	// Chromatogram functions
	MLYNX_RAW_API int __stdcall readBPIChromatogram(CMassLynxBaseReader mlChromatogramReader, int nWhichFunction, float** ppTimes, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall readTICChromatogram(CMassLynxBaseReader mlChromatogramReader, int nWhichFunction, float** ppTimes, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall readMassChromatograms(CMassLynxBaseReader mlChromatogramReader, const int nWhichFunction, const float* massList, const int massListSize, float** ppTimes, float **ppIntensities, const float massWindow, const bool bDaughters, int* pSize);
	MLYNX_RAW_API int __stdcall readSonarMassChromatogram(CMassLynxBaseReader mlChromatogramReader, const int nWhichFunction, const float preCursorMass, const float pMass, float** ppTimes, float** ppIntensities, const float precursorMassWindow, const float massWindow, int* pSize);
	MLYNX_RAW_API int __stdcall readMRMChromatograms(CMassLynxBaseReader mlChromatogramReader, const int nWhichFunction, const int* mrmList, const int nMRM, float** ppTimes, float ** ppIntensities, int* pSize);

	// Analog reader functions
	MLYNX_RAW_API int __stdcall getChannelCount(const CMassLynxBaseReader mlAnalogReader, int* nChannels);
	MLYNX_RAW_API int __stdcall readChannel(const CMassLynxBaseReader mlAnalogReader, const int nWhichChannel, const float** pTimes, const float** pIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall getChannelDesciption(const CMassLynxBaseReader mlAnalogReader, const int nWhichChannel, char** ppDescription);
	MLYNX_RAW_API int __stdcall getChannelUnits(const CMassLynxBaseReader mlAnalogReader, const int nWhichChannel, char** ppUnits);

	// Base processor
	MLYNX_RAW_API int __stdcall createRawProcessor(CMassLynxBaseProcessor* mlRawProcessor, MassLynxBaseType nType, ProgressCallBack pCallback, void* pCaller);
	MLYNX_RAW_API int __stdcall destroyRawProcessor(CMassLynxBaseProcessor mlRawProcessor);
	MLYNX_RAW_API int __stdcall getProcessorMessage(CMassLynxBaseProcessor mlRawProcessor, const int nCode, char** ppMessage);
	MLYNX_RAW_API int __stdcall setRawReader(CMassLynxBaseProcessor mlRawProcessor, CMassLynxBaseReader mlRawReader);
	MLYNX_RAW_API int __stdcall setRawPath(CMassLynxBaseProcessor mlRawProcessor, const char *path);
	MLYNX_RAW_API int __stdcall setProcessorCallBack(CMassLynxBaseProcessor mlRawProcessor, ProgressCallBack pCallback, void* pCaller);

	// Lock mass processor
	MLYNX_RAW_API int __stdcall setLockMassParameters(const CMassLynxBaseProcessor mlLockMassProcessor, const CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall getLockMassParameter(CMassLynxBaseProcessor mlLockMassProcessor, char** ppParameters);
	MLYNX_RAW_API int __stdcall lockMassCorrect(const CMassLynxBaseProcessor mlLockMassProcessor, bool* pApplied);
	MLYNX_RAW_API int __stdcall removeLockMassCorrection(CMassLynxBaseProcessor mlLockMassProcessor);
	MLYNX_RAW_API int __stdcall getLockMassCandidates(CMassLynxBaseProcessor mlLockMassProcessor, float** ppMasses, float** ppIntensities, int* nSize);
	MLYNX_RAW_API int __stdcall LMP_isLockMassCorrected(const CMassLynxBaseProcessor mlLockMassProcessor, int* applied);
	MLYNX_RAW_API int __stdcall LMP_canLockMassCorrect( const CMassLynxBaseProcessor mlLockMassProcessor, int* canApply);
	MLYNX_RAW_API int __stdcall getLockMassValues(CMassLynxBaseProcessor mlLockMassProcessor, float* pMass, float* pTolerance);
	MLYNX_RAW_API int __stdcall getLockMassCorrection(CMassLynxBaseProcessor mlLockMassProcessor, const float retentionTime, float* pGain);
	
	// Scan processor
	MLYNX_RAW_API int __stdcall getScan(CMassLynxBaseProcessor mlScanProcessor, float** ppMasses, float** ppIntensities, int* nSize);

	// combine
	MLYNX_RAW_API int __stdcall combineScan(CMassLynxBaseProcessor mlScanProcessor, const int nWhichFunction, const int* pScans, const int nScans);

	// smooth
	MLYNX_RAW_API int __stdcall smoothScan(CMassLynxBaseProcessor mlScanProcessor);
	MLYNX_RAW_API int __stdcall setSmoothParameter(CMassLynxBaseProcessor mlScanProcessor, const CMassLynxParameters pParameters);
	//	MLYNX_RAW_API int __stdcall setSmoothParameter(CMassLynxBaseProcessor mlScanProcessor, const char* pParameters);
	//	MLYNX_RAW_API int __stdcall getSmoothParameter(CMassLynxBaseProcessor mlScanProcessor, char** ppParameters);

	// centroid
	MLYNX_RAW_API int __stdcall centroidScan(CMassLynxBaseProcessor mlScanProcessor);

	// theshold
	MLYNX_RAW_API int __stdcall thresholdScan(CMassLynxBaseProcessor mlScanProcessor);
	MLYNX_RAW_API int __stdcall setThresholdParameter(const CMassLynxBaseProcessor mlSpectrumProcessor, const CMassLynxParameters pParameters);
	//	MLYNX_RAW_API int __stdcall setThresholdParameter(CMassLynxBaseProcessor mlScanProcessor, const char* pParameters);
	//	MLYNX_RAW_API int __stdcall getThresholdParameter(CMassLynxBaseProcessor mlScanProcessor, char** ppParameters);

		// chromatogram processor
	MLYNX_RAW_API int __stdcall getProcessedChromatograms(CMassLynxBaseProcessor mlChromatogramProcessor, const int nChromatograms, float** ppTimes, float** const ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall getProcessedChromatogramCount(CMassLynxBaseProcessor mlChromatogramProcessor, int* pChromatograms);
	MLYNX_RAW_API int __stdcall extract(CMassLynxBaseProcessor mlChromatogramProcessor);
	MLYNX_RAW_API int __stdcall setExtractParameter(CMassLynxBaseProcessor mlChromatogramProcessor, const char* pParameters);
	MLYNX_RAW_API int __stdcall getExtractParameter(CMassLynxBaseProcessor mlChromatogramProcessor, char** ppParameters);

	// smooth
	MLYNX_RAW_API int __stdcall smoothChromatogram(CMassLynxBaseProcessor mlChromatogramProcessor);
	MLYNX_RAW_API int __stdcall setChromatogramSmoothParameter(CMassLynxBaseProcessor mlChromatogramProcessor, const char* pParameters);
	MLYNX_RAW_API int __stdcall getChromatogramSmoothParameter(CMassLynxBaseProcessor mlChromatogramProcessor, char** ppParameters);

	// Parameter helper
	MLYNX_RAW_API int __stdcall createParameters(CMassLynxParameters* mlParameters);
	MLYNX_RAW_API int __stdcall createParametersFromParameters(CMassLynxParameters mlSourceParameters, CMassLynxParameters* mlParameters);
	MLYNX_RAW_API int __stdcall destroyParameters(CMassLynxParameters mlParameters);
	MLYNX_RAW_API int __stdcall setParameterValue(const CMassLynxParameters mlParameters, const int nKey, const char* pValue);
	MLYNX_RAW_API int __stdcall getParameterValue(const CMassLynxParameters mlParameters, const int nKey, char** ppValue);
	MLYNX_RAW_API int __stdcall getParameterKeys(const CMassLynxParameters mlParameters, int** ppKeys, int* pSize);

	// RAW folder processes
	MLYNX_RAW_API int __stdcall centroidRaw(CMassLynxBaseProcessor mlRawProcessor, const char *path);

	// Raw Writer
	MLYNX_RAW_API int __stdcall createRawWriter(CMassLynxRawWriter* mlRawWriter, const char* pTarget);
	MLYNX_RAW_API int __stdcall destroyRawWriter(CMassLynxRawWriter mlRawWriter);
	MLYNX_RAW_API int __stdcall appendRawFunction(const CMassLynxRawWriter mlRawWriter, const CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall appendRawFunctionWithStats(const CMassLynxRawWriter mlRawWriter, const CMassLynxParameters pParameters, const MassLynxScanItem* pStats, const int size);
	MLYNX_RAW_API int __stdcall appendRawScan(CMassLynxRawWriter mlRawWriter, const float* pMasses, const float* pIntensities, const int nSize, const float fRT);
	MLYNX_RAW_API int __stdcall appendRawScanWithStats(const CMassLynxRawWriter mlRawWriter, const float* pMasses, const float* pIntensities, const int nSize, const float fRT, const CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall setRawHeaderItems(const CMassLynxRawWriter mlRawWriter, const CMassLynxParameters pParameters);
	MLYNX_RAW_API int __stdcall appendRawTrace(const CMassLynxRawWriter mlRawWriter, const CMassLynxParameters pParameters, const float* pTimes, const float* pIntensities, const int nSize);
	MLYNX_RAW_API int __stdcall closeRawWriter(CMassLynxRawWriter mlRawWriter);

	// DDA processor
	MLYNX_RAW_API int __stdcall ddaRaw(const CMassLynxBaseProcessor mlRawProcessor, float** ppRT, float** ppMasses, int* nSize);
	MLYNX_RAW_API int __stdcall ddaGetScan(const CMassLynxBaseProcessor mlRawProcessor, const int nWhichPrecursor, const MassLynxScanType scanType, float** ppMasses, float** ppIntensities, int* pSize);
	MLYNX_RAW_API int __stdcall ddaGetNextScan(const CMassLynxBaseProcessor mlRawProcessor, float* pRT, MassLynxScanType* scanType, float* pSetMass, float** ppMasses, float** ppIntensities, int* pSize, bool* pNext);
	MLYNX_RAW_API int __stdcall ddaResetScan(const CMassLynxBaseProcessor mlRawProcessor);

	// MSe processor
	MLYNX_RAW_API int __stdcall mseGetNextScan(const CMassLynxBaseProcessor mlRawProcessor, float* pRT, MassLynxScanType* scanType, float** ppMasses, float** ppIntensities, int* pSize, bool* pNext);
	MLYNX_RAW_API int __stdcall mseResetScan(const CMassLynxBaseProcessor mlRawProcessor);

	// Sample list
	MLYNX_RAW_API int __stdcall createSampleList(CMassLynxSampleList* mlSampleList);
	MLYNX_RAW_API int __stdcall destroySampleList(CMassLynxSampleList mlSampleList);
	MLYNX_RAW_API int __stdcall addSampleListRow(const CMassLynxSampleList mlSampleList, const CMassLynxParameters pRow);
	MLYNX_RAW_API int __stdcall sampleListToString(const CMassLynxSampleList mlSampleList, char** ppString);

	// Acquisition
	MLYNX_RAW_API int __stdcall createAcquisition(CMassLynxAcquisition* mlAcquisition);
	MLYNX_RAW_API int __stdcall destroyAcquisition(CMassLynxAcquisition mlAcquisition);
	MLYNX_RAW_API int __stdcall getAcquisitionStatus(const CMassLynxAcquisition mlAcquisition, const char *path, int* status);

#ifdef __cplusplus
}
#endif