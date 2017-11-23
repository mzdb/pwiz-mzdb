#ifndef DE_BDAL_CPP_IO_TIMSDATA_CPP_H
#define DE_BDAL_CPP_IO_TIMSDATA_CPP_H

/** \file
 *
 * Sample for a light-weight header-only C++ layer wrapping the C API for Bruker's TDF
 * reader DLL. You can modify this file as desired.
 *
 * See 'timsdata.h' for more details about the underlying C API.
 *
 */

#include <stdexcept>
#include <string>
#include <cstdint>
#include <numeric>
#include <vector>
#include <limits>

#include "boost/throw_exception.hpp"
#include "boost/noncopyable.hpp"
#include "boost/shared_array.hpp"
#include "boost/range/iterator_range.hpp"

#include "include/c/timsdata.h" // fundamental C API

namespace timsdata
{

    /// Proxy object to conveniently access the data read via tims_read_scans() of the C
    /// API. (Copies of this object share the same underlying data buffer.)
    class FrameProxy
    {
    public:
        FrameProxy (size_t num_scans_, const boost::shared_array<uint32_t> & pData_)
            : num_scans(num_scans_)
            , pData(pData_)
            , scan_offsets(num_scans_ + 1)
        {
            scan_offsets[0] = 0;
            std::partial_sum(pData.get() + 0, pData.get() + num_scans, scan_offsets.begin() + 1);
        }

        /// Get number of scans represented by this object (if this was produced by
        /// TimsData::readScans(), it contains only the requested scan range).
        size_t getNbrScans () const {
            return num_scans;
        }

        size_t getTotalNbrPeaks () const {
            return scan_offsets.back();
        }

        size_t getNbrPeaks (size_t scan_num) const {
            throwIfInvalidScanNumber(scan_num);
            return pData[scan_num];
        }

        boost::iterator_range<const uint32_t *> getScanX (size_t scan_num) const {
            return makeRange(scan_num, 0);
        }

        boost::iterator_range<const uint32_t *> getScanY (size_t scan_num) const {
            return makeRange(scan_num, pData[scan_num]);
        }

    private:
        const size_t num_scans;
        const boost::shared_array<uint32_t> pData; //< data layout as described in tims_read_scans()
        std::vector<uint32_t> scan_offsets;

        void throwIfInvalidScanNumber (size_t scan_num) const {
            if(scan_num >= getNbrScans())
                BOOST_THROW_EXCEPTION(std::invalid_argument("Scan number out of range."));
        }

        boost::iterator_range<const uint32_t *> makeRange (size_t scan_num, size_t offset) const {
            throwIfInvalidScanNumber(scan_num);
            const uint32_t *p = pData.get() + num_scans + 2*scan_offsets[scan_num] + offset;
            return boost::make_iterator_range(p, p + pData[scan_num]);
        }

    };

    /// \throws std::runtime_error containing the last timsdata.dll error string.
    inline void throwLastError ()
    {
        uint32_t len = tims_get_last_error_string(0, 0);

        boost::shared_array<char> buf(new char[len]);
        tims_get_last_error_string(buf.get(), len);

        BOOST_THROW_EXCEPTION(std::runtime_error(buf.get()));
    }

    /// Reader for TIMS binary data (.tdf_bin). (The SQLite file (.tdf) containing all the
    /// metadata may be opened separately using any desired SQLite API.)
    class TimsBinaryData : public boost::noncopyable
    {
    public:
        /// Open specified TIMS analysis.
        ///
        /// \param analysis_directory_name in UTF-8 encoding.
        ///
        /// \throws std::exception in case of an error
        explicit TimsBinaryData(const std::string &analysis_directory_name)
            : handle(0)
            , initial_frame_buffer_size(128)
        {
            handle = tims_open(analysis_directory_name.c_str(), 0);
            if(handle == 0)
                throwLastError();
        }

        /// Close TIMS analysis.
        ~TimsBinaryData()
        {
            tims_close(handle);
        }

        /// Get the C-API handle corresponding to this instance. (Caller does not get
        /// ownership of the handle.) (This call is here for the case that the user wants
        /// to call C-library functions directly.)
        uint64_t getHandle () const
        {
            return handle;
        }

        /// Read a range of scans from a single frame. Not thread-safe.
        ///
        /// \returns a proxy object that represents only the requested scan range of the
        /// specified frame (i.e., FrameProxy::getNbrScans() will return 'scan_end -
        /// scan_begin', and scan #0 in the proxy will correspond to 'scan_begin').
        ::timsdata::FrameProxy readScans (
            int64_t tims_id,      //< from .tdf SQLite: Frames.TimsId
            uint32_t scan_begin,  //< first scan number to read (inclusive)
            uint32_t scan_end )   //< last scan number (exclusive)
        {
            if(scan_end < scan_begin)
                BOOST_THROW_EXCEPTION(std::runtime_error("scan_end must be >= scan_begin"));
            const uint32_t num_scans = scan_end - scan_begin;

            boost::shared_array<uint32_t> pData;

            // buffer-growing loop
            for(;;) {
                pData.reset(new uint32_t[initial_frame_buffer_size]);

                uint32_t required_len = tims_read_scans(handle, tims_id, scan_begin, scan_end,
                                                        pData.get(), uint32_t(4*initial_frame_buffer_size));
                if(required_len == 0)
                    throwLastError();

                if(4 * initial_frame_buffer_size > required_len) {
                    if(required_len < 4 * num_scans)
                        BOOST_THROW_EXCEPTION(std::runtime_error("Data array too small."));
                    return ::timsdata::FrameProxy(num_scans, pData);
                }

                if(required_len > 16777216) // arbitrary limit for now...
                    BOOST_THROW_EXCEPTION(std::runtime_error("Maximum expected frame size exceeded."));
                
                initial_frame_buffer_size = required_len / 4 + 1; // grow buffer
            }
        }

        #define BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(CPPNAME, CNAME) \
        template<typename Vector> \
        void CPPNAME ( \
            int64_t frame_id,               /**< from .tdf SQLite: Frames.Id */ \
            boost::iterator_range<const uint32_t *> & in, /**< vector of input values (can be empty) */ \
            Vector<double> & out )     /**< vector of corresponding output values (will be resized automatically) */ \
        { \
            doTransformation(frame_id, in, out, CNAME); \
        }

        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(indexToMz, tims_index_to_mz)
        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(mzToIndex, tims_mz_to_index)
        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(scanNumToOneOverK0, tims_scannum_to_oneoverk0)
        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(oneOverK0ToScanNum, tims_oneoverk0_to_scannum)
        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(scanNumToVoltage, tims_scannum_to_voltage)
        //BDAL_TIMS_DEFINE_CONVERSION_FUNCTION(voltageToScanNum, tims_voltage_to_scannum)

        template<typename VectorIn, typename VectorOut>
        void indexToMz(int64_t frame_id, const VectorIn& in, VectorOut& out) const
        {
            if (in.empty()) { out.clear(); return; }
            if (in.size() > std::numeric_limits<uint32_t>::max()) BOOST_THROW_EXCEPTION(std::runtime_error("Input range too large."));
            out.resize(in.size());
            tims_index_to_mz(handle, frame_id, &in[0], &out[0], uint32_t(in.size()));
        }

        template<typename VectorIn, typename VectorOut>
        void scanNumToOneOverK0(int64_t frame_id, const VectorIn& in, VectorOut& out) const
        {
            if (in.empty()) { out.clear(); return; }
            if (in.size() > std::numeric_limits<uint32_t>::max()) BOOST_THROW_EXCEPTION(std::runtime_error("Input range too large."));
            out.resize(in.size());
            tims_scannum_to_oneoverk0(handle, frame_id, &in[0], &out[0], uint32_t(in.size()));
        }

    private:
        uint64_t handle;
        size_t initial_frame_buffer_size; // number of uint32_t elements
    };

} // namespace timsdata

#endif // DE_BDAL_CPP_IO_TIMSDATA_CPP_H
