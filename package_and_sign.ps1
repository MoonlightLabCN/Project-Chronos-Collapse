# Package and Sign Script for SandboxProbe
# This script creates an AppX package with test_compile.tccmod and signs it

param(
    [string]$BuildOutput = "",
    [string]$ProjectRoot = "",
    [string]$OutputName = "SandboxProbe_Fixed.appx"
)

$ErrorActionPreference = "Stop"

Write-Host "=== SandboxProbe Package and Sign ===" -ForegroundColor Cyan
Write-Host ""

# Paths
$scriptRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = $scriptRoot
}
if ([string]::IsNullOrWhiteSpace($BuildOutput)) {
    $BuildOutput = Join-Path $ProjectRoot "ARM\Release\SandboxProbe"
}
$makeappx = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\makeappx.exe"
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
$tempDir = Join-Path $ProjectRoot "temp_package"
$tccmodSource = Join-Path $ProjectRoot "SandboxProbe\test_compile.tccmod"
$pfxFile = Join-Path $ProjectRoot "SandboxProbe_Matching.pfx"
$outputAppx = if ([System.IO.Path]::IsPathRooted($OutputName)) {
    $OutputName
} else {
    Join-Path ([Environment]::GetFolderPath("Desktop")) $OutputName
}

# Check if build output exists
if (-not (Test-Path -LiteralPath $BuildOutput -PathType Container)) {
    Write-Host "[ERROR] Build output not found: $BuildOutput" -ForegroundColor Red
    exit 1
}

# Create temp directory
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force
}
New-Item -ItemType Directory -Path $tempDir | Out-Null

Write-Host "[1/6] Copying build output..." -ForegroundColor Yellow
# Copy all files from build output
Copy-Item "$BuildOutput\*" $tempDir -Recurse -Force

Write-Host "[2/6] Adding test_compile.tccmod..." -ForegroundColor Yellow
if (Test-Path $tccmodSource) {
    Copy-Item $tccmodSource (Join-Path $tempDir "test_compile.tccmod")
    Write-Host "  Added: test_compile.tccmod" -ForegroundColor Green
} else {
    Write-Host "  [WARN] test_compile.tccmod not found at: $tccmodSource" -ForegroundColor Yellow
}

Write-Host "[3/6] Cleaning old package artifacts..." -ForegroundColor Yellow
Remove-Item (Join-Path $tempDir "AppxSignature.p7x") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $tempDir "AppxMetadata\CodeIntegrity.cat") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $tempDir "AppxBlockMap.xml") -ErrorAction SilentlyContinue

Write-Host "[4/6] Creating AppX package..." -ForegroundColor Yellow
& $makeappx pack /d $tempDir /p $outputAppx /o

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] MakeAppx failed" -ForegroundColor Red
    exit 1
}

Write-Host "[5/6] Signing package..." -ForegroundColor Yellow
& $signtool sign /fd SHA256 /f $pfxFile /p "test123" $outputAppx

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Signing failed" -ForegroundColor Red
    exit 1
}

Write-Host "[6/6] Cleaning up..." -ForegroundColor Yellow
Remove-Item $tempDir -Recurse -Force

Write-Host ""
Write-Host "=== SUCCESS ===" -ForegroundColor Green
Write-Host "Package created: $outputAppx" -ForegroundColor Green
Write-Host ""
Write-Host "To install:" -ForegroundColor Cyan
Write-Host "  1. Ensure certificate is installed (OuyangQuan_CodeSigning.cer)"
Write-Host "  2. Double-click $OutputName"
