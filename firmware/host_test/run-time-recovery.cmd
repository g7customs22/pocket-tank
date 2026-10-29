@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
cl /nologo /std:c11 /D_CRT_SECURE_NO_WARNINGS /I%~dp0time_recovery_mocks %~dp0time_recovery_host.c /Fetime_recovery.exe
if errorlevel 1 exit /b %errorlevel%
time_recovery.exe
