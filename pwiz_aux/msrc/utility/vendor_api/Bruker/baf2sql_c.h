#ifndef DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_C_H
#define DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_C_H

/** \file
 *
 * Definition of the C API for Bruker's SQL-assisted BAF-file reader
 * library. See 'baf2sql.cs' for an example C# program using this API.
 *
 * To use this library under Windows, you need to have the "Visual C++
 * Redistributable for Visual Studio 2012" installed on your system.
 *
 */

#ifdef _MSC_VER
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

    /// ---------------------------------------------------------------
    /// SQL access
    /// ---------------------------------------------------------------

    /// For the specified BAF file 'baf_filename', return the full
    /// path name of a valid corresponding SQLite cache in the
    /// provided buffer.
    ///
    /// The database is either an existing one that is quickly checked
    /// for consistency, or a newly generated one, either next to the
    /// BAF or in a temporary system directory.
    ///
    /// - If there is an existing SQLite cache in the same directory
    ///   as the BAF, we try to use it. However, if the BAF has
    ///   changed since creation of the SQLite cache, or if the schema
    ///   version is not compatible with the one specified for this
    ///   version of BAF2SQL, it will simply delete the existing cache
    ///   and replace it with a freshly generated one. If the
    ///   directory in which the BAF resides is not writable, the new
    ///   SQLite cache will be generated in a temporary system
    ///   directory.
    ///
    /// - If no SQLite cache is present next to the BAF file, it will
    ///   generate a new one. If the directory in which the BAF
    ///   resides is not writable, the SQLite cache will be generated
    ///   in a temporary system directory.
    ///
    /// On success, returns the same value of 'sql_filename_buflen'
    /// that you passed in. If that value was too low (i.e., your
    /// buffer was too short to hold the filename), returns a larger
    /// value (the minimum required buffer length); you can then call
    /// the function again with a buffer of the correct size (don't
    /// worry about performance; the SQLite cache will only be
    /// generated once).
    ///
    /// On error, returns 0. You can then use baf2sql_get_last_error_
    /// string() to obtain a string describing the problem.
    ///
    /// Note on thread safety: you may get into trouble when trying to
    /// call this function concurrently for the same BAF file;
    /// concurrency for different BAF files shouldn't be a problem.
    ///
    /// \param sql_filename_buf a pointer to a buffer into which the
    /// SQLite database filename will be copied. May be NULL only if
    /// 'sql_filename_buflen' is zero.
    ///
    /// \param sql_filename_buflen the maximum number of bytes that
    /// may be written to the buffer pointed to by 'sql_filename_buf'.
    ///
    /// \param baf_filename the path name of an 'analysis.baf' file
    /// (note: it is not sufficient to only specify the .d directory
    /// here).
    ///
    DllExport uint32_t baf2sql_get_sqlite_cache_filename (
        char *sql_filename_buf,
        uint32_t sql_filename_buflen,
        const char *baf_filename);

    /// ---------------------------------------------------------------
    /// Binary array-data access
    /// ---------------------------------------------------------------

    /// Open binary array storage for the specified BAF file. Note:
    /// this will internally generate or open the SQLite cache
    /// according to the description in
    /// baf2sql_get_sqlite_cache_filename().
    ///
    /// Notes on calibration: the BAF file contains m/z data on the
    /// basis of the current calibration of the acquisition
    /// instrument. Using the Bruker DataAnalysis software, more
    /// sophisticated post-acquisition calibration can be performed;
    /// that improved calibration status will be stored in a file
    /// called 'Calibrator.ami' which is in the same directory as the
    /// original BAF. Baf2sql_C takes into account this information
    /// and returns properly calibrated data UNLESS you set the
    /// parameter ignore_calibrator_ami to the value 1 -- in that case
    /// the raw calibration is always returned, and the contents of
    /// Calibrator.ami are ignored.
    ///
    /// On success, returns a non-zero instance handle that needs to
    /// be passed to subsequent baf2sql_array_XXX() calls, in
    /// particular to the required call to
    /// baf2sql_array_close_storage().
    ///
    /// On failure, returns 0, and you can use
    /// baf2sql_get_last_error_string() to obtain a string describing
    /// the problem.
    ///
    /// \param ignore_calibrator_ami if set to 1, will always return
    /// raw m/z data from the BAF file; if set to 0 and a
    /// Calibrator.ami file is present in the same directory as the
    /// BAF, will use the calibration status stored there.
    ///
    /// \param 
    ///
    /// Note on thread safety: you can open as many binary storage
    /// interfaces as you want. Different threads may NOT access the
    /// same storage concurrently; accessing different storages
    /// concurrently is fine, though.
    ///
    DllExport uint64_t baf2sql_array_open_storage (
        int ignore_calibrator_ami,
        const char *baf_filename);

    /// Get the number of elements stored in an array.
    ///
    /// On success, returns 1, and the variable pointed to by
    /// 'num_elements' contains the number of elements of the array
    /// with the ID 'id'. It may be zero, indicating that the BAF file
    /// stores no data for the requested component of the spectrum
    /// (sorry, the SQL table doesn't give you that info in advance).
    ///
    /// On failure, returns 0, and you can use
    /// baf2sql_get_last_error_string() to obtain a string describing
    /// the problem.
    ///
    /// Note: different threads may NOT access the same storage handle
    /// concurrently.
    ///
    DllExport int baf2sql_array_get_num_elements (uint64_t handle,
                                                  uint64_t id,
                                                  uint64_t *num_elements);

    /// Read array into a user-provided buffer. The data will be
    /// converted to the requested type on the fly. The provided
    /// buffer must be large enough to hold the entire array.
    ///
    /// On success, returns 1. On failure, returns 0, and you can use
    /// baf2sql_get_last_error_string() to obtain a string describing
    /// the problem.
    ///
    /// Note: different threads may NOT access the same storage handle
    /// concurrently.
    ///
    DllExport int baf2sql_array_read_double (uint64_t handle, uint64_t id, double buf[]);
    DllExport int baf2sql_array_read_float (uint64_t handle, uint64_t id, float buf[]);
    DllExport int baf2sql_array_read_uint32 (uint64_t handle, uint64_t id, uint32_t buf[]);

    /// Close binary array storage.
    DllExport void baf2sql_array_close_storage (uint64_t handle);

    /// ---------------------------------------------------------------
    /// Misc
    /// ---------------------------------------------------------------

    /// Return the last error as a string (thread-local).
    ///
    /// \param buf pointer to a buffer into which the error string
    /// will be written.
    ///
    /// \param len length of the buffer
    ///
    /// \returns the actual length of the error message (including the
    /// final zero byte). If this is longer than the input parameter
    /// 'len', you know that the returned error string was truncated
    /// to fit in the provided buffer.
    ///
    DllExport uint32_t baf2sql_get_last_error_string (char *buf, uint32_t len);

    /// Set the number of threads that BAF2SQL is allowed to use
    /// internally. [The index->mass transformation is internally
    /// parallelized using OpenMP; this call is simply forwarded to
    /// omp_set_num_threads()].
    ///
    /// \param n number of threads to use (n must be >= 1).
    ///
    DllExport void baf2sql_set_num_threads (uint32_t n);

#ifdef __cplusplus
}
#endif

#endif // DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_C_H

/* Local Variables:  */
/* mode: c           */
/* End:              */
