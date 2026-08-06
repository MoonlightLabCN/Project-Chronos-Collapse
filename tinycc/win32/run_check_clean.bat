@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
del build\check_layout.obj 2>nul
del build\check_layout.exe 2>nul
cl /nologo /I.. check_layout.c /Fobuild\check_layout.obj /Fe:build\check_layout.exe
if errorlevel 1 exit /b 1
echo.
build\check_layout.exe
