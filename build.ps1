# Build TachyonDesk (Viewer, Server, and Repeater) in Release|x64 using Visual Studio 2022+.
param(
    [switch]$Rebuild,
    [string]$BuildRoot = (Join-Path $PSScriptRoot 'build'),
    [string]$PlatformToolset = ''
)

$ErrorActionPreference = 'Stop'

function Find-VSInstall {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        # Prefer full VS IDE (with MFC/ATL for .rc files), then any VS with C++ tools.
        foreach ($argsList in @(
            @('-version', '[17.0,18.0)', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.ATLMFC', '-property', 'installationPath'),
            @('-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.ATLMFC', '-property', 'installationPath'),
            @('-version', '[17.0,18.0)', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'),
            @('-latest', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'),
            @('-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath')
        )) {
            $path = & $vswhere @argsList | Select-Object -First 1
            if ($path -and (Test-Path $path)) { return $path }
        }
    }
    foreach ($ver in @('2022', '18')) {
        foreach ($edition in @('Enterprise', 'Professional', 'Community', 'BuildTools')) {
            $candidate = "C:\Program Files\Microsoft Visual Studio\$ver\$edition"
            if (Test-Path (Join-Path $candidate 'MSBuild\Current\Bin\MSBuild.exe')) { return $candidate }
        }
    }
    throw 'Visual Studio 2022+ with C++ desktop workload not found.'
}

function Resolve-Toolset([string]$vsRoot, [string]$requested) {
    if ($requested) { return $requested }
    foreach ($pair in @(
        @{ Toolset = 'v143'; Rel = 'MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\v143' },
        @{ Toolset = 'v145'; Rel = 'MSBuild\Microsoft\VC\v180\Platforms\x64\PlatformToolsets\v145' }
    )) {
        if (Test-Path (Join-Path $vsRoot $pair.Rel)) { return $pair.Toolset }
    }
    return 'v143'
}

$vsRoot = Find-VSInstall
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
$toolset = Resolve-Toolset $vsRoot $PlatformToolset
Write-Host "Using MSBuild: $msbuild (PlatformToolset=$toolset)"

$buildRootNormalized = $BuildRoot.TrimEnd('\', '/') + '\'
New-Item -ItemType Directory -Force (Join-Path $buildRootNormalized 'logs'), (Join-Path $buildRootNormalized 'temp') | Out-Null

$oldTemp = $env:TEMP
$oldTmp = $env:TMP
$failed = $false
$buildTarget = if ($Rebuild) { '/t:Rebuild' } else { '/t:Build' }

try {
    $env:TEMP = Join-Path $buildRootNormalized 'temp'
    $env:TMP = $env:TEMP
    foreach ($entry in @(
        @{ Solution = 'viewer\TachyonViewer.sln'; Log = 'viewer-release-x64.log' },
        @{ Solution = 'server\TachyonServer.sln'; Log = 'server-release-x64.log' },
        @{ Solution = 'repeater\TachyonRepeater.sln'; Log = 'repeater-release-x64.log' }
    )) {
        Write-Host "Building $($entry.Solution)..."
        & $msbuild (Join-Path $PSScriptRoot $entry.Solution) $buildTarget /m:4 /nologo /v:minimal `
            /p:Configuration=Release /p:Platform=x64 "/p:PlatformToolset=$toolset" `
            "/p:TachyonBuildRoot=$buildRootNormalized" `
            /p:PreBuildEventUseInBuild=false /p:PostBuildEventUseInBuild=false `
            /fl "/flp:logfile=$(Join-Path $buildRootNormalized "logs\$($entry.Log)");verbosity=normal;encoding=UTF-8"
        if ($LASTEXITCODE -ne 0) { $failed = $true }
    }
} finally {
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp
}

if ($failed) { exit 1 }
Write-Host "Build succeeded. Output: $(Join-Path $buildRootNormalized 'bin\x64\Release')"
