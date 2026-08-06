@echo off
REM Simple build script for tccmod components
REM Uses Visual Studio's cl.exe

setlocal EnableDelayedExpansion

echo === TccMod Build Script ===
echo.

REM Setup Visual Studio environment
set "VSPATH=C:\Program Files\Microsoft Visual Studio\18\Community"
set "VCVARS=%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
    echo ERROR: Visual Studio not found
    exit /b 1
)

echo Setting up Visual Studio environment...
call "%VCVARS%" >nul 2>&1

REM Change to script directory
cd /d "%~dp0"

REM Create build directory
if not exist build mkdir build

echo.
echo Building test module creator...
cl /nologo /W3 /O2 /I.. create_test_module.c /Fe:build\create_test_module.exe
if errorlevel 1 (
    echo ERROR: Failed to build module creator
    exit /b 1
)
echo   Success!

echo.
echo Creating test .tccmod file...
build\create_test_module.exe build\test_module.tccmod
if errorlevel 1 (
    echo ERROR: Failed to create test module
    exit /b 1
)

echo.
echo Building loader test program...
cl /nologo /W3 /O2 /I.. test_tccmod_loader.c ..\tccmod_loader.c /Fe:build\test_tccmod_loader.exe
if errorlevel 1 (
    echo ERROR: Failed to build test program
    exit /b 1
)
echo   Success!

echo.
echo Running test...
echo.
build\test_tccmod_loader.exe build\test_module.tccmod

echo.
echo === Build Complete ===
echo Built artifacts in: build\
dir /b build

endlocal
