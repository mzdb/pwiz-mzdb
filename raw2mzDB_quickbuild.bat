@echo off

rem erase former directory
IF EXIST "pwiz_mzdb\target" (
  rmdir /S /Q "pwiz_mzdb\target"
)

rem start building raw2mzDB
set start=%time%
echo Build started at %start%
rem -j<n> indicates using the number of processors used for compiling (some say that you should use n+1 cores, in my case I have a quad-core so I set -j5)
rem set debug-symbols=on to compile in debug mode
call quickbuild -j5 address-model=64 pwiz_mzdb --i-agree-to-the-vendor-licenses --incremental debug-symbols=off > raw2mzDB.log 2>&1
rem return code is not very informative because success can have a code of 0 or 1... (the real test is to check if raw2mzDB.exe file exists or not)
echo Compilation return code is %ERRORLEVEL%

rem check if raw2mzDB.exe has been generated
IF EXIST "pwiz_mzdb\target\raw2mzDB.exe" (
  rem remove files that should be removed with bjam
  erase "pwiz_mzdb\target\raw2mzDB.obj"
  erase "pwiz_mzdb\target\raw2mzDB.obj.rsp"
  erase "pwiz_mzdb\target\raw2mzDB.exe.rsp"
  rem happy ending
  echo File raw2mzDB has been generated !
  goto CalcTime
) else (
  rem this is a problem !
  echo File raw2mzDB is missing !
  goto Quit
)

:CalcTime

set end=%time%
echo Build finished at %end%

rem calculate elapsed time
set options="tokens=1-4 delims=:."
for /f %options% %%a in ("%start%") do set start_h=%%a&set /a start_m=100%%b %% 100&set /a start_s=100%%c %% 100&set /a start_ms=100%%d %% 100
for /f %options% %%a in ("%end%") do set end_h=%%a&set /a end_m=100%%b %% 100&set /a end_s=100%%c %% 100&set /a end_ms=100%%d %% 100
set /a hours=%end_h%-%start_h%
set /a mins=%end_m%-%start_m%
set /a secs=%end_s%-%start_s%
set /a ms=%end_ms%-%start_ms%
if %hours% lss 0 set /a hours = 24%hours%
if %mins% lss 0 set /a hours = %hours% - 1 & set /a mins = 60%mins%
if %secs% lss 0 set /a mins = %mins% - 1 & set /a secs = 60%secs%
if %ms% lss 0 set /a secs = %secs% - 1 & set /a ms = 100%ms%
if 1%ms% lss 100 set ms=0%ms%
set /a totalsecs = %hours%*3600 + %mins%*60 + %secs% 
echo Elapsed time: %hours%:%mins%:%secs%

:Quit
echo.
