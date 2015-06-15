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

//author marc.dubois@ipbs.fr

#ifndef __MZ_CYCLE_OBJECT__
#define __MZ_CYCLE_OBJECT__

#include <vector>
#include <map>
#include "../../utils/mzUtils.hpp"
#include <memory>

#include "spectrum.hpp"

//forward declaration, fail to compil on gcc 4.7
//template<class mz_t, class int_t>
//class mzSpectrum;

namespace mzdb {

/**
 * mzSpectraContainer class
 * ========================
 *
 * Cycle object used especially in DDA/DIA producers. Basically, a cycle object
 * can contain low AND high resolution spectra.
 *
 * @see mzDDAProducer
 */
template< class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
struct mzSpectraContainer {


    typedef typename h_mz_t h_mz_t;
    typedef typename h_int_t h_int_t;
    typedef typename l_mz_t l_mz_t;
    typedef typename l_int_t l_int_t;

    typedef std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > HighResSpectumSPtr;
    typedef std::shared_ptr<mzSpectrum<l_mz_t, l_int_t> > LowResSpectrumSPtr;

    /** ms level */
    int msLevel;

    std::vector<pair<HighResSpectumSPtr, LowResSpectrumSPtr > > spectra;

    /** if cycle of ms level 2, will be filled else will be null */
    HighResSpectumSPtr parentSpectrum;

    inline mzSpectraContainer() {}
    inline mzSpectraContainer(int msLevel): msLevel(msLevel) {}

    /** add low resolution spectrum */
    inline void addLowResSpectrum(LowResSpectrumSPtr s) {
        spectra.push_back(std::make_pair(nullptr, s));
    }

    /** add high resolution spectrum */
    inline void addHighResSpectrum(HighResSpectumSPtr s) {
        spectra.push_back(std::make_pair(s, nullptr));
    }


    /** return retention time of the first spectrum of the cycle */
    inline float getBeginRt() const {
        if (spectra.front().first != nullptr)
            return spectra.front().first->rt();
        else
            return spectra.front().second->rt();
    }

    /** return ID (which is stored in sqlite) of the first spectrum of the cycle*/
    inline int getBeginId() const {
        if (spectra.front().first != nullptr)
            return spectra.front().first->id;
        else
            return spectra.front().second->id;
    }

    inline bool empty() const { return spectra.empty();}

    inline int size() const {return spectra.size();}

    /**
     *
     * @param highResSpec high resolution spectra output
     * @param lowResSpec low resolution output
     */
    inline void getSpectra(std::vector<HighResSpectumSPtr>& highResSpec,
                           std::vector<LowResSpectrumSPtr>& lowResSpec) const {
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {
            if (it->first != nullptr)
                highResSpec.push_back(it->first);
            else
                lowResSpec.push_back(it->second);
        }
    }

};

template<class h_mz_t, class h_int_t>
struct mzSpectraCollection {
    int msLevel;
    std::vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > > spectra;
    //pwiz::msdata::SpectrumPtr parentSpectrum;
    std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > parentSpectrum;

    inline explicit mzSpectraCollection(int msLevel): msLevel(msLevel) {}

    //the move constructor
    //mzSpectraCollection(mzSpectraCollection<h_mz_t, h_int_t>&& s) {
    //}
    inline void addSpectrum( std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > s) {
        spectra.push_back(s);
    }

    inline float getBeginRt() const {
        if ( ! spectra.empty())
            return spectra.front()->rt();
        return 0.0;
    }

    inline int getBeginId() const {
        if ( ! spectra.empty())
            return spectra.front()->id;
        return 1;
    }

    inline bool empty() const { return spectra.empty();}

    inline int size() const {return spectra.size();}

    inline std::vector<std::shared_ptr<mzSpectrum<h_mz_t, h_int_t> > >& getSpectra() {
      return spectra;
    }

};


}//end namespace
#endif
