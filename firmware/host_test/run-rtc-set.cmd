@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
cl /nologo /std:c11 /DPOCKET_TANK_WIDE_V1=1 /D_CRT_SECURE_NO_WARNINGS /I%~dp0rtc_mocks /I%~dp0../../common %~dp0rtc_set_host.c /Fortc_set.obj /Fertc_set.exe
if errorlevel 1 exit /b %errorlevel%
rtc_set.exe
