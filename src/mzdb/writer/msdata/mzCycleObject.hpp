#ifndef __MZ_CYCLE_OBJECT__
#define __MZ_CYCLE_OBJECT__

#include <vector>
#include <map>
#include "../../utils/mzUtils.hpp"
#include <memory>

//forward declaration
template<class mz_t, class int_t>
class mzSpectrum;

namespace mzdb {

/**
 *
 */
template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
struct mzCycle {

    int spectrumCount;

    mzSpectrum<h_mz_t, h_int_t>* parentSpectrum; //assume the parentSpectrum is always in HighResMode could change later
    map<int, std::vector<pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* > > > msnSpectra;

    Ownership ownership;

    //default constructor
    inline mzCycle() : spectrumCount(0), parentSpectrum(0), ownership(DO_NOT_TAKE_OWNERSHIP){}

    inline ~mzCycle() {
        if (ownership == TAKE_OWNERSHIP) {
            if (parentSpectrum) {
                delete parentSpectrum;
#ifdef SAFE_DELETE
                parentSpectrum = 0;
#endif
            }
            for (auto it = msnSpectra.begin(); it != msnSpectra.end(); ++it) {
                const int& msLevel = it->first;
                for (auto it_ = it->second.begin(); it_ != it->second.end(); ++it_) {
                    if (it_->first) delete it_->first;
                    else delete it_->second;
                }
            }
        }
    }

    inline void addLowResSpectra(mzSpectrum<l_mz_t, l_int_t>* s) {
        msnSpectra[s->msLevel()].push_back(make_pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* >(0, s));
        ++spectrumCount;
    }

    inline void addHighResSpectra(mzSpectrum<h_mz_t, h_int_t>* s) {
        msnSpectra[s->msLevel()].push_back(make_pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* >(s, 0));
        ++spectrumCount;
    }

    inline void setParentSpectrum(mzSpectrum<h_mz_t, h_int_t>* s) {
        parentSpectrum = s;
        ++spectrumCount;
    }

    inline bool isEmpty() const { return ( parentSpectrum == 0 && msnSpectra.empty() ); }

    inline const float parentSpectrumRt() {return parentSpectrum->rt(); }
};

/**
 *
 */
template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
struct mzCycleCollection {

    //std::map<int, float> bbWidthManager, bbHeightManager;
    std::vector<mzCycle<h_mz_t, h_int_t, l_mz_t, l_int_t>* > cycles;

    Ownership ownership;

    inline mzCycleCollection(): ownership(TAKE_OWNERSHIP){}

    //inline mzCycleCollection(const std::map<int, float>& bbWidthManager_, const std::map<int, float>& bbHeightManager_):
    //    bbWidthManager(bbWidthManager_), bbHeightManager(bbHeightManager_), ownership(TAKE_OWNERSHIP){}


    inline void getSpectra(std::vector<mzSpectrum<h_mz_t, h_int_t>* >& ms1Buffer, map<int, pair<std::vector<mzSpectrum<h_mz_t, h_int_t>* >, std::vector<mzSpectrum<l_mz_t, l_int_t>* > > >& spectra) const{

        for (auto it = cycles.begin(); it != cycles.end(); ++it) {
            ms1Buffer.push_back((*it)->parentSpectrum);
            for (auto it_ = (*it)->msnSpectra.begin(); it_ != (*it)->msnSpectra.end(); ++it_) {
                const int& msLevel = it_->first;

                auto& lowResMsnSpectra = spectra[msLevel].second;
                auto& highResMsnSpectra = spectra[msLevel].first;
                for (auto it__ = it_->second.begin(); it__ != it_->second.end(); ++it__) {
                    if (it__->first) highResMsnSpectra.push_back(it__->first);
                    else lowResMsnSpectra.push_back(it__->second);
                }
            }
        }
    }

    inline ~mzCycleCollection() {
        if (ownership == TAKE_OWNERSHIP) {
            for (auto it = cycles.begin(); it != cycles.end(); ++it) {
                delete (*it);
            }
        }
    }

    inline const float rtAtFront() const { return cycles.front()->parentSpectrum->rt(); }

    inline const float rtAtBack() const { return cycles.back()->parentSpectrum->rt(); }

    inline bool empty() const {return cycles.empty();}

    inline int spectrumCount() const{
        int sum = 0;
        for_each(cycles.begin(), cycles.end(), [&sum](mzCycle<h_mz_t, h_int_t, l_mz_t, l_int_t>* c) {
            sum += c->spectrumCount;
        });
        return sum;
    }

};

template<class h_mz_t, class h_int_t, class l_mz_t, class l_int_t>
struct mzSpectraContainer {

    int msLevel;
    std::vector<pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* > > spectra;
    pwiz::msdata::SpectrumPtr parentSpectrum;

    inline mzSpectraContainer(int msLevel): msLevel(msLevel) {}

    inline void addLowResSpectra(mzSpectrum<l_mz_t, l_int_t>* s) {
        spectra.push_back(std::make_pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* >(0, s));
    }

    inline void addHighResSpectra(mzSpectrum<h_mz_t, h_int_t>* s) {
        spectra.push_back(std::make_pair<mzSpectrum<h_mz_t, h_int_t>*, mzSpectrum<l_mz_t, l_int_t>* >(s, 0));
    }

    inline float getBeginRt() {
        if (spectra.front().first)
            return spectra.front().first->rt();
        else
            return spectra.front().second->rt();
    }

    inline int getBeginId() {
        if (spectra.front().first)
            return spectra.front().first->id;
        else
            return spectra.front().second->id;
    }

    inline bool empty() { return spectra.empty();}

    inline int size() {return spectra.size();}

    inline void getSpectra(std::vector<mzSpectrum<h_mz_t, h_int_t>* >& highResSpec,
                           std::vector<mzSpectrum<l_mz_t, l_int_t>* >& lowResSpec) const {
        for (auto it = spectra.begin(); it != spectra.end(); ++it) {
            if (it->first)
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
    pwiz::msdata::SpectrumPtr parentSpectrum;


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
