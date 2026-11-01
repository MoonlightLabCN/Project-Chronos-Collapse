[CmdletBinding(DefaultParameterSetName = 'build')]
param(
    [Parameter(Mandatory = $true, Position = 0, ParameterSetName = 'build')]
    [string]$ProjectDirectory,

    [Parameter(ParameterSetName = 'build')]
    [string]$OutputDirectory = '',

    [Parameter(ParameterSetName = 'build')]
    [switch]$KeepIntermediate
)

$ErrorActionPreference = 'Stop'

function Get-SafeRelativePath {
    param([string]$Value, [string]$FieldName)

    if ([string]::IsNullOrWhiteSpace($Value) -or
        [IO.Path]::IsPathRooted($Value) -or
        $Value -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "chronos.json field '$FieldName' must be a non-empty relative path."
    }
    return $Value.Replace('/', '\\')
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

$studioDirectory = $PSScriptRoot
$repositoryRoot = Split-Path -Path $studioDirectory -Parent
$compiler = Join-Path $repositoryRoot 'tinycc\win32\arm-win32-tcc.exe'
$projectRoot = (Resolve-Path -LiteralPath $ProjectDirectory).Path
$descriptorPath = Join-Path $projectRoot 'chronos.json'

if (-not (Test-Path -LiteralPath $projectRoot -PathType Container)) {
    throw "Project directory was not found: $ProjectDirectory"
}
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "ARM TinyCC was not found: $compiler"
}
if (-not (Test-Path -LiteralPath $descriptorPath -PathType Leaf)) {
    throw "Project descriptor was not found: $descriptorPath"
}

$descriptor = Get-Content -LiteralPath $descriptorPath -Raw | ConvertFrom-Json
if ($descriptor.format -ne 'chronos-project') {
    throw "chronos.json format must be 'chronos-project'."
}
if ([int]$descriptor.formatVersion -ne 1) {
    throw 'Only chronos project formatVersion 1 is supported.'
}
if ([string]::IsNullOrWhiteSpace($descriptor.id) -or $descriptor.id -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{2,63}$') {
    throw 'Project id must be 3-64 characters: letters, digits, dot, underscore, or hyphen.'
}
if ([string]::IsNullOrWhiteSpace($descriptor.name)) {
    throw 'Project name is required.'
}
if ([string]::IsNullOrWhiteSpace($descriptor.entry) -or $descriptor.entry -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
    throw 'Entry must be a valid C symbol name.'
}

$sourceRelative = Get-SafeRelativePath $descriptor.source 'source'
$viewRelative = Get-SafeRelativePath $descriptor.view 'view'
$sourcePath = Join-Path $projectRoot $sourceRelative
$viewPath = Join-Path $projectRoot $viewRelative
if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) { throw "C source was not found: $sourcePath" }
if (-not (Test-Path -LiteralPath $viewPath -PathType Leaf)) { throw "XAML view was not found: $viewPath" }

if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'out' }
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$safeId = $descriptor.id -replace '[^A-Za-z0-9._-]', '_'
$staging = Join-Path $outputRoot ('.chronos-stage-' + [Guid]::NewGuid().ToString('N'))
$modulePath = Join-Path $staging 'module.tccmod'
$packagePath = Join-Path $outputRoot ($safeId + '.chronos')

try {
    New-Item -ItemType Directory -Force -Path $staging | Out-Null

    # The runtime owns the only permitted host symbols.  This intentionally
    # prevents an authored module from importing desktop DLLs or a CRT.
    & $compiler -nostdlib -rdynamic -o $modulePath $sourcePath
    if ($LASTEXITCODE -ne 0) { throw "TinyCC failed with exit code $LASTEXITCODE." }
    if (-not (Test-Path -LiteralPath $modulePath -PathType Leaf)) { throw 'TinyCC did not create module.tccmod.' }

    $header = [IO.File]::ReadAllBytes($modulePath)
    if ($header.Length -lt 80 -or [BitConverter]::ToUInt32($header, 0) -ne 0x4D434354) {
        throw 'TinyCC output is not a valid TCCM module.'
    }

    Copy-Item -LiteralPath $viewPath -Destination (Join-Path $staging 'view.xaml') -Force
    $packageManifest = [ordered]@{
        format = 'chronos-package'
        formatVersion = 1
        id = [string]$descriptor.id
        name = [string]$descriptor.name
        architecture = 'arm-thumb2'
        apiVersion = 1
        entry = [string]$descriptor.entry
        module = 'module.tccmod'
        view = 'view.xaml'
        moduleSha256 = Get-Sha256 $modulePath
        viewSha256 = Get-Sha256 (Join-Path $staging 'view.xaml')
    }
    $packageManifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $staging 'manifest.json') -Encoding utf8

    # .chronos deliberately is not a ZIP. The Runtime has to run on Windows
    # 10 Mobile, where a fixed-layout container avoids an archive dependency.
    # Layout: CHRP / u16 major / u16 minor / u32 manifest / u32 module /
    # u32 XAML / manifest UTF-8 / module bytes / XAML UTF-8.
    $manifestBytes = [Text.Encoding]::UTF8.GetBytes((Get-Content -LiteralPath (Join-Path $staging 'manifest.json') -Raw))
    $moduleBytes = [IO.File]::ReadAllBytes($modulePath)
    $viewBytes = [IO.File]::ReadAllBytes((Join-Path $staging 'view.xaml'))
    if (Test-Path -LiteralPath $packagePath) { Remove-Item -LiteralPath $packagePath -Force }
    $stream = [IO.File]::Open($packagePath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        $writer = New-Object IO.BinaryWriter($stream, [Text.Encoding]::UTF8, $true)
        try {
            $writer.Write([byte[]][char[]]'CHRP')
            $writer.Write([uint16]1)
            $writer.Write([uint16]0)
            $writer.Write([uint32]$manifestBytes.Length)
            $writer.Write([uint32]$moduleBytes.Length)
            $writer.Write([uint32]$viewBytes.Length)
            $writer.Write($manifestBytes)
            $writer.Write($moduleBytes)
            $writer.Write($viewBytes)
        }
        finally { $writer.Dispose() }
    }
    finally { $stream.Dispose() }

    [PSCustomObject]@{
        Package = $packagePath
        Module = $modulePath
        Name = $packageManifest.name
        Id = $packageManifest.id
        Entry = $packageManifest.entry
        Bytes = (Get-Item -LiteralPath $packagePath).Length
    }
}
finally {
    if (-not $KeepIntermediate -and (Test-Path -LiteralPath $staging)) {
        Remove-Item -LiteralPath $staging -Recurse -Force
    }
}
