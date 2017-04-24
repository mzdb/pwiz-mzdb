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
 * @file version.h
 * @brief Versioning of the application: we distinguish schema database version against application version, both can have different version numbers
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

namespace mzdb {

//versioning of the application: we distinguish schema database
//version against application version, both can have different version numbers

#define SCHEMA_VERSION_STR "0.7.0"
#define SCHEMA_VERSION_INT 0.7.0
#define SOFT_VERSION_STR "0.9.10-beta"
#define SOFT_VERSION_INT 0.9.10-beta

}
