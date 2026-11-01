[CmdletBinding()]
param(
    [string]$OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'
$studioDirectory = $PSScriptRoot
$repositoryRoot = Split-Path -Path $studioDirectory -Parent
$sampleDirectory = Join-Path $repositoryRoot 'Samples\HelloChronos'
$runtimeDirectory = Join-Path $repositoryRoot 'SandboxProbe'
$payloadDirectory = Join-Path $runtimeDirectory 'Payload'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
$armEnvironment = 'E:\Apotheosis\port\arm32-uwp-env.ps1'
$vcTargetsPath = 'C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V140\'

if (-not (Test-Path -LiteralPath $msbuild -PathType Leaf)) {
    throw "MSBuild was not found: $msbuild"
}
if (-not (Test-Path -LiteralPath $armEnvironment -PathType Leaf)) {
    throw "ARM UWP compiler environment was not found: $armEnvironment"
}
if (-not (Test-Path -LiteralPath $vcTargetsPath -PathType Container)) {
    throw "UWP MSBuild targets were not found: $vcTargetsPath"
}

& (Join-Path $studioDirectory 'Build-ChronosProject.ps1') -ProjectDirectory $sampleDirectory
if ($LASTEXITCODE -ne 0) { throw 'Chronos Studio failed to build the demo package.' }

$package = Join-Path $sampleDirectory 'out\com.ouyangquan.chronos.hello.chronos'
if (-not (Test-Path -LiteralPath $package -PathType Leaf)) {
    throw "Demo package was not created: $package"
}

New-Item -ItemType Directory -Force -Path $payloadDirectory | Out-Null
Copy-Item -LiteralPath $package -Destination (Join-Path $payloadDirectory 'HelloChronos.chronos') -Force

if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repositoryRoot 'ChronosArtifacts\RuntimeDemo' }
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$solution = Join-Path $repositoryRoot 'SandboxProbe.sln'
. $armEnvironment
$env:VCTargetsPath = $vcTargetsPath
& $msbuild $solution /m /t:Rebuild /v:minimal `
    '/p:Configuration=Release' '/p:Platform=ARM' '/p:PlatformToolset=v140' `
    '/p:PackageCertificatePassword=test123' '/p:AppxBundle=Never' `
    '/p:GenerateAppxPackageOnBuild=true' '/p:UapAppxPackageBuildMode=Test' `
    ("/p:AppxPackageDir=" + $outputRoot + '\\')
if ($LASTEXITCODE -ne 0) { throw "Chronos Runtime build failed with exit code $LASTEXITCODE." }

$appx = Get-ChildItem -LiteralPath $outputRoot -Recurse -Filter '*.appx' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $appx) { throw 'MSBuild completed but did not produce an AppX.' }

[PSCustomObject]@{
    RuntimeAppx = $appx.FullName
    DemoPackage = $package
    RuntimeName = 'Chronos Runtime'
}
