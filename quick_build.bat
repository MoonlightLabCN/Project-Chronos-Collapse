@echo off
REM Quick Build Script for SandboxProbe
REM Just use Visual Studio Developer Command Prompt

echo ========================================
echo SandboxProbe Quick Build
echo ========================================
echo.

echo Building ARM Release version...
echo.

msbuild SandboxProbe.sln /t:Rebuild /p:Configuration=Release /p:Platform=ARM /v:minimal

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    echo.
    echo Please make sure you run this from:
    echo "Developer Command Prompt for VS 2022"
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESS!
echo ========================================
echo.
echo Output location:
echo   ARM\Release\SandboxProbe\
echo.
echo The .appx package should be in:
echo   ARM\Release\SandboxProbe\SandboxProbe.exe
echo   ARM\Release\SandboxProbe\AppxManifest.xml
echo.
echo To deploy to device:
echo   1. Open SandboxProbe.sln in Visual Studio
echo   2. Right-click project -^> Deploy
echo   3. Or press F5 to deploy and run
echo.
pause
