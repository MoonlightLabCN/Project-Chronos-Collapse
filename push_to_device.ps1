# Windows Device Portal (WDP) 文件推送脚本
# 目标设备: Lumia 950 XL (192.168.3.18)

$DeviceIP = "192.168.3.18"
$DevicePort = "50443"  # WDP HTTPS 端口
$BaseUrl = "https://${DeviceIP}:${DevicePort}"

# WDP 凭据（需要根据实际情况修改）
$Username = "Administrator"
$Password = "P@ssw0rd"  # 请替换为实际密码

# 创建凭据
$SecurePassword = ConvertTo-SecureString $Password -AsPlainText -Force
$Credential = New-Object System.Management.Automation.PSCredential($Username, $SecurePassword)

# 忽略 SSL 证书错误（WDP 使用自签名证书）
if (-not ([System.Management.Automation.PSTypeName]'TrustAllCertsPolicy').Type) {
    Add-Type @"
    using System.Net;
    using System.Security.Cryptography.X509Certificates;
    public class TrustAllCertsPolicy : ICertificatePolicy {
        public bool CheckValidationResult(
            ServicePoint sPoint, X509Certificate cert,
            WebRequest wRequest, int certProblem) {
            return true;
        }
    }
"@
    [System.Net.ServicePointManager]::CertificatePolicy = New-Object TrustAllCertsPolicy
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
}

Write-Host "=== Windows Device Portal 文件推送 ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "目标设备: $DeviceIP" -ForegroundColor Yellow
Write-Host "端口: $DevicePort" -ForegroundColor Yellow
Write-Host ""

# 测试连接
Write-Host "测试设备连接..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "$BaseUrl/api/os/info" -Credential $Credential -UseBasicParsing
    if ($response.StatusCode -eq 200) {
        Write-Host "✓ 设备连接成功！" -ForegroundColor Green
        $osInfo = $response.Content | ConvertFrom-Json
        Write-Host "  设备信息: $($osInfo.ComputerName)" -ForegroundColor Gray
    }
} catch {
    Write-Host "✗ 无法连接到设备" -ForegroundColor Red
    Write-Host "  错误: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "请确保:" -ForegroundColor Yellow
    Write-Host "  1. Lumia 950 XL 已开启开发者模式" -ForegroundColor Gray
    Write-Host "  2. Device Portal 已启用" -ForegroundColor Gray
    Write-Host "  3. 设备 IP 地址正确 (当前: $DeviceIP)" -ForegroundColor Gray
    Write-Host "  4. 防火墙允许连接" -ForegroundColor Gray
    Write-Host "  5. WDP 凭据正确" -ForegroundColor Gray
    exit 1
}

Write-Host ""

# 定义要推送的文件列表
$FilesToPush = @(
    @{
        Local = "tinycc/tccmod.h"
        Remote = "LocalAppData/Phase3Probe/tccmod.h"
        Description = "核心头文件"
    },
    @{
        Local = "tinycc/tccmod_loader.c"
        Remote = "LocalAppData/Phase3Probe/tccmod_loader.c"
        Description = "加载器源码"
    },
    @{
        Local = "tinycc/tccmod_builder.c"
        Remote = "LocalAppData/Phase3Probe/tccmod_builder.c"
        Description = "序列化器源码"
    },
    @{
        Local = "tinycc/win32/build/test_module.tccmod"
        Remote = "LocalAppData/Phase3Probe/test_module.tccmod"
        Description = "测试模块"
    },
    @{
        Local = "TCCMOD_FORMAT.md"
        Remote = "LocalAppData/Phase3Probe/TCCMOD_FORMAT.md"
        Description = "格式规范"
    },
    @{
        Local = "QUICK_REFERENCE.md"
        Remote = "LocalAppData/Phase3Probe/QUICK_REFERENCE.md"
        Description = "快速参考"
    }
)

Write-Host "准备推送 $($FilesToPush.Count) 个文件..." -ForegroundColor Yellow
Write-Host ""

$SuccessCount = 0
$FailCount = 0

foreach ($file in $FilesToPush) {
    $LocalPath = Join-Path $PSScriptRoot $file.Local

    if (-not (Test-Path $LocalPath)) {
        Write-Host "✗ 跳过: $($file.Description) (文件不存在)" -ForegroundColor Red
        $FailCount++
        continue
    }

    Write-Host "推送: $($file.Description)" -ForegroundColor Cyan
    Write-Host "  源: $($file.Local)" -ForegroundColor Gray
    Write-Host "  目标: $($file.Remote)" -ForegroundColor Gray

    try {
        # WDP API: PUT /api/filesystem/apps/file
        $uri = "$BaseUrl/api/filesystem/apps/file?knownfolderid=LocalAppData&filename=$($file.Remote)"

        $fileBytes = [System.IO.File]::ReadAllBytes($LocalPath)
        $fileSize = $fileBytes.Length

        # 使用 Invoke-RestMethod 上传文件
        $boundary = [System.Guid]::NewGuid().ToString()
        $LF = "`r`n"

        $bodyLines = (
            "--$boundary",
            "Content-Disposition: form-data; name=`"file`"; filename=`"$(Split-Path $LocalPath -Leaf)`"",
            "Content-Type: application/octet-stream$LF",
            [System.Text.Encoding]::GetEncoding("iso-8859-1").GetString($fileBytes),
            "--$boundary--$LF"
        ) -join $LF

        $response = Invoke-RestMethod -Uri $uri `
            -Method Put `
            -Credential $Credential `
            -ContentType "multipart/form-data; boundary=$boundary" `
            -Body $bodyLines `
            -UseBasicParsing

        Write-Host "  ✓ 上传成功 ($fileSize 字节)" -ForegroundColor Green
        $SuccessCount++

    } catch {
        Write-Host "  ✗ 上传失败: $($_.Exception.Message)" -ForegroundColor Red
        $FailCount++
    }

    Write-Host ""
}

Write-Host "=== 推送完成 ===" -ForegroundColor Cyan
Write-Host "成功: $SuccessCount 个文件" -ForegroundColor Green
Write-Host "失败: $FailCount 个文件" -ForegroundColor Red

if ($FailCount -eq 0) {
    Write-Host ""
    Write-Host "✓ 所有文件推送成功！" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "⚠ 部分文件推送失败" -ForegroundColor Yellow
    exit 1
}
