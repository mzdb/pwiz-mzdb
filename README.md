pwiz-mzdb
=========

An extension of the ProteoWizard framework enabling the support of the mzDB format.

For details about mzDB concepts (scanSlice, runSlice...) and specifications, have a look to the [related repository](http://github.com/mzdb/mzdb-specs).

## Project contributors

<div>
<a href="http://www.profiproteomics.fr" target="_blank" ><img src="http://www.profiproteomics.fr/wp-content/themes/profi/images/logo-profi.png" alt="ProFI" width="200" />
</a>
<br/>
<a href="http://omics-services.com" target="_blank" >
<img src="http://omics-services.com/images/logo_omics_services.png" alt="Omics Services" width="200" />
</a>
</div>

## Roadmap

Current stable version is 0.9.7.

### Release 0.9.8

New features:
- [x] AbSciex (.WIFF) files support
- [x] Bruker (.d) files support
- [x] --dia option in the command line to force DIA file creation
- [ ] ~~--ignore-error option to force conversion even if error occured~~ (CRT translation to C++ exceptions makes the converter very slow)

Improvements
- [x] reduced time of spectrum table loading (table records stored at the end of the file)
- [x] improvements in exception catching
- [x] new columns 'mz_precision' ansd 'intensity_precision' in data-encoding table (instead of param-tree)
- [x] insert only used data-encoding
- [x] update proteowizard to the latest

Bug fixes:
- [x] Wrong encoding for HCD spectra (32 instead of 64 bits)
- [ ] ~~add missing CvTerms~~ (not present in Pwiz Msdata object, neither in converted mzML files)
- see issues for more informations 

### Release 0.9.9

New features:
- [ ] [MS-Numpress](https://github.com/ms-numpress/ms-numpress) compression algorithm support
- [ ] Integration of the project with existing msconvert tool

### Next releases

Next will releases will be number 0.9.10 then 0.9.1x until we reach the expected feature coverage of the converter and the required stability.

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

* Download project dependencies as static compiled libraries: [pwiz-mzdb-lib](https://github.com/mzdb/pwiz-mzdb/releases/download/v0.9.8/pwiz-mzdb-lib.zip)
* Unzip this file in the directory <code>project_root/pwiz_mzdb/mzdb/lib</code>
* Then run the following command from the project root: <br/>
<code>quickbuild -j8 address-model=64 pwiz_mzdb --i-agree-to-the-vendor-licenses</code>

#### Project structure

TODO

#### Project dependencies

* [Ceres](http://ceres-solver.org/)
* [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page)

#### IDE setup

* Visual Studio: not very well tested.
* QtCreator: importing project with existing sources (from the menu), will provide decent code completion.

### HOW TO ?

#### Iterate through spectra

To iterate over all spectra, simply do the following:

```C++
//build a mzdbfile
MzDBFile mzdb(filename);

mzDBReader reader(mzdb); //build a mzdbreader object
MSData msdata; // build empty Pwiz msdata object

// the following will build a custom SpectrumList, ready for iteration
reader.readMzDB(msdata);

SpectrumListPtr sl = msdata.run.spectrumListPtr; // fetch spectrumList
for (size_t i=0; i < sl.size(); ++i) {
	// fetch spectrum, second argument is for getting spectrum data points
	// it has no effect on the actual implementation, always return a spectrum
	// with mz/intensity arrays
	SpectrumPtr s = sl.spectrum(i, true);
	
	//...do something else
}
```

Warning: this is not suitable for accessing only one random spectrum. User may use the 'getSpectrum' function instead.
	
#### Iterate through run slices

Not yet implemented. You can only extract one runSlice at a time for the moment:

```C++
MzDBFile mzdb(filename);

mzDBReader reader(mzdb); //build a mzdbreader object
MSData msdata; // build empty Pwiz msdata object

// the following will build a custom SpectrumList, ready for iteration
reader.readMzDB(msdata);
vector<mzScan*> results; // mzScan is a simple object containing vector members 'mz' and 'intensities'
reader.extractRunSlice(mzmin, mzmax, msLevel, results);
```

This feature is already implemented in the java reader mzDBAccess

#### Query LC-MS DDA/DIA data using R*Tree queries

To extract region using R*Tree:

```C++
MzDBFile mzdb(filename);

mzDBReader reader(mzdb); //build a mzdbreader object
MSData msdata; // build empty Pwiz msdata object

// the following will build a custom SpectrumList, ready for iteration
reader.readMzDB(msdata);
vector<mzScan*> results; // mzScan is a simple object containing vector members 'mz' and 'intensities'

reader.extractRegion(mzmin, mzmax, rtmin, rtmax, msLevel, results);
```

Specifying a msLevel=1 will extract region using spectra acquired in mslevel=1, suitable for DDA analysis. 
Otherwise, it will request the msn R*Tree suitable to perform DIA analysis.   


