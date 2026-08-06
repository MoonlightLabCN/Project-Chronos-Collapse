# Phase3Probe Deployment Script
# Deploy to Lumia 950 XL (192.168.3.67)

$ErrorActionPreference = "Stop"

$deviceIp = "192.168.3.67"
$appxPath = "E:\Desktop\项目\UWP\Project-Chronos-Collapse\Phase3Probe\AppPackages\Phase3Probe\Phase3Probe_1.0.0.0_ARM_Test\Phase3Probe_1.0.0.0_ARM.appx"
$certPath = "E:\Desktop\项目\UWP\Project-Chronos-Collapse\Phase3Probe\Phase3Probe_TemporaryKey.pfx"
$winAppDeploy = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x86\WinAppDeployCmd.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Phase3Probe Deployment Tool" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Target Device: $deviceIp" -ForegroundColor Green
Write-Host "Package: $(Split-Path $appxPath -Leaf)" -ForegroundColor Green
Write-Host "Size: $((Get-Item $appxPath).Length) bytes" -ForegroundColor Green
Write-Host ""

# Test device connectivity
Write-Host "[1/3] Testing device connectivity..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "http://${deviceIp}/" -Method GET -UseBasicParsing -TimeoutSec 10
    Write-Host "  + Device is online (HTTP $($response.StatusCode))" -ForegroundColor Green
} catch {
    Write-Host "  X Device not responding" -ForegroundColor Red
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Check if pairing is needed
Write-Host "[2/3] Checking device pairing..." -ForegroundColor Yellow
Write-Host ""
Write-Host "To pair with the device for the first time:" -ForegroundColor White
Write-Host "  1. On your Lumia 950 XL, go to Settings > Update & Security > For developers" -ForegroundColor White
Write-Host "  2. Enable 'Developer mode' and 'Device Portal'" -ForegroundColor White
Write-Host "  3. Note the pairing PIN shown on the device" -ForegroundColor White
Write-Host "  4. Run this command with the PIN:" -ForegroundColor White
Write-Host ""
Write-Host "     & `"$winAppDeploy`" install -file `"$appxPath`" -ip $deviceIp -pin <YOUR_PIN>" -ForegroundColor Cyan
Write-Host ""

# Attempt deployment
Write-Host "[3/3] Attempting deployment..." -ForegroundColor Yellow
$pin = Read-Host "Enter pairing PIN (or press Enter to try without PIN)"

if ($pin) {
    & $winAppDeploy install -file $appxPath -ip $deviceIp -pin $pin
} else {
    & $winAppDeploy install -file $appxPath -ip $deviceIp
}

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Deployment Successful!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Phase3Probe is now installed on your device." -ForegroundColor White
    Write-Host "You can launch it from the app list." -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "Deployment failed with exit code: $LASTEXITCODE" -ForegroundColor Red
}
