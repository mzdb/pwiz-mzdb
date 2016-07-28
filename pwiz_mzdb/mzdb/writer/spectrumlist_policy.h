/*
 * Copyright 2014 CNRS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file spectrumList_policy.h
 * @brief Contains function to get spectra using vendor's peakpicking algorithms for spectra that must be centroided
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef SPECTRUMLISTPOLICY_H
#define SPECTRUMLISTPOLICY_H


namespace mzdb {


/**
 * template function for ABI and Thermo spectrumList proteowizard implementation.
 * Accept a supplementary parameter, a vector containing the index of msLevel to
 * perform a centroidization using vendor algorithm
 */
template<typename SpectrumListType>
static pwiz::msdata::SpectrumPtr getSpectrum(SpectrumListType* spectrumList,
                                            size_t index,
                                            bool getBinaryData,  // always true
                                            pwiz::util::IntegerSet& levelsToCentroid) {

    return spectrumList->spectrum(index, getBinaryData, levelsToCentroid);
}

/**
 * template specialization
 */
template<>
static pwiz::msdata::SpectrumPtr getSpectrum(pwiz::msdata::SpectrumList* spectrumList,
                                            size_t index,
                                            bool getBinaryData, // always true
                                            pwiz::util::IntegerSet& levelsToCentroid) {

    return spectrumList->spectrum(index, getBinaryData);
}

}
#endif // SPECTRUMLISTPOLICY_H
