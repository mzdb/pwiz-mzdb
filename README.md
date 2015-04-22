pwiz-mzdb
=========

An extension of the ProteoWizard framework enabling the support of the mzDB format.

For details about mzDB concepts and specifications, have a look to the [related repository](http://github.com/mzdb/mzdb-specs).

## Users

### Convert vendor raw files into mzDB files

#### Download and setup

* Download the <a href="https://github.com/mzdb/pwiz-mzdb/releases/download/v0.9.7-beta.1/pwiz_mzdb_0.9.7.zip">zip archive</a>
* For conversion of Thermo raw files, install the <a href="http://sjsupport.thermofinnigan.com/public/detail.asp?id=703">MSFileReader</a>. It will install all necessary C++ redistribuables.
* Ensure your regional settings parameters are '.' for the decimal symbol and ',' for the list separator

#### Command line usage

Open a command line window in the directory containing raw2mzdb.exe then type: 

**raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\>**

By defaut, the raw file will be converted in the "fitted" mode for the MS1 (MS2 is often in centroid mode and can not be converted in fitted mode). If the MS2 (or superior) are acquired in high resolution (i.e in profile mode), you could specify that you want to convert specific MS levels in the required mode:

**raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -f 1-2** will try to convert MS and MS/MS spectra in fitted mode.

There are two other available conversion modes:
* "profile", the command line is then: **raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -p 1** (means you want profile mode for MS1, other MS levels will be stored as they were stored in the raw file)
* "centroid" : **raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -c 1** (means you want centroid mode for MS1, other MS levels will be stored as they were stored in the raw file)

### Convert vendor mzDB files into mzML files

TODO

## Developers

#### Build from command line

Recent ongoing developement where only tested on Windows using MSVC 2010.
Compilation on Linux may require some code corrections for the moment. We plan to be cross-platform very soon.

In order to build with *bjam*:

**quickbuilt.bat -j8 address-model=64 pwiz_mzdb --i-agree-to-the-vendor-licenses**

#### Project structure

TODO

#### Project dependencies

#### IDE setup

### HOW TO ?

TODO

#### Iterate through spectra

#### Iterate through run slices

#### Query LC-MS DDA data using R*Tree queries

#### Query LC-MS/MS DIA data using R*Tree queries
