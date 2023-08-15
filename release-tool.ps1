<#
.SYNOPSIS
KeePassXC Release Tool

.DESCRIPTION
Commands:
  merge      Merge release branch into main branch and create release tags
  build      Build and package binary release from sources
  sign       Sign previously compiled release packages

.NOTES
The following are descriptions of certain parameters:
  -Vcpkg           Specify VCPKG toolchain location (example: C:\vcpkg)
  -Tag             Release tag to check out (defaults to version number)
  -Snapshot        Build current HEAD without checkout out Tag
  -CMakeGenerator  Override the default CMake generator
  -CMakeOptions    Additional CMake options for compiling the sources
  -CPackGenerators Set CPack generators (default: WIX;ZIP)
  -Compiler        Compiler to use (example: g++, clang, msbuild)
  -MakeOptions     Options to pass to the make program
  -SignBuild       Perform platform specific App Signing before packaging
  -SignKey         Specify the App Signing Key/Identity
  -TimeStamp       Explicitly set the timestamp server to use for appsign
  -SourceBranch    Source branch to merge from (default: 'release/$Version')
  -TargetBranch    Target branch to merge to (default: master)
  -VSToolChain     Specify Visual Studio Toolchain by name if more than one is available
#>

param(
    [Parameter(ParameterSetName = "merge", Mandatory, Position = 0)]
    [switch] $Merge,
    [Parameter(ParameterSetName = "build", Mandatory, Position = 0)]
    [switch] $Build,
    [Parameter(ParameterSetName = "sign", Mandatory, Position = 0)]
    [switch] $Sign,

    [Parameter(ParameterSetName = "merge", Mandatory, Position = 1)]
    [Parameter(ParameterSetName = "build", Mandatory, Position = 1)]
    [Parameter(ParameterSetName = "sign", Mandatory, Position = 1)]
    [string] $Version,

    [Parameter(ParameterSetName = "build", Mandatory)]
    [string] $Vcpkg,

    [Parameter(ParameterSetName = "sign", Mandatory)]
    [SupportsWildcards()]
    [string[]] $SignFiles,

    # [Parameter(ParameterSetName = "build")]
    # [switch] $DryRun,
    [Parameter(ParameterSetName = "build")]
    [switch] $Snapshot,
    [Parameter(ParameterSetName = "build")]
    [switch] $SignBuild,
    
    [Parameter(ParameterSetName = "build")]
    [string] $CMakeGenerator = "Ninja",
    [Parameter(ParameterSetName = "build")]
    [string] $CMakeOptions,
    [Parameter(ParameterSetName = "build")]
    [string] $CPackGenerators = "WIX;ZIP",
    [Parameter(ParameterSetName = "build")]
    [string] $Compiler,
    [Parameter(ParameterSetName = "build")]
    [string] $MakeOptions,
    [Parameter(ParameterSetName = "build")]
    [Parameter(ParameterSetName = "sign")]
    [string] $SignKey,
    [Parameter(ParameterSetName = "build")]
    [Parameter(ParameterSetName = "sign")]
    [string] $Timestamp = "http://timestamp.sectigo.com",
    [Parameter(ParameterSetName = "merge")]
    [Parameter(ParameterSetName = "build")]
    [Parameter(ParameterSetName = "sign")]
    [string] $GpgKey = "CFB4C2166397D0D2",
    [Parameter(ParameterSetName = "merge")]
    [Parameter(ParameterSetName = "build")]
    [string] $SourceDir = ".",
    [Parameter(ParameterSetName = "build")]
    [string] $OutDir = ".\release",
    [Parameter(ParameterSetName = "merge")]
    [Parameter(ParameterSetName = "build")]
    [string] $Tag,
    [Parameter(ParameterSetName = "merge")]
    [string] $SourceBranch,
    [Parameter(ParameterSetName = "build")]
    [string] $VSToolChain,
    [Parameter(ParameterSetName = "merge")]
    [Parameter(ParameterSetName = "build")]
    [Parameter(ParameterSetName = "sign")]
    [string] $ExtraPath
)

# Helper function definitions
function Test-RequiredPrograms {
    # If any of these fail they will throw an exception terminating the script
    if ($Build) {
        Get-Command git | Out-Null
        Get-Command cmake | Out-Null
    }
    if ($Merge) {
        Get-Command git | Out-Null
        Get-Command tx | Out-Null
        Get-Command lupdate | Out-Null
    }
    if ($Sign -or $SignBuild) {
        if ($SignKey.Length) {
            Get-Command signtool | Out-Null
        }
        Get-Command gpg | Out-Null
    }
}

function Test-VersionInFiles {
    # Check CMakeLists.txt
    $Major, $Minor, $Patch = $Version.split(".", 3)
    if (!(Select-String "$SourceDir\CMakeLists.txt" -pattern "KEEPASSXC_VERSION_MAJOR `"$Major`"" -Quiet) `
            -or !(Select-String "$SourceDir\CMakeLists.txt" -pattern "KEEPASSXC_VERSION_MINOR `"$Minor`"" -Quiet) `
            -or !(Select-String "$SourceDir\CMakeLists.txt" -pattern "KEEPASSXC_VERSION_PATCH `"$Patch`"" -Quiet)) {
        throw "CMakeLists.txt has not been updated to $Version."
    }

    # Check Changelog
    if (!(Select-String "$SourceDir\CHANGELOG.md" -pattern "^## $Version \(\d{4}-\d{2}-\d{2}\)$" -Quiet)) {
        throw "CHANGELOG.md does not contain a section for $Version."
    }

    # Check AppStreamInfo
    if (!(Select-String "$SourceDir\share\linux\org.keepassxc.KeePassXC.appdata.xml" `
                -pattern "<release version=`"$Version`" date=`"\d{4}-\d{2}-\d{2}`">" -Quiet)) {
        throw "share/linux/org.keepassxc.KeePassXC.appdata.xml does not contain a section for $Version."
    }
}

function Test-WorkingTreeClean {
    & git diff-index --quiet HEAD --
    if ($LASTEXITCODE) {
        throw "Current working tree is not clean! Please commit or unstage any changes."
    }
}

function Invoke-VSToolchain([String] $Toolchain, [String] $Path, [String] $Arch) {
    # Find Visual Studio installations
    $vs = Get-CimInstance MSFT_VSInstance
    if ($vs.count -eq 0) {
        $err = "No Visual Studio installations found, download one from https://visualstudio.com/downloads."
        $err = "$err`nIf Visual Studio is installed, you may need to repair the install then restart."
        throw $err
    }

    $VSBaseDir = $vs[0].InstallLocation
    if ($Toolchain) {
        # Try to find the specified toolchain by name
        foreach ($_ in $vs) {
            if ($_.Name -eq $Toolchain) {
                $VSBaseDir = $_.InstallLocation
                break
            }
        }
    } elseif ($vs.count -gt 1) {
        # Ask the user which install to use
        $i = 0
        foreach ($_ in $vs) {
            $i = $i + 1
            $i.ToString() + ") " + $_.Name | Write-Host
        }
        $i = Read-Host -Prompt "Which Visual Studio installation do you want to use?"
        $i = [Convert]::ToInt32($i, 10) - 1
        if ($i -lt 0 -or $i -ge $vs.count) {
            throw "Invalid selection made"
        }
        $VSBaseDir = $vs[$i].InstallLocation
    }
    
    # Bootstrap the specified VS Toolchain
    Import-Module "$VSBaseDir\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
    Enter-VsDevShell -VsInstallPath $VSBaseDir -Arch $Arch -StartInPath $Path | Write-Host
    Write-Host # Newline after command output
}

function Invoke-Cmd([string] $command, [string[]] $options = @(), [switch] $maskargs, [switch] $quiet) {
    $call = ('{0} {1}' -f $command, ($options -Join ' '))
    if ($maskargs) {
        Write-Host "$command <masked>" -ForegroundColor DarkGray
    }
    else {
        Write-Host $call -ForegroundColor DarkGray
    }
    if ($quiet) {
        Invoke-Expression $call > $null
    } else {
        Invoke-Expression $call
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to run command: {0}" -f $command
    }
    Write-Host #insert newline after command output
}

function Invoke-SignFiles([string[]] $files, [string] $key, [string] $time) {
    if (!(Test-Path -Path "$key" -PathType leaf)) {
        throw "Appsign key file was not found! ($key)"
    }
    if ($files.Length -eq 0) {
        return
    }

    Write-Host "Signing files using $key" -ForegroundColor Cyan
    $KeyPassword = Read-Host "Key password: " -MaskInput

    foreach ($_ in $files) {
        Write-Host "Signing file '$_' using Microsoft signtool..."
        Invoke-Cmd "signtool" "sign -f `"$key`" -p `"$KeyPassword`" -d `"KeePassXC`" -td sha256 -fd sha256 -tr `"$time`" `"$_`"" -maskargs
    }
}

function Invoke-GpgSignFiles([string[]] $files, [string] $key) {
    if ($files.Length -eq 0) {
        return
    }

    Write-Host "Signing files using GPG key $key" -ForegroundColor Cyan

    foreach ($_ in $files) {
        Write-Host "Signing file '$_' and creating DIGEST..."
        if (Test-Path "$_.sig") {
            Remove-Item "$_.sig"
        }
        Invoke-Cmd "gpg" "--output `"$_.sig`" --armor --local-user `"$key`" --detach-sig `"$_`""
        $FileName = (Get-Item $_).Name
        (Get-FileHash "$_" SHA256).Hash + " *$FileName" | Out-File "$_.DIGEST" -NoNewline
    }
}


# Handle errors and restore state
$OrigDir = (Get-Location).Path
$OrigBranch = & git rev-parse --abbrev-ref HEAD
$ErrorActionPreference = 'Stop'
trap {
    Write-Host "Restoring state..." -ForegroundColor Yellow
    & git checkout $OrigBranch
    Set-Location "$OrigDir"
}

Write-Host "KeePassXC Release Preparation Helper" -ForegroundColor Green
Write-Host "Copyright (C) 2022 KeePassXC Team <https://keepassxc.org/>`n" -ForegroundColor Green

# Prepend extra PATH locations as specified
if ($ExtraPath) {
    $env:Path = "$ExtraPath;$env:Path"
}

# Resolve absolute directory for paths
$SourceDir = (Resolve-Path $SourceDir).Path

# Check format of -Version
if ($Version -notmatch "^\d+\.\d+\.\d+(-Beta\d*)?$") {
    throw "Invalid format for -Version input"
}

# Check platform
if (!$IsWindows) {
    throw "The PowerShell release tool is not available for Linux or macOS at this time."
}

if ($Merge) {
    Test-RequiredPrograms

    # Change to SourceDir
    Set-Location "$SourceDir"

    Test-VersionInFiles
    Test-WorkingTreeClean

    if (!$SourceBranch.Length) {
        $SourceBranch = & git branch --show-current
    }

    if ($SourceBranch -notmatch "^release/.*$") {
        throw "Must be on a release/* branch to continue."
    }

    # Update translation files
    Write-Host "Updating source translation file..."
    Invoke-Cmd "lupdate" "-no-ui-lines -disable-heuristic similartext -locations none", `
        "-extensions c,cpp,h,js,mm,qrc,ui -no-obsolete ./src -ts share/translations/keepassxc_en.ts"

    Write-Host "Pulling updated translations from Transifex..."
    Invoke-Cmd "tx" "pull -af --minimum-perc=60 -r keepassxc.share-translations-keepassxc-en-ts--develop"

    # Only commit if there are changes
    $changes = & git status --porcelain
    if ($changes.Length -gt 0) {
        Write-Host "Committing translation updates..."
        Invoke-Cmd "git" "add -A ./share/translations/" -quiet
        Invoke-Cmd "git" "commit -m `"Update translations`"" -quiet
    }

    # Read the version release notes from CHANGELOG
    $Changelog = ""
    $ReadLine = $false
    Get-Content "CHANGELOG.md" | ForEach-Object {
        if ($ReadLine) {
            if ($_ -match "^## ") {
                $ReadLine = $false
            } else {
                $Changelog += $_ + "`n"
            }
        } elseif ($_ -match "$Version \(\d{4}-\d{2}-\d{2}\)") {
            $ReadLine = $true
        }
    }

    Write-Host "Creating tag for '$Version'..."
    $tmp = New-TemporaryFile
    "Release $Version`n$Changelog" | Out-File $tmp.FullName
    Invoke-Cmd "git" "tag -a `"$Version`" -F `"$tmp`" -s" -quiet
    Remove-Item $tmp.FullName -Force

    Write-Host "Moving latest tag..."
    Invoke-Cmd "git" "tag -f -a `"latest`" -m `"Latest stable release`" -s" -quiet

    Write-Host "All done!"
    Write-Host "Please merge the release branch back into the develop branch now and then push your changes."
    Write-Host "Don't forget to also push the tags using 'git push --tags'."
} elseif ($Build) {
    $Vcpkg = (Resolve-Path "$Vcpkg/scripts/buildsystems/vcpkg.cmake").Path

    # Find Visual Studio and establish build environment
    Invoke-VSToolchain $VSToolChain $SourceDir -Arch "amd64"

    Test-RequiredPrograms

    if ($Snapshot) {
        $Tag = "HEAD"
        $SourceBranch = & git rev-parse --abbrev-ref HEAD
        $ReleaseName = "$Version-snapshot"
        $CMakeOptions = "-DKEEPASSXC_BUILD_TYPE=Snapshot -DOVERRIDE_VERSION=`"$ReleaseName`" $CMakeOptions"
        Write-Host "Using current branch '$SourceBranch' to build." -ForegroundColor Cyan
    } else {
        Test-WorkingTreeClean

        # Clear output directory
        if (Test-Path $OutDir) {
            Remove-Item $OutDir -Recurse
        }
        
        if ($Version -match "-beta\d*$") {
            $CMakeOptions = "-DKEEPASSXC_BUILD_TYPE=PreRelease $CMakeOptions"
        } else {
            $CMakeOptions = "-DKEEPASSXC_BUILD_TYPE=Release $CMakeOptions"
        }

        # Setup Tag if not defined then checkout tag
        if ($Tag -eq "" -or $Tag -eq $null) {
            $Tag = $Version
        }
        Write-Host "Checking out tag 'tags/$Tag' to build." -ForegroundColor Cyan
        Invoke-Cmd "git" "checkout `"tags/$Tag`""
    }

    # Create directories
    New-Item "$OutDir" -ItemType Directory -Force | Out-Null
    $OutDir = (Resolve-Path $OutDir).Path

    $BuildDir = "$OutDir\build-release"
    New-Item "$BuildDir" -ItemType Directory -Force | Out-Null

    # Enter build directory
    Set-Location "$BuildDir"

    # Setup CMake options
    $CMakeOptions = "-DWITH_XC_ALL=ON -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release $CMakeOptions"
    $CMakeOptions = "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=`"$Vcpkg`" -DX_VCPKG_APPLOCAL_DEPS_INSTALL=ON $CMakeOptions"

    Write-Host "Configuring build..." -ForegroundColor Cyan
    Invoke-Cmd "cmake" "-G `"$CMakeGenerator`" $CMakeOptions `"$SourceDir`""

    Write-Host "Compiling sources..." -ForegroundColor Cyan
    Invoke-Cmd "cmake" "--build . --config Release -- $MakeOptions"
    
    if ($SignBuild) {
        $files = Get-ChildItem "$BuildDir\src" -Include "*keepassxc*.exe", "*keepassxc*.dll" -Recurse -File | ForEach-Object { $_.FullName }
        Invoke-SignFiles $files $SignKey $Timestamp
    }

    Write-Host "Create deployment packages..." -ForegroundColor Cyan
    Invoke-Cmd "cpack" "-G `"$CPackGenerators`""
    Move-Item "$BuildDir\keepassxc-*" -Destination "$OutDir" -Force

    if ($SignBuild) {
        # Enter output directory
        Set-Location -Path "$OutDir"

        # Sign MSI files using AppSign key
        $files = Get-ChildItem $OutDir -Include "*.msi" -Name
        Invoke-SignFiles $files $SignKey $Timestamp

        # Sign all output files using the GPG key then hash them
        $files = Get-ChildItem $OutDir -Include "*.msi", "*.zip" -Name
        Invoke-GpgSignFiles $files $GpgKey
    }

    # Restore state
    Invoke-Command {git checkout $OrigBranch}
    Set-Location "$OrigDir"
} elseif ($Sign) {
    if (Test-Path $SignKey) {
        # Need to include path to signtool program
        Invoke-VSToolchain $VSToolChain $SourceDir -Arch "amd64"
    }

    Test-RequiredPrograms

    # Resolve wildcard paths
    $ResolvedFiles = @()
    foreach ($_ in $SignFiles) {
        $ResolvedFiles += (Get-ChildItem $_ -File | ForEach-Object { $_.FullName })
    }

    $AppSignFiles = $ResolvedFiles.Where({ $_ -match "\.(msi|exe|dll)$" })
    Invoke-SignFiles $AppSignFiles $SignKey $Timestamp

    $GpgSignFiles = $ResolvedFiles.Where({ $_ -match "\.(msi|zip|gz|xz|dmg|appimage)$" })
    Invoke-GpgSignFiles $GpgSignFiles $GpgKey
}

# SIG # Begin signature block
# MIIfXAYJKoZIhvcNAQcCoIIfTTCCH0kCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCC3+AltPeIvGycP
# LJr+5kqYiFnGPXyfdXgAkgrw+aI/AqCCGSAwggU6MIIEIqADAgECAhBYotctjMD9
# icz/IeDU7cdKMA0GCSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAkdCMRswGQYDVQQI
# ExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoT
# D1NlY3RpZ28gTGltaXRlZDEkMCIGA1UEAxMbU2VjdGlnbyBSU0EgQ29kZSBTaWdu
# aW5nIENBMB4XDTIxMDMxNTAwMDAwMFoXDTI0MDMxNDIzNTk1OVowgaExCzAJBgNV
# BAYTAlVTMQ4wDAYDVQQRDAUyMjMxNTERMA8GA1UECAwIVmlyZ2luaWExEjAQBgNV
# BAcMCUZyYW5jb25pYTEbMBkGA1UECQwSNjY1MyBBdWRyZXkgS2F5IEN0MR4wHAYD
# VQQKDBVEcm9pZE1vbmtleSBBcHBzLCBMTEMxHjAcBgNVBAMMFURyb2lkTW9ua2V5
# IEFwcHMsIExMQzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALAH0v/7
# XOVxc5Auhi92tgBZL0Hm6L7sT1xcKfrBwHg12ZpFs0zeR1ZzyduM44d45y3aNXNW
# 7+klHVJqAj5XVHUF/OB/O5bnljKIeupdAZ8v3GGokBZK91BGKe+WRn5ZjdDGc6HN
# xCT4FMth1TCrTg7eMy/u+cfp+4ur8dcSyAM5tkLsTIoS1VtZWjiepjS1RO+Ile9E
# j+wUM3wO9Qt5/BlYi8XsbXU0V4oi3bj6EMLO9UEq0SfsM2YUY7UIkAHtLPiMV7BX
# gw/WC+IwB8ZtIGpq/JEME4bt51pJVvVmrjzbKgc0Cz6akhArZIa9QooAkrbAINvO
# Cm+7mx/PBK2lzSECAwEAAaOCAZAwggGMMB8GA1UdIwQYMBaAFA7hOqhTOjHVir7B
# u61nGgOFrTQOMB0GA1UdDgQWBBTu7ZZnt+omJoze71KXMDAYGGhYfTAOBgNVHQ8B
# Af8EBAMCB4AwDAYDVR0TAQH/BAIwADATBgNVHSUEDDAKBggrBgEFBQcDAzARBglg
# hkgBhvhCAQEEBAMCBBAwSgYDVR0gBEMwQTA1BgwrBgEEAbIxAQIBAwIwJTAjBggr
# BgEFBQcCARYXaHR0cHM6Ly9zZWN0aWdvLmNvbS9DUFMwCAYGZ4EMAQQBMEMGA1Ud
# HwQ8MDowOKA2oDSGMmh0dHA6Ly9jcmwuc2VjdGlnby5jb20vU2VjdGlnb1JTQUNv
# ZGVTaWduaW5nQ0EuY3JsMHMGCCsGAQUFBwEBBGcwZTA+BggrBgEFBQcwAoYyaHR0
# cDovL2NydC5zZWN0aWdvLmNvbS9TZWN0aWdvUlNBQ29kZVNpZ25pbmdDQS5jcnQw
# IwYIKwYBBQUHMAGGF2h0dHA6Ly9vY3NwLnNlY3RpZ28uY29tMA0GCSqGSIb3DQEB
# CwUAA4IBAQAPbD9O3krI9tfYz6FZXCCqkqbjaeTpo3Ye9Kn4paWsHa37OIv7UhFf
# CrtLRXunZ7Vkry5cvMGNQNUaMFy6CHmEYb3kwZWW3EImuv9uTUd7rYvszBXF8flv
# kysT+8L9wdxLEQHUBnBnFgqMM99HzVuINVyH75d4qa9TF9PhcajsxwmpPgr9NwvC
# KNJv8BaxdH6vYFcQCqCygbfuST99s8qKaknTuHF39E1hWkTfcT3fMJDVONzW0/cN
# ourxylLqMZRjk007NGCnaYZwYfKW/pD/F/jmo28eKoVVy129j2h/RAWODl5gOvis
# sNr6aSz1/Ul3xoNYpyx8IGeWiFMoh99tMIIF9TCCA92gAwIBAgIQHaJIMG+bJhjQ
# guCWfTPTajANBgkqhkiG9w0BAQwFADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
# Ck5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUg
# VVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlm
# aWNhdGlvbiBBdXRob3JpdHkwHhcNMTgxMTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5
# WjB8MQswCQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVyMRAw
# DgYDVQQHEwdTYWxmb3JkMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxJDAiBgNV
# BAMTG1NlY3RpZ28gUlNBIENvZGUgU2lnbmluZyBDQTCCASIwDQYJKoZIhvcNAQEB
# BQADggEPADCCAQoCggEBAIYijTKFehifSfCWL2MIHi3cfJ8Uz+MmtiVmKUCGVEZ0
# MWLFEO2yhyemmcuVMMBW9aR1xqkOUGKlUZEQauBLYq798PgYrKf/7i4zIPoMGYmo
# bHutAMNhodxpZW0fbieW15dRhqb0J+V8aouVHltg1X7XFpKcAC9o95ftanK+ODtj
# 3o+/bkxBXRIgCFnoOc2P0tbPBrRXBbZOoT5Xax+YvMRi1hsLjcdmG0qfnYHEckC1
# 4l/vC0X/o84Xpi1VsLewvFRqnbyNVlPG8Lp5UEks9wO5/i9lNfIi6iwHr0bZ+UYc
# 3Ix8cSjz/qfGFN1VkW6KEQ3fBiSVfQ+noXw62oY1YdMCAwEAAaOCAWQwggFgMB8G
# A1UdIwQYMBaAFFN5v1qqK0rPVIDh2JvAnfKyA2bLMB0GA1UdDgQWBBQO4TqoUzox
# 1Yq+wbutZxoDha00DjAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIB
# ADAdBgNVHSUEFjAUBggrBgEFBQcDAwYIKwYBBQUHAwgwEQYDVR0gBAowCDAGBgRV
# HSAAMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0LmNvbS9V
# U0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2BggrBgEFBQcB
# AQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNvbS9VU0VS
# VHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDovL29jc3Au
# dXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEATWNQ7Uc0SmGk295qKoyb
# 8QAAHh1iezrXMsL2s+Bjs/thAIiaG20QBwRPvrjqiXgi6w9G7PNGXkBGiRL0C3da
# nCpBOvzW9Ovn9xWVM8Ohgyi33i/klPeFM4MtSkBIv5rCT0qxjyT0s4E307dksKYj
# alloUkJf/wTr4XRleQj1qZPea3FAmZa6ePG5yOLDCBaxq2NayBWAbXReSnV+pbjD
# bLXP30p5h1zHQE1jNfYw08+1Cg4LBH+gS667o6XQhACTPlNdNKUANWlsvp8gJRAN
# GftQkGG+OY96jk32nw4e/gdREmaDJhlIlc5KycF/8zoFm/lv34h/wCOe0h5DekUx
# wZxNqfBZslkZ6GqNKQQCd3xLS81wvjqyVVp4Pry7bwMQJXcVNIr5NsxDkuS6T/Fi
# kyglVyn7URnHoSVAaoRXxrKdsbwcCtp8Z359LukoTBh+xHsxQXGaSynsCz1XUNLK
# 3f2eBVHlRHjdAd6xdZgNVCT98E7j4viDvXK6yz067vBeF5Jobchh+abxKgoLpbn0
# nu6YMgWFnuv5gynTxix9vTp3Los3QqBqgu07SqqUEKThDfgXxbZaeTMYkuO1dfih
# 6Y4KJR7kHvGfWocj/5+kUZ77OYARzdu1xKeogG/lU9Tg46LC0lsa+jImLWpXcBw8
# pFguo/NbSwfcMlnzh6cabVgwggbsMIIE1KADAgECAhAwD2+s3WaYdHypRjaneC25
# MA0GCSqGSIb3DQEBDAUAMIGIMQswCQYDVQQGEwJVUzETMBEGA1UECBMKTmV3IEpl
# cnNleTEUMBIGA1UEBxMLSmVyc2V5IENpdHkxHjAcBgNVBAoTFVRoZSBVU0VSVFJV
# U1QgTmV0d29yazEuMCwGA1UEAxMlVVNFUlRydXN0IFJTQSBDZXJ0aWZpY2F0aW9u
# IEF1dGhvcml0eTAeFw0xOTA1MDIwMDAwMDBaFw0zODAxMTgyMzU5NTlaMH0xCzAJ
# BgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcT
# B1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDElMCMGA1UEAxMcU2Vj
# dGlnbyBSU0EgVGltZSBTdGFtcGluZyBDQTCCAiIwDQYJKoZIhvcNAQEBBQADggIP
# ADCCAgoCggIBAMgbAa/ZLH6ImX0BmD8gkL2cgCFUk7nPoD5T77NawHbWGgSlzkeD
# tevEzEk0y/NFZbn5p2QWJgn71TJSeS7JY8ITm7aGPwEFkmZvIavVcRB5h/RGKs3E
# Wsnb111JTXJWD9zJ41OYOioe/M5YSdO/8zm7uaQjQqzQFcN/nqJc1zjxFrJw06PE
# 37PFcqwuCnf8DZRSt/wflXMkPQEovA8NT7ORAY5unSd1VdEXOzQhe5cBlK9/gM/R
# EQpXhMl/VuC9RpyCvpSdv7QgsGB+uE31DT/b0OqFjIpWcdEtlEzIjDzTFKKcvSb/
# 01Mgx2Bpm1gKVPQF5/0xrPnIhRfHuCkZpCkvRuPd25Ffnz82Pg4wZytGtzWvlr7a
# TGDMqLufDRTUGMQwmHSCIc9iVrUhcxIe/arKCFiHd6QV6xlV/9A5VC0m7kUaOm/N
# 14Tw1/AoxU9kgwLU++Le8bwCKPRt2ieKBtKWh97oaw7wW33pdmmTIBxKlyx3GSuT
# lZicl57rjsF4VsZEJd8GEpoGLZ8DXv2DolNnyrH6jaFkyYiSWcuoRsDJ8qb/fVfb
# Enb6ikEk1Bv8cqUUotStQxykSYtBORQDHin6G6UirqXDTYLQjdprt9v3GEBXc/Bx
# o/tKfUU2wfeNgvq5yQ1TgH36tjlYMu9vGFCJ10+dM70atZ2h3pVBeqeDAgMBAAGj
# ggFaMIIBVjAfBgNVHSMEGDAWgBRTeb9aqitKz1SA4dibwJ3ysgNmyzAdBgNVHQ4E
# FgQUGqH4YRkgD8NBd0UojtE1XwYSBFUwDgYDVR0PAQH/BAQDAgGGMBIGA1UdEwEB
# /wQIMAYBAf8CAQAwEwYDVR0lBAwwCgYIKwYBBQUHAwgwEQYDVR0gBAowCDAGBgRV
# HSAAMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0LmNvbS9V
# U0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2BggrBgEFBQcB
# AQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNvbS9VU0VS
# VHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDovL29jc3Au
# dXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAbVSBpTNdFuG1U4GRdd8D
# ejILLSWEEbKw2yp9KgX1vDsn9FqguUlZkClsYcu1UNviffmfAO9Aw63T4uRW+VhB
# z/FC5RB9/7B0H4/GXAn5M17qoBwmWFzztBEP1dXD4rzVWHi/SHbhRGdtj7BDEA+N
# 5Pk4Yr8TAcWFo0zFzLJTMJWk1vSWVgi4zVx/AZa+clJqO0I3fBZ4OZOTlJux3LJt
# QW1nzclvkD1/RXLBGyPWwlWEZuSzxWYG9vPWS16toytCiiGS/qhvWiVwYoFzY16g
# u9jc10rTPa+DBjgSHSSHLeT8AtY+dwS8BDa153fLnC6NIxi5o8JHHfBd1qFzVwVo
# mqfJN2Udvuq82EKDQwWli6YJ/9GhlKZOqj0J9QVst9JkWtgqIsJLnfE5XkzeSD2b
# NJaaCV+O/fexUpHOP4n2HKG1qXUfcb9bQ11lPVCBbqvw0NP8srMftpmWJvQ8eYtc
# ZMzN7iea5aDADHKHwW5NWtMe6vBE5jJvHOsXTpTDeGUgOw9Bqh/poUGd/rG4oGUq
# NODeqPk85sEwu8CgYyz8XBYAqNDEf+oRnR4GxqZtMl20OAkrSQeq/eww2vGnL8+3
# /frQo4TZJ577AWZ3uVYQ4SBuxq6x+ba6yDVdM3aO8XwgDCp3rrWiAoa6Ke60WgCx
# jKvj+QrJVF3UuWp0nr1Irpgwggb1MIIE3aADAgECAhA5TCXhfKBtJ6hl4jvZHSLU
# MA0GCSqGSIb3DQEBDAUAMH0xCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVy
# IE1hbmNoZXN0ZXIxEDAOBgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28g
# TGltaXRlZDElMCMGA1UEAxMcU2VjdGlnbyBSU0EgVGltZSBTdGFtcGluZyBDQTAe
# Fw0yMzA1MDMwMDAwMDBaFw0zNDA4MDIyMzU5NTlaMGoxCzAJBgNVBAYTAkdCMRMw
# EQYDVQQIEwpNYW5jaGVzdGVyMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxLDAq
# BgNVBAMMI1NlY3RpZ28gUlNBIFRpbWUgU3RhbXBpbmcgU2lnbmVyICM0MIICIjAN
# BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEApJMoUkvPJ4d2pCkcmTjA5w7U0Rzs
# aMsBZOSKzXewcWWCvJ/8i7u7lZj7JRGOWogJZhEUWLK6Ilvm9jLxXS3AeqIO4OBW
# ZO2h5YEgciBkQWzHwwj6831d7yGawn7XLMO6EZge/NMgCEKzX79/iFgyqzCz2Ix6
# lkoZE1ys/Oer6RwWLrCwOJVKz4VQq2cDJaG7OOkPb6lampEoEzW5H/M94STIa7GZ
# 6A3vu03lPYxUA5HQ/C3PVTM4egkcB9Ei4GOGp7790oNzEhSbmkwJRr00vOFLUHty
# 4Fv9GbsfPGoZe267LUQqvjxMzKyKBJPGV4agczYrgZf6G5t+iIfYUnmJ/m53N9e7
# UJ/6GCVPE/JefKmxIFopq6NCh3fg9EwCSN1YpVOmo6DtGZZlFSnF7TMwJeaWg4Ga
# 9mBmkFgHgM1Cdaz7tJHQxd0BQGq2qBDu9o16t551r9OlSxihDJ9XsF4lR5F0zXUS
# 0Zxv5F4Nm+x1Ju7+0/WSL1KF6NpEUSqizADKh2ZDoxsA76K1lp1irScL8htKycOU
# QjeIIISoh67DuiNye/hU7/hrJ7CF9adDhdgrOXTbWncC0aT69c2cPcwfrlHQe2zY
# HS0RQlNxdMLlNaotUhLZJc/w09CRQxLXMn2YbON3Qcj/HyRU726txj5Ve/Fchzpk
# 8WBLBU/vuS/sCRMCAwEAAaOCAYIwggF+MB8GA1UdIwQYMBaAFBqh+GEZIA/DQXdF
# KI7RNV8GEgRVMB0GA1UdDgQWBBQDDzHIkSqTvWPz0V1NpDQP0pUBGDAOBgNVHQ8B
# Af8EBAMCBsAwDAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEFBQcDCDBK
# BgNVHSAEQzBBMDUGDCsGAQQBsjEBAgEDCDAlMCMGCCsGAQUFBwIBFhdodHRwczov
# L3NlY3RpZ28uY29tL0NQUzAIBgZngQwBBAIwRAYDVR0fBD0wOzA5oDegNYYzaHR0
# cDovL2NybC5zZWN0aWdvLmNvbS9TZWN0aWdvUlNBVGltZVN0YW1waW5nQ0EuY3Js
# MHQGCCsGAQUFBwEBBGgwZjA/BggrBgEFBQcwAoYzaHR0cDovL2NydC5zZWN0aWdv
# LmNvbS9TZWN0aWdvUlNBVGltZVN0YW1waW5nQ0EuY3J0MCMGCCsGAQUFBzABhhdo
# dHRwOi8vb2NzcC5zZWN0aWdvLmNvbTANBgkqhkiG9w0BAQwFAAOCAgEATJtlWPrg
# ec/vFcMybd4zket3WOLrvctKPHXefpRtwyLHBJXfZWlhEwz2DJ71iSBewYfHAyTK
# x6XwJt/4+DFlDeDrbVFXpoyEUghGHCrC3vLaikXzvvf2LsR+7fjtaL96VkjpYeWa
# OXe8vrqRZIh1/12FFjQn0inL/+0t2v++kwzsbaINzMPxbr0hkRojAFKtl9RieCqE
# eajXPawhj3DDJHk6l/ENo6NbU9irALpY+zWAT18ocWwZXsKDcpCu4MbY8pn76rSS
# ZXwHfDVEHa1YGGti+95sxAqpbNMhRnDcL411TCPCQdB6ljvDS93NkiZ0dlw3oJok
# nk5fTtOPD+UTT1lEZUtDZM9I+GdnuU2/zA2xOjDQoT1IrXpl5Ozf4AHwsypKOazB
# pPmpfTXQMkCgsRkqGCGyyH0FcRpLJzaq4Jgcg3Xnx35LhEPNQ/uQl3YqEqxAwXBb
# mQpA+oBtlGF7yG65yGdnJFxQjQEg3gf3AdT4LhHNnYPl+MolHEQ9J+WwhkcqCxuE
# dn17aE+Nt/cTtO2gLe5zD9kQup2ZLHzXdR+PEMSU5n4k5ZVKiIwn1oVmHfmuZHaR
# 6Ej+yFUK7SnDH944psAU+zI9+KmDYjbIw74Ahxyr+kpCHIkD3PVcfHDZXXhO7p9e
# IOYJanwrCKNI9RX8BE/fzSEceuX1jhrUuUAxggWSMIIFjgIBATCBkDB8MQswCQYD
# VQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQHEwdT
# YWxmb3JkMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxJDAiBgNVBAMTG1NlY3Rp
# Z28gUlNBIENvZGUgU2lnbmluZyBDQQIQWKLXLYzA/YnM/yHg1O3HSjANBglghkgB
# ZQMEAgEFAKCBhDAYBgorBgEEAYI3AgEMMQowCKACgAChAoAAMBkGCSqGSIb3DQEJ
# AzEMBgorBgEEAYI3AgEEMBwGCisGAQQBgjcCAQsxDjAMBgorBgEEAYI3AgEVMC8G
# CSqGSIb3DQEJBDEiBCBp7GODztdZALJXKtlDO7iMqjlod5DXJQUaVMMwaRVDkzAN
# BgkqhkiG9w0BAQEFAASCAQB8i7HZnhNcD6S7hrG+nk6bDcg8LyL+C3QOnmxIQKA3
# +TQB02qB83WI+ErrH7GQHgi+7kB4K8NYs1dK/yYIp6pwgXUnYqQlsQCYzMRk9Shn
# gvJWO04dV3V17NHfAXHT/+gHKTOOUJf58/Yabo87/vu4K5gE2g3TOrMHm0G9x1k8
# PXgW6mzMD6xEz0tuvXdGZ8BSZ5VlDYV5ITchn8Eni29RTSIBIbZHbMWI5Gcsbzqd
# ZLKHmVOoT2Las0VWNzq96+1X1HjFGhPqAIm19ByZyGI3OO9fgP6lfGuHyE2eyYUp
# MKQ6qr8nfPzmp3bF0JLSGV3pEViDOqRgkkQmOXHfHlqsoYIDSzCCA0cGCSqGSIb3
# DQEJBjGCAzgwggM0AgEBMIGRMH0xCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVh
# dGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3Rp
# Z28gTGltaXRlZDElMCMGA1UEAxMcU2VjdGlnbyBSU0EgVGltZSBTdGFtcGluZyBD
# QQIQOUwl4XygbSeoZeI72R0i1DANBglghkgBZQMEAgIFAKB5MBgGCSqGSIb3DQEJ
# AzELBgkqhkiG9w0BBwEwHAYJKoZIhvcNAQkFMQ8XDTIzMDgxNTAzMDcyMlowPwYJ
# KoZIhvcNAQkEMTIEMDyZLb3/c3iiuzTHkbucrdYjiOpKcwtj+r0u6p2ypdNrYVlH
# k9k7PaDY1LfCeRwkPTANBgkqhkiG9w0BAQEFAASCAgAKipaEem0CK2ALaiJYcBkB
# AUYGZl6MpMKScYGmiqopzfp3sLp6EiM9lWtU0skS/d81DDlT/Hm55NNEoNEjB1R8
# 3OygPagYohu/UoNLPUCZjLMsHGfsW2KXxr0OODpjLU9YkiucaQNHTjhO0zpm18A4
# btEgi8pooELnNx2uX/ZB+HwnYUsCMGE7WnYshDXGmE8puiCUTddf9R5uqbRrCVJS
# G6jf+YhNvUeJSVgwnJoWqfHMKoHo7CdYgGR91RUIVdsbMO/2QosBTm1HzvWfB82C
# 0s43uZNPP05a6ITmzjytk5OYEA+xwYFM6qo8TjeLGrgAeHyk7BUYdk5hUS2lQ+Th
# SU0AivCqsAQvMcwKd69fhMGN0hrQu+ydKhQe6pmm8J5/DRoV/pNYVcecgYMXowrI
# EXK8l0ihsk/pEpoONcpNEacQ5U/miylp7f37WlGU7bY3EWc8BOENYMRUA+hKMXka
# kPQ1RID0BU4OyjuI76+klf9xBngSu1xR/2aw9/+TlzWGNRpjKRLQkIFJ8iaFX91B
# KfJ4+B6HELBEn2wjLvL+fLLn3ajp2fdeRf1CWWzOHQox15KHzEK7o6zZ43IzWOeO
# blzPpVotmoJ8ncBZXGtLDmS64Z04J58KvpfV2g02/EIHZhry2E3sG3jWAv0WY2z7
# fgsXlQSQR8WnONsJJQ95CA==
# SIG # End signature block
