@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if "%IDF_PATH%"=="" (echo Set IDF_PATH to ESP-IDF 5.4.2 & exit /b 1)
cl /nologo /std:c11 /D_CRT_SECURE_NO_WARNINGS /I"%~dp0weather_mocks" /I"%IDF_PATH%/components/json/cJSON" "%~dp0weather_host.c" "%IDF_PATH%/components/json/cJSON/cJSON.c" /Feweather.exe
if errorlevel 1 exit /b %errorlevel%
weather.exe
