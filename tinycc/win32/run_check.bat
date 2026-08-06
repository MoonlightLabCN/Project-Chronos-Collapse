@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
cl /nologo /I.. check_layout.c /Fe:build\check_layout.exe
if errorlevel 1 exit /b 1
echo.
build\check_layout.exe
