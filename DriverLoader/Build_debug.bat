set "projectpath=%cd%"
cd ../
set "preProjectpath=%cd%/HuoRWDriver"
cd %projectpath%
set "SignFullPath=%preProjectpath%/x64/Debug/HuoRWDriver.sys"
Build.exe %SignFullPath%

