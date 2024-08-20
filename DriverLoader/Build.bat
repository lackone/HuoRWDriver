set "projectpath=%cd%"
cd ../
set "preProjectpath=%cd%/HuoRWDriver"
cd %projectpath%
set "SignFullPath=%preProjectpath%/x64/Release/HuoRWDriver.sys"
Build.exe %SignFullPath%

