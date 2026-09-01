[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ArtifactDirectory,

    [Parameter(Mandatory)]
    [ValidateSet('x64', 'arm64')]
    [string] $Architecture,

    [string] $UpgradeFromMsi,

    [switch] $Headless
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedMachine = @{
    x64 = 0x8664
    arm64 = 0xAA64
}[$Architecture]
$expectedTemplate = @{
    x64 = 'x64;1033'
    arm64 = 'Arm64;1033'
}[$Architecture]

function Get-PeMachine {
    param([Parameter(Mandatory)][string] $Path)

    $stream = [IO.File]::OpenRead($Path)
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "$Path is not a PE file."
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadUInt32()
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "$Path has an invalid PE signature."
        }
        return $reader.ReadUInt16()
    }
    finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

function Assert-PeFiles {
    param(
        [Parameter(Mandatory)][string] $Root,
        [Parameter(Mandatory)][int] $Machine,
        [Parameter(Mandatory)][string] $Architecture
    )

    $files = @(Get-ChildItem -LiteralPath $Root -Recurse -File |
        Where-Object { $_.Extension -in '.exe', '.dll' })
    if ($files.Count -eq 0) {
        throw "No EXE or DLL files found under $Root."
    }
    foreach ($file in $files) {
        $actual = Get-PeMachine -Path $file.FullName
        if ($actual -ne $Machine) {
            $expectedRedist = "vc_redist.$Architecture.exe"
            if ($file.Name -ieq $expectedRedist -and $actual -eq 0x014C) {
                Write-Host "Accepted x86 vendor bootstrapper: $($file.FullName)"
                continue
            }
            throw ("Unexpected PE machine for {0}: expected 0x{1:X4}, found 0x{2:X4}." -f
                $file.FullName, $Machine, $actual)
        }
    }
    Write-Host "Verified $($files.Count) PE files under $Root."
}

function Get-MsiMetadata {
    param([Parameter(Mandatory)][string] $Path)

    $installer = New-Object -ComObject WindowsInstaller.Installer
    $database = $installer.OpenDatabase($Path, 0)
    $summary = $database.SummaryInformation(0)
    $view = $database.OpenView("SELECT ``Value`` FROM ``Property`` WHERE ``Property``='ProductCode'")
    [void] $view.Execute()
    $record = $view.Fetch()
    if (-not $record) {
        throw "MSI does not define a ProductCode: $Path"
    }
    return [pscustomobject]@{
        Template = [string] $summary.Property(7)
        PageCount = [int] $summary.Property(14)
        ProductCode = [string] $record.StringData(1)
    }
}

function Test-ProductRegistration {
    param([Parameter(Mandatory)][string] $ProductCode)

    return (Test-Path "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$ProductCode") -or
        (Test-Path "HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\$ProductCode")
}

function Invoke-CheckedProcess {
    param(
        [Parameter(Mandatory)][string] $FilePath,
        [Parameter(Mandatory)][string[]] $Arguments,
        [int] $TimeoutSeconds = 120
    )

    Write-Host "Running: $FilePath $($Arguments -join ' ')"
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        [void] $startInfo.ArgumentList.Add($argument)
    }

    $process = [Diagnostics.Process]::Start($startInfo)
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        $process.WaitForExit()
        throw "$FilePath timed out after $TimeoutSeconds seconds."
    }

    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    if ($stdout) { Write-Host $stdout.TrimEnd() }
    if ($stderr) { Write-Host $stderr.TrimEnd() }
    if ($process.ExitCode -ne 0) {
        throw "$FilePath failed with exit code $($process.ExitCode)."
    }
}

function Invoke-MsiExec {
    param([Parameter(Mandatory)][string[]] $Arguments)

    Invoke-CheckedProcess -FilePath "$env:SystemRoot\System32\msiexec.exe" -Arguments $Arguments
}

function Assert-VersionCommands {
    param(
        [Parameter(Mandatory)][string] $Root,
        [switch] $Headless
    )

    $executables = @('keepassxc-cli.exe')
    if (-not $Headless) {
        $executables += 'KeePassXC.exe'
    } else {
        Write-Host 'Skipping KeePassXC.exe launch on the non-interactive hosted runner.'
    }
    foreach ($name in $executables) {
        $executable = Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $name |
            Select-Object -First 1
        if (-not $executable) {
            throw "$name was not found under $Root."
        }
        Invoke-CheckedProcess -FilePath $executable.FullName -Arguments @('--version') -TimeoutSeconds 30
    }
}

$artifacts = (Resolve-Path -LiteralPath $ArtifactDirectory).Path
$zip = @(Get-ChildItem -LiteralPath $artifacts -Recurse -File -Filter '*.zip')
$msi = @(Get-ChildItem -LiteralPath $artifacts -Recurse -File -Filter '*.msi')
if ($zip.Count -ne 1 -or $msi.Count -ne 1) {
    throw "Expected exactly one ZIP and one MSI; found $($zip.Count) ZIP and $($msi.Count) MSI."
}
$architectureName = if ($Architecture -eq 'arm64') { 'arm64|aarch64' } else { 'x64|amd64|x86_64' }
foreach ($artifact in @($zip[0], $msi[0])) {
    if ($artifact.Name -notmatch $architectureName) {
        throw "Artifact name does not identify the $Architecture target: $($artifact.Name)"
    }
}

$metadata = Get-MsiMetadata -Path $msi[0].FullName
if ($metadata.Template -ne $expectedTemplate) {
    throw "Expected MSI Template '$expectedTemplate', found '$($metadata.Template)'."
}
if ($metadata.PageCount -ne 500) {
    throw "Expected MSI Page Count 500, found $($metadata.PageCount)."
}
Write-Host "MSI Template=$($metadata.Template), PageCount=$($metadata.PageCount)"

$verificationRoot = Join-Path $artifacts ".package-verification-$([Guid]::NewGuid())"
$archiveRoot = Join-Path $verificationRoot 'archive'
$installRoot = Join-Path $verificationRoot 'installed'
$currentInstalled = $false
$previousInstalled = $false
$previousMetadata = $null
New-Item -ItemType Directory -Path $archiveRoot -Force | Out-Null

try {
    Expand-Archive -LiteralPath $zip[0].FullName -DestinationPath $archiveRoot
    Assert-PeFiles -Root $archiveRoot -Machine $expectedMachine -Architecture $Architecture
    Assert-VersionCommands -Root $archiveRoot -Headless:$Headless

    if ($UpgradeFromMsi) {
        $previousMsiPath = (Resolve-Path -LiteralPath $UpgradeFromMsi).Path
        $previousMetadata = Get-MsiMetadata -Path $previousMsiPath
        Invoke-MsiExec -Arguments @(
            '/i', $previousMsiPath, '/qn', '/norestart',
            "INSTALL_ROOT=$installRoot",
            '/l*v', (Join-Path $verificationRoot 'install-previous.log')
        )
        $previousInstalled = $true
        if (-not (Test-ProductRegistration -ProductCode $previousMetadata.ProductCode)) {
            throw "Previous MSI product $($previousMetadata.ProductCode) was not registered."
        }
        Assert-VersionCommands -Root $installRoot -Headless:$Headless
    }

    Invoke-MsiExec -Arguments @(
        '/i', $msi[0].FullName, '/qn', '/norestart',
        "INSTALL_ROOT=$installRoot",
        '/l*v', (Join-Path $verificationRoot 'install-current.log')
    )
    $currentInstalled = $true

    if (-not (Test-Path -LiteralPath $installRoot)) {
        throw "MSI install path was not created: $installRoot"
    }
    if (-not (Test-ProductRegistration -ProductCode $metadata.ProductCode)) {
        throw "MSI product $($metadata.ProductCode) was not registered."
    }
    if ($previousMetadata -and (Test-ProductRegistration -ProductCode $previousMetadata.ProductCode)) {
        throw "Previous MSI product remained registered after the architecture upgrade."
    }

    Assert-PeFiles -Root $installRoot -Machine $expectedMachine -Architecture $Architecture
    Assert-VersionCommands -Root $installRoot -Headless:$Headless

    Invoke-MsiExec -Arguments @(
        '/x', $metadata.ProductCode, '/qn', '/norestart',
        '/l*v', (Join-Path $verificationRoot 'uninstall.log')
    )
    $currentInstalled = $false
    $previousInstalled = $false

    if (Test-ProductRegistration -ProductCode $metadata.ProductCode) {
        throw "MSI product remained registered after uninstall."
    }
    if (Test-Path -LiteralPath $installRoot) {
        throw "MSI install path was not removed after uninstall: $installRoot"
    }
}
finally {
    if ($currentInstalled) {
        & "$env:SystemRoot\System32\msiexec.exe" /x $metadata.ProductCode /qn /norestart
    }
    if ($previousInstalled -and $previousMetadata) {
        & "$env:SystemRoot\System32\msiexec.exe" /x $previousMetadata.ProductCode /qn /norestart
    }
    Remove-Item -LiteralPath $verificationRoot -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Windows $Architecture package verification completed successfully."
