pwiz-mzdb
=========

Raw2MzDB is an extension of the ProteoWizard framework enabling the support of the mzDB format.
The current stable version is 0.9.10.

For details about mzDB concepts (scanSlice, runSlice...) and specifications, have a look to the [related repository](http://github.com/mzdb/mzdb-specs).

## Project contributors

<div>
<a href="http://www.profiproteomics.fr" target="_blank" ><img src="http://www.profiproteomics.fr/wp-content/themes/profi/images/logo-profi.png" alt="ProFI" width="200" />
</a>
<br/>
</div>

## Roadmap

### Roadmap v1.0

New features:
- [ ] Add an option to filter spectra upon retention time (RT range) (#41)
- [ ] Restore the mzDB2mzML.exe (#42)

Improvements
- [ ] Add acquisition method parameters for AB Sciex and Bruker data (#43)
- [ ] Add FK constraints (set "PRAGMA foreign_keys = ON" after sqlite3_open) + see "DEFERRABLE INITIALLY DEFERRED" change below (#44)
- [ ] Evaluate the performance benefits of "PRAGMA optimize;" (https://sqlite.org/pragma.html#pragma_optimize)
- [ ] Update ProteoWizard libraries (#45)
- [ ] Update SQLite library (#46)

Database schema
- [ ] Add "DEFERRABLE INITIALLY DEFERRED" constraint to all FKs (#47)
- [ ] Upgrade mzDB specification to 1.0 (#48)

### Roadmap v2.0
- [ ] Replace table bounding_box_msn_rtree with table msn_layer (#49)
- [ ] Replace blobs by vectors (#50)
- [ ] Support other instrument vendors (#51)
- [ ] Ion mobility support ? (#52)
- [ ] [MS-Numpress](https://github.com/ms-numpress/ms-numpress) compression algorithm support (#53)
- [ ] Quality control for fitting algorithm: compute difference between raw profile and reconstructed profile by calculating the RMSD based score (#54)

### Roadmap Other
- [ ] Integration of the project with existing msconvert tool (#55)
- [ ] Linux build for "mzML -> mzDB" and "Thermo RAW file -> mzDB" (#56)
- [ ] ~~add missing CvTerms~~ (not present in Pwiz Msdata object, neither in converted mzML files) compare a mzML from mzDB and mzML from raw file (#58)
- [ ] Implement an mzDB validator using mzDB-access (#59)

## Change log

### Roadmap v0.9.10

Bug fixes:
- [x] Issue #24 sample name is empty for AB Sciex analyses (also check for Bruker)
- [x] Issue #26 is for AB Sciex DIA acquisition (and accession 1001954)
- [x] Issue #38: Find and fix memory leak for AB Sciex data
- [x] Issue #39: The field mzdb.param_tree can be corrupted for some Thermo raw files
- [x] Issue #57: Check MS3 analyses
- [x] Issue #60: Missing spectra in some mzDB files
- [x] Issue #61: Problem with DIA analyses

Improvements
- [x] Issue #40: Improve cycle filtering by checking cycle number before centroiding data
- [x] Issue #62: Improving DIA storing in bounding boxes

### Release 0.9.9

New features:
- [x] FITTED mode is fully functional for Thermo, AB Sciex and Bruker analysis
- [x] Safe mode added : fall back to centroid if requested mode is not possible (ie. centroid -> profile)
- [x] --cycles option in the command line to convert a subset of the input file
- [x] Build number is added
- [x] add an "--log" option to write logs to a file and/or to the console
- [x] add an option to display version information (-v or --version)

Improvements
- [x] Using QTofPeakpicker algorithm for AB Sciex data
- [x] Added a summary at the end of the conversion
- [x] --dia option has been replaced by -a or --acquisition option, user can tell if the analysis is DDA, DIA or let the converter determine it
- [x] Better input and output file verification (convert AB Sciex data by calling .wiff or .wiff.scan files, convert Bruker data by calling .d directory)
- [x] Added some dlls to avoid Visual C++ pre-requisites

Bug fixes:
- [x] Wrong data peak count
- [x] Algorithm to check DDA/DIA is now working on Thermo, AB Sciex and Bruker analysis
- [x] mzML file support is improved
- [x] fixed encoding issue with low resolution spectra
- [x] fixed encoding issue with NO_LOSS option
- [x] remove some temporary files after compiling
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
- See fixed [issues](https://github.com/mzdb/pwiz-mzdb/issues?utf8=%E2%9C%93&q=is%3Aissue%20is%3Aclosed) for more information

## Users

### Convert vendor raw files into mzDB files

#### Download and setup

* Download the <a href="https://github.com/mzdb/pwiz-mzdb/releases/download/0.9.10/raw2mzDB_0.9.10_build20170802.zip">zip archive</a>
* Raw2mzDB should work on any modern 64 bits Windows environment. If you encounter missing dlls issues, you may try to install Microsoft's .NET Framework 3.5 SP1 and 4.0. Also consider Visual C++ Redistributable for Visual Studio 2008, 2012 and 2013.

#### Command line usage

Open a command line window in the directory containing raw2mzdb.exe then type: 

**raw2mzdb.exe -i \<rawfilename\> -o \<outputfilename\>**

Example:
**raw2mzdb.exe -i "D:\myfile.raw" -o "D:\myfile.mzDB"**

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
	-o, --output : specify the output filename
	-c, --centroid : centroidization, eg: -c 1 (centroidization msLevel 1) or -c 1-5 (centroidization msLevel 1 to msLevel 5) 
	-p, --profile : idem but for profile mode 
	-f, --fitted : idem buf for fitted mode 
    -T, --bbTimeWidth : bounding box width for ms1 in seconds, default: 15s for DDA, 60s for DIA
    -t, --bbTimeWidthMSn : bounding box width for ms > 1 in seconds, default: 0s for DDA, 75s for DIA
    -M, --bbMzWidth : bounding box height for ms1 in Da, default: 5Da for DDA and DIA
    -m, --bbMzWidthMSn : bounding box height for msn in Da, default: 10000Da for DDA, 20Da for DIA
	-a, --acquisition : dda, dia or auto (converter will try to determine if the analysis is DIA or DDA), default: auto
    --noLoss : if present, leads to 64 bits conversion of mz and intenstites (larger ouput file)
 	--cycles : only convert the selected range of cycles, eg: 1-10 (first ten cycles) or 10- (from cycle 10 to the end) ; using this option will disable progress information
	-s, --safeMode : use centroid mode if the requested mode is not available
    --log : console, file or both (log file will be put in the same directory as the output file), default: console
    -v, --version: display version information
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

#### Debug raw2mzDB

To debug raw2mzDB, you first need to compile it with debug symbols. To do so, open the raw2mzDB_quickbuild.bat script and set *debug-symbols=on*
Or just run quickbuild.bat with this argument correctly set. This should create a raw2mzDB.pdb file in the target directory.
Using this raw2mzDB.pdb file, you can use a debugging tool such as MTuner.

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


