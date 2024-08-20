set "projectpath=%cd%"
cd ../
set "preProjectpath=%cd%\HuoRWDriver"
cd %projectpath%

copy "%projectpath%\DriverLoader.sys.vmp" "%preProjectpath%\x64\Debug"

set "SignFullPath=%preProjectpath%\x64\Debug\DriverLoader.sys.vmp"

set "SignPath=%preProjectpath%\x64\Debug\DriverLoader.vmp.sys"

"I:\Program Files (x86)\VMProtect Ultimate\VMProtect_Con.exe" %SignFullPath%

set "d=%date:~0,10%"
date 2013/8/15
"I:\Program Files (x86)\DSignTool\CSignTool.exe" sign /r landong /f %SignPath% /ac
date %d%

@rem copy %SignPath% "F:\nginx-1.13.12\html\1.sys"
