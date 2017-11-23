#ifndef DE_BDAL_CPP_IO_TIMSDATA_H
#define DE_BDAL_CPP_IO_TIMSDATA_H

/** \file
 *
 * Definition of the public C "Mini API" for reading Bruker's TIMS raw-data format.
 *
 */

#ifdef _MSC_VER
  #ifdef DE_BDAL_CPP_IO_TIMSDATA_BUILDING_DLL
    #define BdalTimsdataDllSpec __declspec(dllexport)
  #else
    #define BdalTimsdataDllSpec __declspec(dllimport)
  #endif
#else
  #define BdalTimsdataDllSpec
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    /// Open data set.
    ///
    /// On success, returns a non-zero instance handle that needs to be passed to
    /// subsequent API calls, in particular to the required call to tims_close().
    ///
    /// On failure, returns 0, and you can use tims_get_last_error_string() to obtain a
    /// string describing the problem.
    ///
    /// \param analysis_directory_name the name of the directory in the file system that
    /// contains the analysis data, in UTF-8 encoding.
    ///
    /// \param use_recalibrated_state if non-zero, use the current recalibrated state of
    /// the analysis, if there is one [not supported at the moment]; if zero, use the
    /// original calibration written during acquisition time.
    ///
    BdalTimsdataDllSpec uint64_t tims_open (
        const char *analysis_directory_name,
        uint32_t use_recalibrated_state
        );

    /// Close data set.
    ///
    /// \param handle obtained by tims_open(); passing 0 is ok and has no effect.
    ///
    BdalTimsdataDllSpec void tims_close (uint64_t handle);

    /// Return the last error as a string (thread-local).
    ///
    /// \param buf pointer to a buffer into which the error string will be written.
    ///
    /// \param len length of the buffer
    ///
    /// \returns the actual length of the error message (including the final zero
    /// byte). If this is longer than the input parameter 'len', you know that the
    /// returned error string was truncated to fit in the provided buffer.
    ///
    BdalTimsdataDllSpec uint32_t tims_get_last_error_string (char *buf, uint32_t len);

    /// Returns 1 if the raw data have been recalibrated after acquisition, e.g. in the
    /// DataAnalysis software. Note that masses and 1/K0 values in the raw-data SQLite
    /// file are always in the raw calibration state, not the recalibrated state.
    ///
    BdalTimsdataDllSpec uint32_t tims_has_recalibrated_state (uint64_t handle);

    /// Read a range of scans from a single frame.
    ///
    /// Output layout: (N = scan_end - scan_begin = number of requested scans)
    ///   N x uint32_t: number of peaks in the N requested scans
    ///   N x (two uint32_t arrays: first indices, then intensities)
    ///
    /// Note: different threads must not read scans from the same storage handle
    /// concurrently.
    ///
    /// \returns 0 on error, otherwise the required buffer length in bytes (if this is
    /// larger than the provided length, you are missing data).
    ///
    BdalTimsdataDllSpec uint32_t tims_read_scans (
        uint64_t handle,
        int64_t tims_id,       //< from .tdf SQLite: Frames.TimsId
        uint32_t scan_begin,   //< first scan number to read (inclusive)
        uint32_t scan_end,     //< last scan number (exclusive)
        void *buf,             //< destination buffer allocated by user
        uint32_t len           //< length of buffer (in bytes)
        );

    /// -----------------------------------------------------------------------------------
    ///
    /// Conversion functions coming up. All these functions share the same signature (see
    /// typedef 'BdalTimsConversionFunction'). They all return 1 on success, 0 on failure.
    ///
    /// -----------------------------------------------------------------------------------
    
    typedef uint32_t BdalTimsConversionFunction (
        uint64_t handle,
        int64_t frame_id,      //< from .tdf SQLite: Frames.Id
        const double *index,   //<  in: array of values
        double *mz,            //< out: array of values
        uint32_t cnt           //< number of values to convert (arrays must have
                               //< corresponding size)
        );
    
    /// m/z transformation: convert back and forth between (possibly non-integer) index
    /// values and m/z values.
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_index_to_mz;
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_mz_to_index;

    /// mobility transformation: convert back and forth between (possibly non-integer)
    /// scan numbers and 1/K0 values.
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_scannum_to_oneoverk0;
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_oneoverk0_to_scannum;

    /// mobility transformation: convert back and forth between (possibly non-integer)
    /// scan numbers and TIMS voltages.
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_scannum_to_voltage;
    BdalTimsdataDllSpec BdalTimsConversionFunction tims_voltage_to_scannum;
    
    /// Set the number of threads that this DLL is allowed to use internally. [The
    /// index<->m/z transformation is internally parallelized using OpenMP; this call is
    /// simply forwarded to omp_set_num_threads()].
    ///
    /// \param n number of threads to use (n must be >= 1).
    ///
    BdalTimsdataDllSpec void tims_set_num_threads (uint32_t n);

#ifdef __cplusplus
}
#endif

#endif //  DE_BDAL_CPP_IO_TIMSDATA_H

/* Local Variables:  */
/* mode: c           */
/* End:              */
