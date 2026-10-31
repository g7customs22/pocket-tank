@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
set SRC=%~dp0..\..
cl /nologo /std:c11 /FI"%~dp0notification_msvc_compat.h" /O2 /Gy /DPOCKET_WORLD_PROTO=1 /DPOCKET_TANK_WIDE_V1=1 /DPOP_CAP=5 /D_CRT_SECURE_NO_WARNINGS /I%SRC%/common /I%SRC%/firmware/main %SRC%/firmware/host_test/animation_clock_host.c %SRC%/firmware/main/ui_349_v1.c %SRC%/common/world_proto.c %SRC%/common/tank.c %SRC%/common/advisor.c %SRC%/common/progression.c %SRC%/common/icons.c %SRC%/common/setup.c %SRC%/common/notice.c %SRC%/common/battery.c %SRC%/common/llm/advisor_core.c %SRC%/common/llm/q4_model.c %SRC%/common/llm/word_tok.c /Feanimation_clock.exe /link /OPT:REF
if errorlevel 1 exit /b %errorlevel%
animation_clock.exe
