set "projectpath=%cd%"
cd ../
set "preProjectpath=%cd%/DriverLoader"
cd %projectpath%
set "SignFullPath=%preProjectpath%/x64/Release/DriverLoader.sys"
Build.exe %SignFullPath%

