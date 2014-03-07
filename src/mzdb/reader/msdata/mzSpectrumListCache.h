#ifndef MZSPECTRUMLISTCACHE_H
#define MZSPECTRUMLISTCACHE_H

#include <vector>

#include "pwiz/data/msdata/MSData.hpp"
#include "pwiz/data/msdata/SpectrumListBase.hpp"

#include "mzDataCache.h"
#include "../../utils/mzDBFile.h"
#include "mzSpectrumList.h"

namespace mzdb {

using namespace std;

class mzSpectrumListCache : public pwiz::msdata::SpectrumListBase {
protected:
    mzDataCache* _cacheDB;
    MzDBFile* _file;
    vector<pwiz::msdata::SpectrumIdentity*> _identities;
    pwiz::msdata::MSData* _msdata;

private:
    void populateSpectrumIdentities();

public:

    mzSpectrumListCache(MzDBFile* f, pwiz::msdata::MSData* msdata, mzDataCache* cache); //: _cacheDB(cache) {}

    ~mzSpectrumListCache() {
        for (size_t i = 0; i < _identities.size(); ++i) {
            delete _identities[i];
        }
        delete _cacheDB;
    }

    //interface herited by SpectrumListBase
    pwiz::msdata::SpectrumPtr spectrum(size_t index, bool getBinaryData) const;

    size_t size() const { return _identities.size(); }

    //may be the one difficult to implement
    const pwiz::msdata::SpectrumIdentity& spectrumIdentity(size_t index) const;

};




}
#endif // MZSPECTRUMLISTCACHE_H
