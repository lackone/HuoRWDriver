set "projectpath=%cd%"
cd ../
set "preProjectpath=%cd%/HuoRWDriver"
cd /d %projectpath%
set "SignFullPath=%preProjectpath%/x64/Debug/HuoRWDriver.sys"
set "VMPath=%preProjectpath%/x64/Release/YJ.sys.vmp"

set "d=%date:~0,10%"
set "path=%path%;D:/VMProtect Ultimate/;I:/Program Files (x86)/DSignTool/"

@rem VMProtect_Con.exe %VMPath%

date 2013/8/15
CSignTool.exe  sign /r landong /f %SignFullPath% /ac
date %d%

