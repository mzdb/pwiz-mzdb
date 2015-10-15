#ifndef MZCHROMATOGRAMLIST_H
#define MZCHROMATOGRAMLIST_H

#include "pwiz/data/msdata/ChromatogramListBase.hpp"

#include "../../utils/mzDBFile.h"
#include "../../utils/mzIDeserializer.h"

namespace mzdb {


class mzChromatogramList : public pwiz::msdata::ChromatogramListBase{

    MzDBFile* _mzdb;
    pwiz::msdata::MSData* _msdata;
    std::vector<pwiz::msdata::ChromatogramIdentity*> _chromatogramIdentities;

    pwiz::msdata::ChromatogramPtr nextChromatogram();
    void initIteration();

public:
     mzChromatogramList(MzDBFile*, pwiz::msdata::MSData*);
     ~mzChromatogramList();
     size_t size() const;
     const pwiz::msdata::ChromatogramIdentity& chromatogramIdentity(size_t index) const;
     pwiz::msdata::ChromatogramPtr chromatogram(size_t index, bool getBinaryData = false) const;
};


}//end namespace


#endif // MZCHROMATOGRAMLIST_H
