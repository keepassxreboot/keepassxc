[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('amd64', 'arm64')]
    [string] $PlatformTarget,

    [string] $SourceDirectory = '.',
    [string] $OutputDirectory = 'release',
    [int] $Parallelism = [Environment]::ProcessorCount,
    [int] $TimeoutMinutes = 300,
    [double] $MinimumInitialFreeGiB = 12,
    [double] $MinimumFinalFreeGiB = 2,
    [ValidateRange(1, 300)]
    [int] $DiskPollSeconds = 5
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-FreeGiB {
    param([Parameter(Mandatory)][string] $Path)

    $root = [IO.Path]::GetPathRoot([IO.Path]::GetFullPath($Path))
    return [Math]::Round((Get-PSDrive -Name $root.TrimEnd('\').TrimEnd(':')).Free / 1GB, 2)
}

function Get-DirectorySummary {
    param([string] $Path)

    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject]@{ Count = 0; GiB = 0 }
    }
    $files = @(Get-ChildItem -LiteralPath $Path -File -Recurse -ErrorAction SilentlyContinue)
    $totalBytes = if ($files.Count -gt 0) {
        ($files | Measure-Object -Property Length -Sum).Sum
    } else {
        0
    }
    return [pscustomobject]@{
        Count = $files.Count
        GiB = [Math]::Round($totalBytes / 1GB, 2)
    }
}

$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$output = [IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDirectory))
$cachePath = $env:VCPKG_DEFAULT_BINARY_CACHE
if (-not $cachePath -and $env:VCPKG_BINARY_SOURCES -match 'files,([^,;]+),') {
    $cachePath = $Matches[1]
}

$initialFreeGiB = Get-FreeGiB -Path $source
$initialCache = Get-DirectorySummary -Path $cachePath
Write-Host "runner.image=$env:ImageOS"
Write-Host "runner.image_version=$env:ImageVersion"
Write-Host "runner.arch=$env:PROCESSOR_ARCHITECTURE"
Write-Host "visual_studio=$(& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationVersion)"
Write-Host "cmake=$((& cmake --version | Select-Object -First 1) -replace '^cmake version ', '')"
Write-Host "cpack=$((& cpack --version | Select-Object -First 1) -replace '^cpack version ', '')"
Write-Host "vcpkg.installation_root=$env:VCPKG_INSTALLATION_ROOT"
Write-Host "vcpkg.host_processor=$([Runtime.InteropServices.RuntimeInformation]::OSArchitecture)"
Write-Host "vcpkg.target=$PlatformTarget"
Write-Host "vcpkg.binary_sources=$env:VCPKG_BINARY_SOURCES"
Write-Host "vcpkg.cache_before.count=$($initialCache.Count)"
Write-Host "vcpkg.cache_before.gib=$($initialCache.GiB)"
Write-Host "disk.initial_free_gib=$initialFreeGiB"

if ($initialFreeGiB -lt $MinimumInitialFreeGiB) {
    throw "Initial free space is $initialFreeGiB GiB; at least $MinimumInitialFreeGiB GiB is required."
}

$python = (Get-Command python -ErrorAction Stop).Source
$arguments = @(
    'release-tool.py',
    'build',
    '0.0.0',
    '--src-dir', $source,
    '--output-dir', $output,
    '--platform-target', $PlatformTarget,
    '--parallelism', $Parallelism,
    '--build-qt',
    '--with-tests',
    '--yes',
    '--snapshot'
)

$start = Get-Date
$stopwatch = [Diagnostics.Stopwatch]::StartNew()
$process = $null
$timedOut = $false
$headroomBreached = $false
$minimumFreeGiB = $initialFreeGiB
try {
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $python
    $startInfo.WorkingDirectory = $source
    $startInfo.UseShellExecute = $false
    foreach ($argument in $arguments) {
        [void] $startInfo.ArgumentList.Add([string] $argument)
    }

    Write-Host "build.command=$python $($arguments -join ' ')"
    $process = [Diagnostics.Process]::Start($startInfo)
    while (-not $process.WaitForExit($DiskPollSeconds * 1000)) {
        $sampleFreeGiB = Get-FreeGiB -Path $source
        $minimumFreeGiB = [Math]::Min($minimumFreeGiB, $sampleFreeGiB)
        Write-Host "disk.sample_free_gib=$sampleFreeGiB"
        if ($sampleFreeGiB -lt $MinimumFinalFreeGiB) {
            $headroomBreached = $true
            $process.Kill($true)
            $process.WaitForExit()
            break
        }
        if ($stopwatch.Elapsed.TotalMinutes -ge $TimeoutMinutes) {
            $timedOut = $true
            $process.Kill($true)
            $process.WaitForExit()
            break
        }
    }
}
finally {
    $stopwatch.Stop()
    $finalFreeGiB = Get-FreeGiB -Path $source
    $minimumFreeGiB = [Math]::Min($minimumFreeGiB, $finalFreeGiB)
    $finalCache = Get-DirectorySummary -Path $cachePath
    Write-Host "build.started_utc=$($start.ToUniversalTime().ToString('o'))"
    Write-Host "build.elapsed_minutes=$([Math]::Round($stopwatch.Elapsed.TotalMinutes, 2))"
    Write-Host "disk.final_free_gib=$finalFreeGiB"
    Write-Host "disk.minimum_free_gib=$minimumFreeGiB"
    Write-Host "disk.consumed_gib=$([Math]::Round($initialFreeGiB - $finalFreeGiB, 2))"
    Write-Host "vcpkg.cache_after.count=$($finalCache.Count)"
    Write-Host "vcpkg.cache_after.gib=$($finalCache.GiB)"
}

if ($timedOut) {
    throw "Release build exceeded the $TimeoutMinutes minute limit."
}
if ($headroomBreached) {
    throw "Free space fell to $minimumFreeGiB GiB during the build; at least $MinimumFinalFreeGiB GiB is required."
}
if (-not $process -or $process.ExitCode -ne 0) {
    $exitCode = if ($process) { $process.ExitCode } else { 'not-started' }
    throw "Release build failed with exit code $exitCode."
}
if ($finalFreeGiB -lt $MinimumFinalFreeGiB) {
    throw "Final free space is $finalFreeGiB GiB; at least $MinimumFinalFreeGiB GiB is required."
}

Write-Host 'Release build completed successfully.'
