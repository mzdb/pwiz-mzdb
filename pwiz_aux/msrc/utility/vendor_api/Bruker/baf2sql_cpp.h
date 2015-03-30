#ifndef DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_CPP_H
#define DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_CPP_H

/** \file
 *
 * Light-weight header-only C++ layer wrapping the C API for Bruker's
 * SQL-assisted BAF-file reader library.
 *
 * See 'bafscan.cpp' for an example C++ program using this API.
 *
 * See 'baf2sql_c.h' for more details about the underlying C API.
 *
 */

#include <stdexcept>
#include <vector>
#include <string>
#include <cstdint>
#include <limits>

#include "boost/throw_exception.hpp"
#include "boost/noncopyable.hpp"
#include "boost/shared_array.hpp"

#include "baf2sql_c.h" // fundamental C API

namespace baf2sql
{

    /// Throw last Baf2Sql error string as an exception.
    inline void throwLastBaf2SqlError ()
    {
        uint32_t len = baf2sql_get_last_error_string(0, 0);

        boost::shared_array<char> buf(new char[len]);
        baf2sql_get_last_error_string(buf.get(), len);

        BOOST_THROW_EXCEPTION(std::runtime_error(buf.get()));
    }

    /// Find out the file name of the SQLite cache corresponding to the
    /// specified BAF file.  (If the SQLite cache doesn't exist yet, it
    /// will be created.)
    inline std::string getSQLiteCacheFilename (const std::string &baf_filename)
    {
        uint32_t len = baf2sql_get_sqlite_cache_filename(0, 0, baf_filename.c_str());
        if(len == 0)
        {
            throwLastBaf2SqlError();
        }

        boost::shared_array<char> buf(new char[len]);
        baf2sql_get_sqlite_cache_filename(buf.get(), len, baf_filename.c_str());

        return buf.get();
    }

    /// RAII wrapper for BAF binary storage
    class BinaryStorage : public boost::noncopyable
    {
    private:
        uint64_t handle;

        bool doReadArray (uint64_t id, double *buf)
        {
            return 0 != baf2sql_array_read_double(handle, id, buf);
        }

        bool doReadArray (uint64_t id, float *buf)
        {
            return 0 != baf2sql_array_read_float(handle, id, buf);
        }

        bool doReadArray (uint64_t id, uint32_t *buf)
        {
            return 0 != baf2sql_array_read_uint32(handle, id, buf);
        }

    public:
        /// Open specified BAF. Throws on error.
        BinaryStorage (const std::string &baf_filename, bool ignore_calibrator_ami = false)
            : handle(0)
        {
            handle = baf2sql_array_open_storage( ignore_calibrator_ami ? 1 : 0, baf_filename.c_str() );
            if(handle == 0)
            {
                baf2sql::throwLastBaf2SqlError();
            }
            // (handle guaranteed to be non-zero when object was
            // constructed successfully.)
        }

        /// Close the specified BAF.
        ~BinaryStorage ()
        {
            baf2sql_array_close_storage(handle);
        }

        /// Get an array from the BAF file.
        ///
        /// @param id the Id of the desired array.
        ///
        /// @param result std::vector output argument; will contain
        /// the requested array data.
        ///
        /// @tparam T the desired data format (if necessary, the data
        /// will be converted on the fly during reading). Currently
        /// supported formats are double, float and uint32_t.
        ///
        template <typename T>
        void readArray ( uint64_t id, std::vector<T> & result )
        {
            uint64_t n = getArrayNumElements(id);

            typedef typename std::vector<T>::size_type size_type;
            if(n > std::numeric_limits<size_type>::max())
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("Array too large."));
            }

            result.resize(static_cast<size_type>(n));
            if( !doReadArray(id, &result[0]) )
            {
                throwLastBaf2SqlError();
            }
        }

        /// Convenience function for backward compatibility. Consider
        /// using the other form if you're reading a lot of profile
        /// data consecutively into the same vector, it's much faster.
        template <typename T>
        std::vector<T> readArray (uint64_t id)
        {
            std::vector<T> result;
            readArray(id, result);
            return result;
        }

        /// Get the number of elements in an array.
        uint64_t getArrayNumElements (uint64_t id)
        {
            uint64_t n = 0;
            if( !baf2sql_array_get_num_elements(handle, id, &n) )
            {
                throwLastBaf2SqlError();
            }

            return n;
        }

        /// Get the C-API handle corresponding to this instance.
        /// (Caller does not get ownership of the handle.)
        uint64_t getHandle () const
        {
            return handle;
        }

    };

} // namespace baf2sql

#endif // DE_BDAL_CPP_IO_BAF2SQL_IO_BAF2SQL_CPP_H
