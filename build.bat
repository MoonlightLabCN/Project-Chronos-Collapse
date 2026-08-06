@echo off
REM SandboxProbe - Build and Package Script
REM This script builds the ARM Release version and creates an .appx package

echo ========================================
echo SandboxProbe Build Script
echo ========================================
echo.

REM Find MSBuild
set MSBUILD=""
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
)

if %MSBUILD%=="" (
    echo [ERROR] MSBuild not found!
    echo Please run this from Visual Studio Developer Command Prompt
    echo Or install Visual Studio 2022
    pause
    exit /b 1
)

echo Found MSBuild: %MSBUILD%
echo.

REM Clean previous build
echo [1/4] Cleaning previous build...
%MSBUILD% SandboxProbe.sln /t:Clean /p:Configuration=Release /p:Platform=ARM /v:minimal
if errorlevel 1 (
    echo [ERROR] Clean failed!
    pause
    exit /b 1
)
echo [OK] Clean completed.
echo.

REM Build project
echo [2/4] Building ARM Release...
%MSBUILD% SandboxProbe.sln /t:Rebuild /p:Configuration=Release /p:Platform=ARM /v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
echo [OK] Build completed.
echo.

REM Create appx package
echo [3/4] Creating .appx package...
if not exist "packages" mkdir packages

REM Find MakeAppx
set MAKEAPPX=""
if exist "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\makeappx.exe" (
    set MAKEAPPX="C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\makeappx.exe"
)
if exist "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\makeappx.exe" (
    set MAKEAPPX="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\makeappx.exe"
)

if %MAKEAPPX%=="" (
    echo [WARNING] MakeAppx not found, using build output directory
    echo You can find the built files in: ARM\Release\SandboxProbe\
    goto :sign
)

%MAKEAPPX% pack /d ARM\Release\SandboxProbe\AppX /p packages\SandboxProbe_ARM_Release.appx /o
if errorlevel 1 (
    echo [WARNING] MakeAppx failed, but build output is available
    goto :sign
)
echo [OK] Package created: packages\SandboxProbe_ARM_Release.appx
echo.

:sign
REM Sign package (optional)
echo [4/4] Package ready for deployment
echo.

echo ========================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ========================================
echo.
echo Output files:
if exist "packages\SandboxProbe_ARM_Release.appx" (
    echo   - packages\SandboxProbe_ARM_Release.appx
) else (
    echo   - ARM\Release\SandboxProbe\ (folder with all files)
)
echo.
echo Next steps:
echo   1. Deploy using Visual Studio (F5)
echo   2. Or use Device Portal to upload the .appx
echo   3. Or run: deploy.bat
echo.
pause
