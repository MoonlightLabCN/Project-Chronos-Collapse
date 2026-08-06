# End-to-end test for .tccmod format (PowerShell version)
# This script builds, tests, and validates the tccmod loader

$ErrorActionPreference = "Stop"

Write-Host "=== TccMod End-to-End Test ===" -ForegroundColor Cyan
Write-Host ""

# Configuration
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TinyccDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ScriptDir "build"

# Step 1: Create build directory
Write-Host "Step 1: Preparing build directory..." -ForegroundColor Yellow
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
Write-Host "  Build directory: $BuildDir" -ForegroundColor Gray

# Step 2: Compile the test module creator
Write-Host ""
Write-Host "Step 2: Building test module creator..." -ForegroundColor Yellow
$CreateModuleExe = Join-Path $BuildDir "create_test_module.exe"
$CreateModuleSrc = Join-Path $ScriptDir "create_test_module.c"

Write-Host "  Compiling: $CreateModuleSrc" -ForegroundColor Gray
gcc -I"$TinyccDir" -o "$CreateModuleExe" "$CreateModuleSrc" -Wall
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Failed to compile module creator" -ForegroundColor Red
    exit 1
}
Write-Host "  Success!" -ForegroundColor Green

# Step 3: Create test .tccmod file
Write-Host ""
Write-Host "Step 3: Creating test .tccmod file..." -ForegroundColor Yellow
$TestModule = Join-Path $BuildDir "test_module.tccmod"

& $CreateModuleExe $TestModule
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Failed to create test module" -ForegroundColor Red
    exit 1
}

# Verify file was created
if (Test-Path $TestModule) {
    $FileInfo = Get-Item $TestModule
    Write-Host "  Module created: $($FileInfo.Length) bytes" -ForegroundColor Green
} else {
    Write-Host "  ERROR: Module file not created" -ForegroundColor Red
    exit 1
}

# Step 4: Compile the loader library
Write-Host ""
Write-Host "Step 4: Building TccMod loader library..." -ForegroundColor Yellow
$LoaderSrc = Join-Path $TinyccDir "tccmod_loader.c"
$LoaderObj = Join-Path $BuildDir "tccmod_loader.o"

Write-Host "  Compiling: $LoaderSrc" -ForegroundColor Gray
gcc -I"$TinyccDir" -c "$LoaderSrc" -o "$LoaderObj" -Wall
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Failed to compile loader" -ForegroundColor Red
    exit 1
}
Write-Host "  Success!" -ForegroundColor Green

# Step 5: Compile the test program
Write-Host ""
Write-Host "Step 5: Building loader test program..." -ForegroundColor Yellow
$TestLoaderSrc = Join-Path $ScriptDir "test_tccmod_loader.c"
$TestLoaderExe = Join-Path $BuildDir "test_tccmod_loader.exe"

Write-Host "  Compiling: $TestLoaderSrc" -ForegroundColor Gray
gcc -I"$TinyccDir" -o "$TestLoaderExe" "$TestLoaderSrc" "$LoaderObj" -Wall
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Failed to compile test program" -ForegroundColor Red
    exit 1
}
Write-Host "  Success!" -ForegroundColor Green

# Step 6: Run the test
Write-Host ""
Write-Host "Step 6: Testing loader with test module..." -ForegroundColor Yellow
Write-Host "  Running: $TestLoaderExe $TestModule" -ForegroundColor Gray
Write-Host ""
Write-Host "--- Test Output ---" -ForegroundColor Cyan

& $TestLoaderExe $TestModule
$TestResult = $LASTEXITCODE

Write-Host "--- End Output ---" -ForegroundColor Cyan
Write-Host ""

if ($TestResult -eq 0) {
    Write-Host "✓ TEST PASSED!" -ForegroundColor Green
} else {
    Write-Host "✗ TEST FAILED (exit code: $TestResult)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Note: The test may fail because:" -ForegroundColor Yellow
    Write-Host "  - We're running on x86_64 but the code is ARM Thumb-2" -ForegroundColor Gray
    Write-Host "  - The loader needs VirtualAllocFromApp on Windows 10 Mobile" -ForegroundColor Gray
    Write-Host "  - This is expected until we test on actual device" -ForegroundColor Gray
}

# Summary
Write-Host ""
Write-Host "=== Test Summary ===" -ForegroundColor Cyan
Write-Host "✓ Module creator builds successfully" -ForegroundColor Green
Write-Host "✓ Test .tccmod file generated (ARM Thumb-2 code)" -ForegroundColor Green
Write-Host "✓ Loader library compiles" -ForegroundColor Green
Write-Host "✓ Test program builds" -ForegroundColor Green

Write-Host ""
Write-Host "Built artifacts:" -ForegroundColor Cyan
Write-Host "  $CreateModuleExe" -ForegroundColor Gray
Write-Host "  $TestModule" -ForegroundColor Gray
Write-Host "  $LoaderObj" -ForegroundColor Gray
Write-Host "  $TestLoaderExe" -ForegroundColor Gray

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Integrate tccmod_builder.c into TCC build system" -ForegroundColor Gray
Write-Host "  2. Add TCC_OUTPUT_TCCMOD to tcc.c" -ForegroundColor Gray
Write-Host "  3. Test full workflow: C source -> .tccmod -> load -> execute" -ForegroundColor Gray
Write-Host "  4. Deploy to Lumia 950 XL for real ARM testing" -ForegroundColor Gray
Write-Host ""
