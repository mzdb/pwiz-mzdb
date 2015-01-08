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

#ifndef MZUSERTEXT_H
#define MZUSERTEXT_H


#include "pwiz/data/msdata/MSData.hpp"


//we introduce here a new kind of param called UserText: metadata specifically
//contained in the raw file that we wanted to be in the mzDB. It is stored as
//plain text
struct UserText : public pwiz::msdata::UserParam {
    inline UserText(const std::string& s="", const std::string& s_="", const std::string& s__=""): pwiz::msdata::UserParam(s, s_, s__){

    }
};
#endif // MZUSERTEXT_H
