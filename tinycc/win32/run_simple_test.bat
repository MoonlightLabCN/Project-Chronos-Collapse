@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"

echo Building simple test...
cl /nologo /W3 /O2 /I.. test_simple_loader.c ..\tccmod_loader.c /Fe:build\test_simple_loader.exe
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo Running test...
echo.
build\test_simple_loader.exe build\test_module.tccmod
