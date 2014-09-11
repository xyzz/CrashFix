rem Signing binary files...

cd ..\cert

call SignFile.bat "..\bin\crashfixd.exe"
if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\bin\crashfix_hwinfo.exe"
if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\bin\uploader.exe"
if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\bin\uploader_gui.exe"
if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\bin\CrashRpt1402.dll"
if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\bin\CrashSender1402.exe"
if not %errorlevel% == 0 exit

rem call ..\cert\SignFile.bat "..\bin\libeay32.dll"
rem if not %errorlevel% == 0 exit

call ..\cert\SignFile.bat "..\service_installer\NSISPlugins\nsis_plugin.dll"
if not %errorlevel% == 0 exit

rem Building installer...
cd ..\service_installer
"%PROGRAMFILES(x86)%\NSIS\makensis.exe" /V4 /OBuildLog.txt %1 installler.nsi 

if not %errorlevel%==0 exit

rem Signing installer...
set /p installer_name=<InstallerName.txt
cd ..\cert
erase InstallerName.txt
call ..\cert\SignFile.bat "..\service_installer\%installer_name%"
if not %errorlevel% == 0 exit

