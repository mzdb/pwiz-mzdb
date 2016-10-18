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

Current stable version is 0.9.7.<br/>
Last available version is 0.9.9, consider it a Release Candidate.

### Next release 0.9.10

New features:
- [ ] [MS-Numpress](https://github.com/ms-numpress/ms-numpress) compression algorithm support
- [ ] Integration of the project with existing msconvert tool
- [ ] add an option to filter spectra upon retention time

Improvements
- [ ] add FK constraints
- [ ] replace blobs with vectors
- [ ] replace table bounding_blo_msn_rtree with table msn_layer
- [ ] update proteowizard libraries ?

Bug fixes:
- [ ] check MS3 analyses
- [ ] ~~add missing CvTerms~~ (not present in Pwiz Msdata object, neither in converted mzML files)
- see issues for more informations

### Release 0.9.9

New features:
- [x] FITTED mode is fully functional for Thermo, AB Sciex and Bruker analysis
- [x] Safe mode added : fall back to centroid if requested mode is not possible (ie. centroid -> profile)
- [x] --cycles option in the command line to convert a subset of the input file
- [x] Build number is added
- [x] add an "--log" option to write logs to a file and/or to the console

Improvements
- [x] Using QTofPeakpicker algorithm for AB Sciex data
- [x] Added a summary at the end of the conversion
- [x] --dia option has been replaced by -a or --acquisition option, user can tell if the analysis is DDA, DIA or let the converter determine it
- [x] Better input and output file verification (convert AB Sciex data by calling .wiff or .wiff.scan files, convert Bruker data by calling .d directory)

Bug fixes:
- [x] Wrong data peak count
- [x] Algorithm to check DDA/DIA is now working on Thermo, AB Sciex and Bruker analysis
- [x] mzML file support is improved
- [x] fixed encoding issue with low resolution spectra
- [x] fixed encoding issue with NO_LOSS option
- see issues for more informations

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
- see issues for more informations 

## Users

### Convert vendor raw files into mzDB files

#### Download and setup

* Download the <a href="https://github.com/mzdb/pwiz-mzdb/releases/download/v0.9.9RC/raw2mzDB_0.9.9RC_build20161018.zip">zip archive</a>
* Raw2mzDB has the same requirements as ProteoWizard, otherwise install the following: .NET Framework 3.5 SP1, .NET Framework 4.0, MSVC 2008 SP1 (x86), MSVC 2012, MSVC 2013 (http://proteowizard.sourceforge.net/user_installation_simple.shtml)

#### Command line usage

Open a command line window in the directory containing raw2mzdb.exe then type: 

**raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\>**

By defaut, the raw file will be converted in the "fitted" mode for the MS1 (MS2 is often in centroid mode and can not be converted in fitted mode). If the MS2 (or superior) are acquired in high resolution (i.e in profile mode), you could specify that you want to convert specific MS levels in the required mode:

**raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -f 1-2** will try to convert MS and MS/MS spectra in fitted mode.

There are two other available conversion modes:
* "profile", the command line is then: **raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -p 1** (means you want profile mode for MS1, other MS levels will be stored as they were stored in the raw file)
* "centroid" : **raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\> -c 1** (means you want centroid mode for MS1, other MS levels will be stored as they were stored in the raw file)

**Complete list of parameters:**

```
usage: raw2mzDB.exe --input filename <parameters>

Options:

	-i, --input : specify the input rawfile path
	-o, --output : specify the output filename (must be an absolute path)
	-c, --centroid : centroidization, eg: -c 1 (centroidization msLevel 1) or -c 1-5 (centroidization msLevel 1 to msLevel 5) 
	-p, --profile : idem but for profile mode 
	-f, --fitted : idem buf for fitted mode 
	-T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s
	-t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s
	-M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da 
	-m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da 
	-a, --acquisition : dda, dia or auto (converter will try to determine if the analysis is DIA or DDA), default: auto
	--no_loss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)
 	--cycles : only convert the selected range of cycles, eg: 1-10 (first ten cycles) or 10- (from cycle 10 to the end) ; using this option will disable progress information
	-s, --safe_mode : use centroid mode if the requested mode is not available
    --log : console, file or both (log file will be put in the same directory as the output file), default: console
	-h --help : show help
```

## Developers

#### Build from command line

Recent ongoing developement where only tested on Windows using MSVC 2010 Ultimate version. 
Compilation on Linux may require some code corrections for the moment. We plan to be cross-platform very soon.

After installing Visual Studio, check following points : 
- Visual Studio path is added to system environment path : *C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin* 
- If you are using 64-bit operating system : allow the cross compilation : 
	- open commandline : Win+R, type *cmd*
	- go to *Microsoft Visual Studio 10.0\VC* : <code>cd C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC</code>
	- execute <code>vcvarsall.bat x86_amd64</code> , you should have <code>Setting environment for using Microsoft Visual Studio 2010 x64 cross tools.</code> message. 

In order to build with *bjam*:
* Unzip pwiz-mzdb-lib.zip file (containing project dependencies as static compiled libraries) located in <code>project_root/pwiz_mzdb/mzdb/lib</code> directory. (You can also download it [here](https://github.com/mzdb/pwiz-mzdb/releases/download/v0.9.8/pwiz-mzdb-lib.zip) if not exist)
* Then run the script *raw2mzDB_quickbuild.bat* from the project root
* Or else, run the following command from the project root: <br/>
<code>quickbuild -j8 address-model=64 pwiz_mzdb --i-agree-to-the-vendor-licenses --incremental</code>
* *--incremental* is not mandatory but it speeds up the compilation process

**raw2mzdb.exe** file is generated in :
<code>project_root/pwiz_mzdb/target</code>

#### Project structure

See [wiki](https://github.com/mzdb/pwiz-mzdb/wiki)

#### Project dependencies(headers already included in the root directory)
* PWIZ of course
* [Ceres](http://ceres-solver.org/)
* [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page)
* [glog](https://github.com/google/glog)

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
	// fetch spectrum, second argument is for getting or not (i.e. fetch only metadata) 
	// spectrum data points, it has no effect on the actual implementation, always 
	// return a spectrum with mz/intensity arrays
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

reader.readMzDB(msdata);
vector<mzScan*> results; // mzScan is a simple object containing vector members 'mz' and 'intensities'

reader.extractRegion(mzmin, mzmax, rtmin, rtmax, msLevel, results);
```

Specifying a msLevel=1 will extract region using spectra acquired in mslevel=1, suitable for DDA analysis. 
Otherwise, it will request the msn R*Tree suitable to perform DIA analysis.   


