#ifndef SPECTRUMLISTPOLICY_H
#define SPECTRUMLISTPOLICY_H

#include "pwiz/data/msdata/MSDataFile.hpp"
//#include "pwiz_aux/msrc/utility/vendor_api/ABI/WiffFile.hpp"
//#include "pwiz/data/vendor_readers/ABI/SpectrumList_ABI.hpp"

/* TODO rewrite in classes ? */
namespace mzdb {

    /* template function for ABI and Thermo spectrumList proteowizard implementation.
     * Accept a supplementary parameter, a vector containing the index of msLevel to
     * perform a centroidization using vendor algorithm */

    template<typename SpectrumListType>
    static pwiz::msdata::SpectrumPtr getSpectrum(SpectrumListType* spectrumList,
                                          size_t index,
                                          bool getBinaryData, pwiz::util::IntegerSet& levelsToCentroid) {
        return spectrumList->spectrum(index, getBinaryData, levelsToCentroid);
    }

    template<>
    static pwiz::msdata::SpectrumPtr getSpectrum(pwiz::msdata::SpectrumList* spectrumList, size_t index,
                                          bool getBinaryData,
                                          pwiz::util::IntegerSet& levelsToCentroid) {
        return spectrumList->spectrum(index, getBinaryData);
    }

/*template<typename SpectrumListT, typename SpectrumT>
static SpectrumT getSpectrum(SpectrumListT* spectrumList,
                                              size_t index,
                                              bool getBinaryData,
                                              pwiz::util::IntegerSet& levelsToCentroid) {
    return spectrumList->spectrum(index, getBinaryData, levelsToCentroid);
}

template<>
static pwiz::msdata::SpectrumPtr getSpectrum(pwiz::msdata::SpectrumList* spectrumList,
                                             size_t index,
                                             bool getBinaryData,
                                             pwiz::util::IntegerSet& levelsToCentroid)  {
    return spectrumList->spectrum(index, getBinaryData);*/

}
#endif // SPECTRUMLISTPOLICY_H
