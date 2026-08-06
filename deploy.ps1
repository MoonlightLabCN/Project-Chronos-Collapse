# SandboxProbe 快速部署脚本
# 使用方法: .\deploy.ps1 -DeviceIP <设备IP地址> [-UseDebug]

param(
    [Parameter(Mandatory=$true)]
    [string]$DeviceIP,

    [switch]$UseDebug = $false
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  SandboxProbe 快速部署工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 确定使用哪个包
$packagePath = if ($UseDebug) {
    ".\SandboxProbe_ARM_Debug.appx"
} else {
    ".\SandboxProbe_ARM_Release_FixedCapability.appx"
}

$packageType = if ($UseDebug) { "Debug" } else { "Release" }

Write-Host "配置信息:" -ForegroundColor Yellow
Write-Host "  目标设备: $DeviceIP"
Write-Host "  包类型: $packageType"
Write-Host "  包路径: $packagePath"
Write-Host ""

# 检查包文件是否存在
if (-not (Test-Path $packagePath)) {
    Write-Host "错误: 找不到 APPX 包文件!" -ForegroundColor Red
    Write-Host "预期位置: $packagePath" -ForegroundColor Red
    exit 1
}

$packageInfo = Get-Item $packagePath
Write-Host "包信息:" -ForegroundColor Yellow
Write-Host "  大小: $([math]::Round($packageInfo.Length / 1MB, 2)) MB"
Write-Host "  创建时间: $($packageInfo.CreationTime)"
Write-Host ""

# 检查 WinAppDeployCmd 是否可用
$winAppDeployCmd = "WinAppDeployCmd.exe"
try {
    $null = Get-Command $winAppDeployCmd -ErrorAction Stop
    Write-Host "✓ 找到 WinAppDeployCmd 工具" -ForegroundColor Green
} catch {
    Write-Host "错误: 找不到 WinAppDeployCmd.exe" -ForegroundColor Red
    Write-Host "请确保已安装 Windows 10 SDK" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "准备部署..." -ForegroundColor Yellow
Write-Host ""

# 尝试连接设备
Write-Host "1. 正在检查设备连接..." -ForegroundColor Cyan
try {
    $devices = & $winAppDeployCmd devices 2>&1
    if ($devices -match $DeviceIP) {
        Write-Host "   ✓ 设备已连接" -ForegroundColor Green
    } else {
        Write-Host "   ⚠ 无法在设备列表中找到目标设备" -ForegroundColor Yellow
        Write-Host "   继续尝试部署..." -ForegroundColor Yellow
    }
} catch {
    Write-Host "   ⚠ 无法列出设备，继续尝试部署..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "2. 正在部署应用到设备..." -ForegroundColor Cyan
Write-Host ""

# 执行部署
try {
    & $winAppDeployCmd install -file $packagePath -ip $DeviceIP

    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  部署成功！" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "下一步:" -ForegroundColor Yellow
        Write-Host "  1. 在设备上找到 'SandboxProbe' 应用"
        Write-Host "  2. 启动应用"
        Write-Host "  3. 点击 'Run JIT Test (ARM32)' 按钮"
        Write-Host "  4. 观察测试结果"
        Write-Host ""
        Write-Host "预期结果:" -ForegroundColor Yellow
        Write-Host "  如果看到 '>>> TEST 1 PASSED <<<'"
        Write-Host "  说明 Windows 10 Mobile 支持 JIT 编译！"
        Write-Host ""
    } else {
        Write-Host ""
        Write-Host "部署失败 (退出代码: $LASTEXITCODE)" -ForegroundColor Red
        Write-Host ""
        Write-Host "常见问题排查:" -ForegroundColor Yellow
        Write-Host "  1. 确认设备已启用开发者模式"
        Write-Host "  2. 确认设备和电脑在同一网络"
        Write-Host "  3. 确认防火墙未阻止连接"
        Write-Host "  4. 尝试使用 Device Portal 部署"
        Write-Host ""
        exit 1
    }
} catch {
    Write-Host ""
    Write-Host "部署过程中发生错误:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    exit 1
}

Write-Host "提示: 使用 -UseDebug 开关可部署 Debug 版本" -ForegroundColor Gray
Write-Host ""
