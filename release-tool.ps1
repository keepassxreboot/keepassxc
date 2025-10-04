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
  -SignCert        Specify the App Signing Certificate
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
    [X509Certificate] $SignCert,
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
    if ($Sign) {
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
    $vs = Get-CimInstance MSFT_VSInstance -Namespace root/cimv2/vs
  
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

function Find-SignCert() {
    $certs = Get-ChildItem Cert:\CurrentUser\My -codesign
    if ($certs.Count -eq 0) {
        throw "No code signing certificate found in User certificate store"
    } elseif ($certs.Count -gt 1) {
        # Ask the user which to use
        $i = 0
        foreach ($_ in $certs) {
            $i = $i + 1
            $i.ToString() + ") $($_.Thumbprint) - $($_.NotAfter)" | Write-Host
        }
        $i = Read-Host -Prompt "Which certificate do you want to use?"
        $i = [Convert]::ToInt32($i, 10) - 1
        if ($i -lt 0 -or $i -ge $certs.count) {
            throw "Invalid selection made"
        }
        return $certs[$i]
    } else {
        Write-Host "Found signing certificate: $($certs[0].Subject) ($($certs[0].Thumbprint))" -ForegroundColor Cyan
        Write-Host
        return $certs[0]
    }
}

function Invoke-SignFiles([string[]] $files, [X509Certificate] $cert, [string] $time, [bool] $signtool = $false) {
    if ($files.Length -eq 0) {
        return
    }

    Write-Host "Signing files using $($cert.Subject) ($($cert.Thumbprint))" -ForegroundColor Cyan
    
    foreach ($file in $files) {
        $sig = Get-AuthenticodeSignature -FilePath "$file" -ErrorAction SilentlyContinue
        if ($sig.Status -ne "Valid") {
            Write-Host "Signing file '$file'"
            if ($signtool) {
                $filename = (Get-Item $file).BaseName
                & signtool sign /fd SHA256 /sha1 $($cert.Thumbprint) /t $time /d "$filename" "$file"
                if ($LASTEXITCODE -ne 0) {
                    Write-Host "Failed to sign file '$file'" -ForegroundColor Red
                }
            } else {
                $tmp = Set-AuthenticodeSignature -Certificate $cert -FilePath "$file" -TimestampServer "$time" -HashAlgorithm "SHA256"
            }
        }
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

    if ($SignBuild -and !$SignCert) {
        $SignCert = Find-SignCert
    }

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
        $VcpkgDir = $BuildDir + "\vcpkg_installed\"
        if (Test-Path $VcpkgDir) {
            $files = Get-ChildItem $VcpkgDir -Filter "*.dll" -Recurse -File | 
                Where-Object {$_.FullName -notlike "$VcpkgDir*debug\*" -and $_.FullName -notlike "$VcpkgDir*tools\*"} | 
                ForEach-Object {$_.FullName}
        }
        $files += Get-ChildItem "$BuildDir\src" -Include "*keepassxc*.exe", "*keepassxc*.dll" -Recurse -File | ForEach-Object { $_.FullName }
        Invoke-SignFiles $files $SignCert $Timestamp
    }

    Write-Host "Create deployment packages..." -ForegroundColor Cyan
    Invoke-Cmd "cpack" "-G `"$CPackGenerators`""
    Move-Item "$BuildDir\keepassxc-*" -Destination "$OutDir" -Force

    if ($SignBuild) {
        # Enter output directory
        Set-Location -Path "$OutDir"

        # Sign MSI files using AppSign key and signtool
        $files = Get-ChildItem $OutDir -Include "*.msi" -Name
        Invoke-SignFiles $files $SignCert $Timestamp $true

        # Sign all output files using the GPG key then hash them
        $files = Get-ChildItem $OutDir -Include "*.msi", "*.zip" -Name
        Invoke-GpgSignFiles $files $GpgKey
    }

    # Restore state
    Invoke-Command {git checkout $OrigBranch}
    Set-Location "$OrigDir"
} elseif ($Sign) {
    Test-RequiredPrograms

    if (!$SignCert) {
        $SignCert = Find-SignCert
    }

    # Resolve wildcard paths
    $ResolvedFiles = @()
    foreach ($_ in $SignFiles) {
        $ResolvedFiles += (Get-ChildItem $_ -File | ForEach-Object { $_.FullName })
    }

    # Sign msi files using signtool
    $AppSignFiles = $ResolvedFiles.Where({ $_ -match "\.msi$" })
    Invoke-SignFiles $AppSignFiles $SignCert $Timestamp $true

    # Sign exe and dll files using Set-AuthenticodeSignature
    $AppSignFiles = $ResolvedFiles.Where({ $_ -match "\.(exe|dll)$" })
    Invoke-SignFiles $AppSignFiles $SignCert $Timestamp

    # Write signature side cars for all binary installer/zip files
    $GpgSignFiles = $ResolvedFiles.Where({ $_ -match "\.(msi|zip|gz|xz|dmg|appimage)$" })
    Invoke-GpgSignFiles $GpgSignFiles $GpgKey
}

# SIG # Begin signature block
# MIImWgYJKoZIhvcNAQcCoIImSzCCJkcCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCDMXWw29negqhYw
# +NaENXckg/4v4xVLBT1H4SC9KgA1OaCCH28wggYUMIID/KADAgECAhB6I67aU2mW
# D5HIPlz0x+M/MA0GCSqGSIb3DQEBDAUAMFcxCzAJBgNVBAYTAkdCMRgwFgYDVQQK
# Ew9TZWN0aWdvIExpbWl0ZWQxLjAsBgNVBAMTJVNlY3RpZ28gUHVibGljIFRpbWUg
# U3RhbXBpbmcgUm9vdCBSNDYwHhcNMjEwMzIyMDAwMDAwWhcNMzYwMzIxMjM1OTU5
# WjBVMQswCQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMSwwKgYD
# VQQDEyNTZWN0aWdvIFB1YmxpYyBUaW1lIFN0YW1waW5nIENBIFIzNjCCAaIwDQYJ
# KoZIhvcNAQEBBQADggGPADCCAYoCggGBAM2Y2ENBq26CK+z2M34mNOSJjNPvIhKA
# VD7vJq+MDoGD46IiM+b83+3ecLvBhStSVjeYXIjfa3ajoW3cS3ElcJzkyZlBnwDE
# JuHlzpbN4kMH2qRBVrjrGJgSlzzUqcGQBaCxpectRGhhnOSwcjPMI3G0hedv2eNm
# GiUbD12OeORN0ADzdpsQ4dDi6M4YhoGE9cbY11XxM2AVZn0GiOUC9+XE0wI7CQKf
# OUfigLDn7i/WeyxZ43XLj5GVo7LDBExSLnh+va8WxTlA+uBvq1KO8RSHUQLgzb1g
# bL9Ihgzxmkdp2ZWNuLc+XyEmJNbD2OIIq/fWlwBp6KNL19zpHsODLIsgZ+WZ1AzC
# s1HEK6VWrxmnKyJJg2Lv23DlEdZlQSGdF+z+Gyn9/CRezKe7WNyxRf4e4bwUtrYE
# 2F5Q+05yDD68clwnweckKtxRaF0VzN/w76kOLIaFVhf5sMM/caEZLtOYqYadtn03
# 4ykSFaZuIBU9uCSrKRKTPJhWvXk4CllgrwIDAQABo4IBXDCCAVgwHwYDVR0jBBgw
# FoAU9ndq3T/9ARP/FqFsggIv0Ao9FCUwHQYDVR0OBBYEFF9Y7UwxeqJhQo1SgLqz
# YZcZojKbMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMBMGA1Ud
# JQQMMAoGCCsGAQUFBwMIMBEGA1UdIAQKMAgwBgYEVR0gADBMBgNVHR8ERTBDMEGg
# P6A9hjtodHRwOi8vY3JsLnNlY3RpZ28uY29tL1NlY3RpZ29QdWJsaWNUaW1lU3Rh
# bXBpbmdSb290UjQ2LmNybDB8BggrBgEFBQcBAQRwMG4wRwYIKwYBBQUHMAKGO2h0
# dHA6Ly9jcnQuc2VjdGlnby5jb20vU2VjdGlnb1B1YmxpY1RpbWVTdGFtcGluZ1Jv
# b3RSNDYucDdjMCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5zZWN0aWdvLmNvbTAN
# BgkqhkiG9w0BAQwFAAOCAgEAEtd7IK0ONVgMnoEdJVj9TC1ndK/HYiYh9lVUacah
# RoZ2W2hfiEOyQExnHk1jkvpIJzAMxmEc6ZvIyHI5UkPCbXKspioYMdbOnBWQUn73
# 3qMooBfIghpR/klUqNxx6/fDXqY0hSU1OSkkSivt51UlmJElUICZYBodzD3M/SFj
# eCP59anwxs6hwj1mfvzG+b1coYGnqsSz2wSKr+nDO+Db8qNcTbJZRAiSazr7KyUJ
# Go1c+MScGfG5QHV+bps8BX5Oyv9Ct36Y4Il6ajTqV2ifikkVtB3RNBUgwu/mSiSU
# ice/Jp/q8BMk/gN8+0rNIE+QqU63JoVMCMPY2752LmESsRVVoypJVt8/N3qQ1c6F
# ibbcRabo3azZkcIdWGVSAdoLgAIxEKBeNh9AQO1gQrnh1TA8ldXuJzPSuALOz1Uj
# b0PCyNVkWk7hkhVHfcvBfI8NtgWQupiaAeNHe0pWSGH2opXZYKYG4Lbukg7HpNi/
# KqJhue2Keak6qH9A8CeEOB7Eob0Zf+fU+CCQaL0cJqlmnx9HCDxF+3BLbUufrV64
# EbTI40zqegPZdA+sXCmbcZy6okx/SjwsusWRItFA3DE8MORZeFb6BmzBtqKJ7l93
# 9bbKBy2jvxcJI98Va95Q5JnlKor3m0E7xpMeYRriWklUPsetMSf2NvUQa/E5vVye
# fQIwggYaMIIEAqADAgECAhBiHW0MUgGeO5B5FSCJIRwKMA0GCSqGSIb3DQEBDAUA
# MFYxCzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxLTArBgNV
# BAMTJFNlY3RpZ28gUHVibGljIENvZGUgU2lnbmluZyBSb290IFI0NjAeFw0yMTAz
# MjIwMDAwMDBaFw0zNjAzMjEyMzU5NTlaMFQxCzAJBgNVBAYTAkdCMRgwFgYDVQQK
# Ew9TZWN0aWdvIExpbWl0ZWQxKzApBgNVBAMTIlNlY3RpZ28gUHVibGljIENvZGUg
# U2lnbmluZyBDQSBSMzYwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQCb
# K51T+jU/jmAGQ2rAz/V/9shTUxjIztNsfvxYB5UXeWUzCxEeAEZGbEN4QMgCsJLZ
# UKhWThj/yPqy0iSZhXkZ6Pg2A2NVDgFigOMYzB2OKhdqfWGVoYW3haT29PSTahYk
# wmMv0b/83nbeECbiMXhSOtbam+/36F09fy1tsB8je/RV0mIk8XL/tfCK6cPuYHE2
# 15wzrK0h1SWHTxPbPuYkRdkP05ZwmRmTnAO5/arnY83jeNzhP06ShdnRqtZlV59+
# 8yv+KIhE5ILMqgOZYAENHNX9SJDm+qxp4VqpB3MV/h53yl41aHU5pledi9lCBbH9
# JeIkNFICiVHNkRmq4TpxtwfvjsUedyz8rNyfQJy/aOs5b4s+ac7IH60B+Ja7TVM+
# EKv1WuTGwcLmoU3FpOFMbmPj8pz44MPZ1f9+YEQIQty/NQd/2yGgW+ufflcZ/ZE9
# o1M7a5Jnqf2i2/uMSWymR8r2oQBMdlyh2n5HirY4jKnFH/9gRvd+QOfdRrJZb1sC
# AwEAAaOCAWQwggFgMB8GA1UdIwQYMBaAFDLrkpr/NZZILyhAQnAgNpFcF4XmMB0G
# A1UdDgQWBBQPKssghyi47G9IritUpimqF6TNDDAOBgNVHQ8BAf8EBAMCAYYwEgYD
# VR0TAQH/BAgwBgEB/wIBADATBgNVHSUEDDAKBggrBgEFBQcDAzAbBgNVHSAEFDAS
# MAYGBFUdIAAwCAYGZ4EMAQQBMEsGA1UdHwREMEIwQKA+oDyGOmh0dHA6Ly9jcmwu
# c2VjdGlnby5jb20vU2VjdGlnb1B1YmxpY0NvZGVTaWduaW5nUm9vdFI0Ni5jcmww
# ewYIKwYBBQUHAQEEbzBtMEYGCCsGAQUFBzAChjpodHRwOi8vY3J0LnNlY3RpZ28u
# Y29tL1NlY3RpZ29QdWJsaWNDb2RlU2lnbmluZ1Jvb3RSNDYucDdjMCMGCCsGAQUF
# BzABhhdodHRwOi8vb2NzcC5zZWN0aWdvLmNvbTANBgkqhkiG9w0BAQwFAAOCAgEA
# Bv+C4XdjNm57oRUgmxP/BP6YdURhw1aVcdGRP4Wh60BAscjW4HL9hcpkOTz5jUug
# 2oeunbYAowbFC2AKK+cMcXIBD0ZdOaWTsyNyBBsMLHqafvIhrCymlaS98+QpoBCy
# KppP0OcxYEdU0hpsaqBBIZOtBajjcw5+w/KeFvPYfLF/ldYpmlG+vd0xqlqd099i
# ChnyIMvY5HexjO2AmtsbpVn0OhNcWbWDRF/3sBp6fWXhz7DcML4iTAWS+MVXeNLj
# 1lJziVKEoroGs9Mlizg0bUMbOalOhOfCipnx8CaLZeVme5yELg09Jlo8BMe80jO3
# 7PU8ejfkP9/uPak7VLwELKxAMcJszkyeiaerlphwoKx1uHRzNyE6bxuSKcutisqm
# KL5OTunAvtONEoteSiabkPVSZ2z76mKnzAfZxCl/3dq3dUNw4rg3sTCggkHSRqTq
# lLMS7gjrhTqBmzu1L90Y1KWN/Y5JKdGvspbOrTfOXyXvmPL6E52z1NZJ6ctuMFBQ
# ZH3pwWvqURR8AgQdULUvrxjUYbHHj95Ejza63zdrEcxWLDX6xWls/GDnVNueKjWU
# H3fTv1Y8Wdho698YADR7TNx8X8z2Bev6SivBBOHY+uqiirZtg0y9ShQoPzmCcn63
# Syatatvx157YK9hlcPmVoa1oDE5/L9Uo2bC5a4CH2RwwggZJMIIEsaADAgECAhAG
# Qz/MzOQzqJLMF7dGpYxlMA0GCSqGSIb3DQEBDAUAMFQxCzAJBgNVBAYTAkdCMRgw
# FgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxKzApBgNVBAMTIlNlY3RpZ28gUHVibGlj
# IENvZGUgU2lnbmluZyBDQSBSMzYwHhcNMjQwMjIzMDAwMDAwWhcNMjcwMjIyMjM1
# OTU5WjBgMQswCQYDVQQGEwJVUzERMA8GA1UECAwIVmlyZ2luaWExHjAcBgNVBAoM
# FURyb2lkTW9ua2V5IEFwcHMsIExMQzEeMBwGA1UEAwwVRHJvaWRNb25rZXkgQXBw
# cywgTExDMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAuJtEjRyetghx
# 6Abi1cpMT88uT6nIcTe3AyUvdSkjCtUM8Gat0YJfqTxokb9dBzJa7j8YWOUU1Yc4
# EDXoYYtVRE+1UkdPAcXNMf2hNXGI45iZVwhBPQZBU4QfKltzYqrjAZgDvxeYd68q
# ImjzUfrCY3uZHwEIuCewmNMPpEgbdjuSXDyBAKKBtaO2iqyaJpqcC39QnDKlXMic
# DPqqH5fI7wK7Lg9f4BwOsaO4P68I3pOv7L/6E5GR9+hTj6txhxFz/yCbDxN1PUvD
# sGaXjMmVeP2M95fkwOFwut5yBESDIwAGEWUFsTJ32hSmE74+xG6rVqtueayV7U9c
# GURznSk9ZlTUqQOW9Z4K+pu29gTZ9zVWlONIsQR7QXfGKZWF+Xik6rTujSRTTsK7
# QNMYzBI6b9v0nD2pEWuGZDXIO5o5N2HzXEFlwxCFY483yWSObHNBp9PFtiDueqv+
# 8vrN+lsirZlDFCxI6hW+F8oYp3XxHdSqxsMRTqbO6dUjH2Tyd0G5fbyT8Rid7DbP
# 6p/apzIrdFOM0kdcKLmppYBp7BInTdjbWJYhtuORIUZQbUOSM71vYCUHj7xkckiY
# YmkUf0XH8xx8jqgVWseBW63gCEowhCEYxaWt0QGyXJ6UrlV4WTUCWzxm45I5OQoo
# fymUvdutKgr9bR3nJ5yS/c+E3KnqJhkCAwEAAaOCAYkwggGFMB8GA1UdIwQYMBaA
# FA8qyyCHKLjsb0iuK1SmKaoXpM0MMB0GA1UdDgQWBBQta729krTac3CUndU0S0Dd
# DscjHTAOBgNVHQ8BAf8EBAMCB4AwDAYDVR0TAQH/BAIwADATBgNVHSUEDDAKBggr
# BgEFBQcDAzBKBgNVHSAEQzBBMDUGDCsGAQQBsjEBAgEDAjAlMCMGCCsGAQUFBwIB
# FhdodHRwczovL3NlY3RpZ28uY29tL0NQUzAIBgZngQwBBAEwSQYDVR0fBEIwQDA+
# oDygOoY4aHR0cDovL2NybC5zZWN0aWdvLmNvbS9TZWN0aWdvUHVibGljQ29kZVNp
# Z25pbmdDQVIzNi5jcmwweQYIKwYBBQUHAQEEbTBrMEQGCCsGAQUFBzAChjhodHRw
# Oi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29QdWJsaWNDb2RlU2lnbmluZ0NBUjM2
# LmNydDAjBggrBgEFBQcwAYYXaHR0cDovL29jc3Auc2VjdGlnby5jb20wDQYJKoZI
# hvcNAQEMBQADggGBAJSy5YPCbh9ZsuDCKgDuzOWZzNza4/FrA+kT7EitDezYN3S/
# P0EVc0tPbgYAKfNqY+ihAMyjZHdgybfBWhGzUTDo+HEipcnZ2pgwPadsw23jJ8MN
# 1tdms9iKDakIQ2MVsB7cGFRU8QjLovkPdZkyLcjuYbkiZRoNoKlhmrOOf6n1oCwX
# VJ9ONJijc+Lr3+4EIqZ39ET2+uI9Wg9Bfd9XrDZfYFEcRJjNzRpCtHb26aIzV/Xi
# MWasHRPaII34SzD0BmaPbsLeGW1UGvW3tQcgVNdT/uajegmShVb+c5J5ktRSJ0cq
# yxmTAYaeMuA6IxG1f6kui1SAFQs2lzlGyEgxgiNGo7cHHN2KidhrBL3U2bGr9Tkd
# p3gmV+Gj3esCdQzJE4aqmUZvIvHpkrair4qbLFZRNozAZJn2SIeQa5u2U0ZmvcAr
# 1C7S3JVLP3t9LKE0mlFkV9pbIU97ND3iH3tO0Zb3SvCK/XjO1PZVb8EXsi67wbfM
# SWAwi2CETDonb7+gBjCCBmIwggTKoAMCAQICEQCkKTtuHt3XpzQIh616TrckMA0G
# CSqGSIb3DQEBDAUAMFUxCzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExp
# bWl0ZWQxLDAqBgNVBAMTI1NlY3RpZ28gUHVibGljIFRpbWUgU3RhbXBpbmcgQ0Eg
# UjM2MB4XDTI1MDMyNzAwMDAwMFoXDTM2MDMyMTIzNTk1OVowcjELMAkGA1UEBhMC
# R0IxFzAVBgNVBAgTDldlc3QgWW9ya3NoaXJlMRgwFgYDVQQKEw9TZWN0aWdvIExp
# bWl0ZWQxMDAuBgNVBAMTJ1NlY3RpZ28gUHVibGljIFRpbWUgU3RhbXBpbmcgU2ln
# bmVyIFIzNjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBANOElfRupFN4
# 8j0QS3gSBzzclIFTZ2Gsn7BjsmBF659/kpA2Ey7NXK3MP6JdrMBNU8wdmkf+SSIy
# jX++UAYWtg3Y/uDRDyg8RxHeHRJ+0U1jHEyH5uPdk1ttiPC3x/gOxIc9P7Gn3OgW
# 7DQc4x07exZ4DX4XyaGDq5LoEmk/BdCM1IelVMKB3WA6YpZ/XYdJ9JueOXeQObSQ
# /dohQCGyh0FhmwkDWKZaqQBWrBwZ++zqlt+z/QYTgEnZo6dyIo2IhXXANFkCHutL
# 8765NBxvolXMFWY8/reTnFxk3MajgM5NX6wzWdWsPJxYRhLxtJLSUJJ5yWRNw+NB
# qH1ezvFs4GgJ2ZqFJ+Dwqbx9+rw+F2gBdgo4j7CVomP49sS7CbqsdybbiOGpB9DJ
# hs5QVMpYV73TVV3IwLiBHBECrTgUfZVOMF0KSEq2zk/LsfvehswavE3W4aBXJmGj
# gWSpcDz+6TqeTM8f1DIcgQPdz0IYgnT3yFTgiDbFGOFNt6eCidxdR6j9x+kpcN5R
# wApy4pRhE10YOV/xafBvKpRuWPjOPWRBlKdm53kS2aMh08spx7xSEqXn4QQldCnU
# WRz3Lki+TgBlpwYwJUbR77DAayNwAANE7taBrz2v+MnnogMrvvct0iwvfIA1W8kp
# 155Lo44SIfqGmrbJP6Mn+Udr3MR2oWozAgMBAAGjggGOMIIBijAfBgNVHSMEGDAW
# gBRfWO1MMXqiYUKNUoC6s2GXGaIymzAdBgNVHQ4EFgQUiGGMoSo3ZIEoYKGbMdCM
# /SwCzk8wDgYDVR0PAQH/BAQDAgbAMAwGA1UdEwEB/wQCMAAwFgYDVR0lAQH/BAww
# CgYIKwYBBQUHAwgwSgYDVR0gBEMwQTA1BgwrBgEEAbIxAQIBAwgwJTAjBggrBgEF
# BQcCARYXaHR0cHM6Ly9zZWN0aWdvLmNvbS9DUFMwCAYGZ4EMAQQCMEoGA1UdHwRD
# MEEwP6A9oDuGOWh0dHA6Ly9jcmwuc2VjdGlnby5jb20vU2VjdGlnb1B1YmxpY1Rp
# bWVTdGFtcGluZ0NBUjM2LmNybDB6BggrBgEFBQcBAQRuMGwwRQYIKwYBBQUHMAKG
# OWh0dHA6Ly9jcnQuc2VjdGlnby5jb20vU2VjdGlnb1B1YmxpY1RpbWVTdGFtcGlu
# Z0NBUjM2LmNydDAjBggrBgEFBQcwAYYXaHR0cDovL29jc3Auc2VjdGlnby5jb20w
# DQYJKoZIhvcNAQEMBQADggGBAAKBPqSGclEh+WWpLj1SiuHlm8xLE0SThI2yLuq+
# 75s11y6SceBchpnKpxWaGtXc8dya1Aq3RuW//y3wMThsvT4fSba2AoSWlR67rA4f
# TYGMIhgzocsids0ct/pHaocLVJSwnTYxY2pE0hPoZAvRebctbsTqENmZHyOVjOFl
# wN2R3DRweFeNs4uyZN5LRJ5EnVYlcTOq3bl1tI5poru9WaQRWQ4eynXp7Pj0Fz4D
# Kr86HYECRJMWiDjeV0QqAcQMFsIjJtrYTw7mU81qf4FBc4u4swphLeKRNyn9DDrd
# 3HIMJ+CpdhSHEGleeZ5I79YDg3B3A/fmVY2GaMik1Vm+FajEMv4/EN2mmHf4zkOu
# hYZNzVm4NrWJeY4UAriLBOeVYODdA1GxFr1ycbcUEGlUecc4RCPgYySs4d00NNui
# cR4a9n7idJlevAJbha/arIYMEuUqTeRRbWkhJwMKmb9yEvppRudKyu1t6l21sIuI
# ZqcpVH8oLWCxHS0LpDRF9Y4jijCCBoIwggRqoAMCAQICEDbCsL18Gzrno7PdNsvJ
# dWgwDQYJKoZIhvcNAQEMBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcg
# SmVyc2V5MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJU
# UlVTVCBOZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgUlNBIENlcnRpZmljYXRp
# b24gQXV0aG9yaXR5MB4XDTIxMDMyMjAwMDAwMFoXDTM4MDExODIzNTk1OVowVzEL
# MAkGA1UEBhMCR0IxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDEuMCwGA1UEAxMl
# U2VjdGlnbyBQdWJsaWMgVGltZSBTdGFtcGluZyBSb290IFI0NjCCAiIwDQYJKoZI
# hvcNAQEBBQADggIPADCCAgoCggIBAIid2LlFZ50d3ei5JoGaVFTAfEkFm8xaFQ/Z
# lBBEtEFAgXcUmanU5HYsyAhTXiDQkiUvpVdYqZ1uYoZEMgtHES1l1Cc6HaqZzEbO
# Op6YiTx63ywTon434aXVydmhx7Dx4IBrAou7hNGsKioIBPy5GMN7KmgYmuu4f92s
# KKjbxqohUSfjk1mJlAjthgF7Hjx4vvyVDQGsd5KarLW5d73E3ThobSkob2SL48Lp
# UR/O627pDchxll+bTSv1gASn/hp6IuHJorEu6EopoB1CNFp/+HpTXeNARXUmdRMK
# bnXWflq+/g36NJXB35ZvxQw6zid61qmrlD/IbKJA6COw/8lFSPQwBP1ityZdwuCy
# sCKZ9ZjczMqbUcLFyq6KdOpuzVDR3ZUwxDKL1wCAxgL2Mpz7eZbrb/JWXiOcNzDp
# QsmwGQ6Stw8tTCqPumhLRPb7YkzM8/6NnWH3T9ClmcGSF22LEyJYNWCHrQqYubNe
# KolzqUbCqhSqmr/UdUeb49zYHr7ALL8bAJyPDmubNqMtuaobKASBqP84uhqcRY/p
# jnYd+V5/dcu9ieERjiRKKsxCG1t6tG9oj7liwPddXEcYGOUiWLm742st50jGwTzx
# bMpepmOP1mLnJskvZaN5e45NuzAHteORlsSuDt5t4BBRCJL+5EZnnw0ezntk9R8Q
# JyAkL6/bAgMBAAGjggEWMIIBEjAfBgNVHSMEGDAWgBRTeb9aqitKz1SA4dibwJ3y
# sgNmyzAdBgNVHQ4EFgQU9ndq3T/9ARP/FqFsggIv0Ao9FCUwDgYDVR0PAQH/BAQD
# AgGGMA8GA1UdEwEB/wQFMAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwgwEQYDVR0g
# BAowCDAGBgRVHSAAMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRy
# dXN0LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDA1
# BggrBgEFBQcBAQQpMCcwJQYIKwYBBQUHMAGGGWh0dHA6Ly9vY3NwLnVzZXJ0cnVz
# dC5jb20wDQYJKoZIhvcNAQEMBQADggIBAA6+ZUHtaES45aHF1BGH5Lc7JYzrftrI
# F5Ht2PFDxKKFOct/awAEWgHQMVHol9ZLSyd/pYMbaC0IZ+XBW9xhdkkmUV/KbUOi
# L7g98M/yzRyqUOZ1/IY7Ay0YbMniIibJrPcgFp73WDnRDKtVutShPSZQZAdtFwXn
# uiWl8eFARK3PmLqEm9UsVX+55DbVIz33Mbhba0HUTEYv3yJ1fwKGxPBsP/MgTECi
# mh7eXomvMm0/GPxX2uhwCcs/YLxDnBdVVlxvDjHjO1cuwbOpkiJGHmLXXVNbsdXU
# C2xBrq9fLrfe8IBsA4hopwsCj8hTuwKXJlSTrZcPRVSccP5i9U28gZ7OMzoJGlxZ
# 5384OKm0r568Mo9TYrqzKeKZgFo0fj2/0iHbj55hc20jfxvK3mQi+H7xpbzxZOFG
# m/yVQkpo+ffv5gdhp+hv1GDsvJOtJinJmgGbBFZIThbqI+MHvAmMmkfb3fTxmSko
# p2mSJL1Y2x/955S29Gu0gSJIkc3z30vU/iXrMpWx2tS7UVfVP+5tKuzGtgkP7d/d
# oqDrLF1u6Ci3TpjAZdeLLlRQZm867eVeXED58LXd1Dk6UvaAhvmWYXoiLz4JA5gP
# Bcz7J311uahxCweNxE+xxxR3kT0WKzASo5G/PyDez6NHdIUKBeE3jDPs2ACc6CkJ
# 1Sji4PKWVT0/MYIGQTCCBj0CAQEwaDBUMQswCQYDVQQGEwJHQjEYMBYGA1UEChMP
# U2VjdGlnbyBMaW1pdGVkMSswKQYDVQQDEyJTZWN0aWdvIFB1YmxpYyBDb2RlIFNp
# Z25pbmcgQ0EgUjM2AhAGQz/MzOQzqJLMF7dGpYxlMA0GCWCGSAFlAwQCAQUAoIGE
# MBgGCisGAQQBgjcCAQwxCjAIoAKAAKECgAAwGQYJKoZIhvcNAQkDMQwGCisGAQQB
# gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcCARUwLwYJKoZIhvcNAQkE
# MSIEIJSUcVuiG37+bixm2NWe2/CG+/54vFR2Fi6bIRYujjbzMA0GCSqGSIb3DQEB
# AQUABIICALJoMBoHPyibbpN/qwDaIBO1uaSRRT81rglZF4zuZv9JcaGPVLerPb38
# 4jf2oGgdnIFSR+7KvbgwWFLhQO+4/pQTkgrN4DD/9Ignj71l3i/Vy1RVRX6Yi9Gq
# EVv0WIrAFcDWbLhqrgB5evYVf3IL2QPKX49RwXEHNslVlMWnGcItB8xDtmJdgYnD
# Qx8FiTPaeXJRSFVjy+hc2DBG0x1W5hcI4F2nl/vMv/LQFRmyj0q9JUJ6qaEbgp+1
# QFDVY+WmYj38gnZL0xhOqS50xN1r5ZQaf0sd6WlNTEV2elcu0NqIhNp7LuNcbCN5
# 0QXEmY9gzWM9iue/WLkl8K89eNGINogw7LfGoZynCsdIL7UEq9lnBJ5jVRVxHGRQ
# nYQ0CrewmEaR+sIW33kDtktRdp+vg6UAj1WmY5s8qa2YsmgWD6E3WAhzjQg1jgz4
# Kr5z5alnzSkAqBATNWA21LITqYV8J2Cg/S8fuPiKeab585HJUXQ1e4jjiYKBjZEL
# uA5Ig/t5yo/wBLj+AqkzGGKT57sV/cBD4JxNggL//MQhwS0qXv1n6FvvYyX1qWG7
# f/hUd0DIxmwjkVNX5I3tRwMxN+BMmXcx8V7fclQ4JKslv0IG3dFNoPQHfaAKRpaI
# zwV3gQwwbt2DYz3AM4bdIpSAG6ftojfHADDsgS0xYEmiHeZJiAUroYIDIzCCAx8G
# CSqGSIb3DQEJBjGCAxAwggMMAgEBMGowVTELMAkGA1UEBhMCR0IxGDAWBgNVBAoT
# D1NlY3RpZ28gTGltaXRlZDEsMCoGA1UEAxMjU2VjdGlnbyBQdWJsaWMgVGltZSBT
# dGFtcGluZyBDQSBSMzYCEQCkKTtuHt3XpzQIh616TrckMA0GCWCGSAFlAwQCAgUA
# oHkwGAYJKoZIhvcNAQkDMQsGCSqGSIb3DQEHATAcBgkqhkiG9w0BCQUxDxcNMjUx
# MDA0MjA1MjQyWjA/BgkqhkiG9w0BCQQxMgQwJC2wMeHLO9T0zuffiHyC8PTeuWCQ
# gW3uegypPM10l5uxavN+o9ezjX6stbCnX/BJMA0GCSqGSIb3DQEBAQUABIICAJhJ
# uv1+P5W/2clO9fuqa/VhwbSeFqwXlFpZipZCqTd44TnZAu6PF4mwRSUM/qjMuLd5
# 1kOhxKJtzO0v9j4XFailGj+FrbYXlNH4fd26PHgWoOzLchM0U4hevMKvjcV729Zu
# wsG9szbDs4tMTB0PB+b2eoEYOK4D7Mm+pq4+AVmuBa1mLI1BR6nezaRtFluTuJmt
# nuT20lqO+qbiBZO3dqvn104puuFbDf2cgPYQWj79+IeoWMZFo4i09OWbM+3pdLCj
# HRxmvxOnvbDOEtJ0bloInn0YtFNubvypAs/gbPOMJoSxMmfMtucFxi76FveWOmzg
# c12Mazlnq958uKICxrXAUSbNRL8EhCIqBuYYPbh+AcUn+zzZKrXGeMKVaiwBZXR0
# Ba/QQTzhocmbahfih4fqR577bi+5SRnSiLFZbj4DLKmu/XlZ91cyrHgP9fGYbiFY
# hzX4HMonbqh8F/08GS2hV2U2xOnHNsY2btnA8MwxHnngfp11xNWmco+5EuyB5onu
# Od21K3iwM64NmdWKMUNkdG4qvK+qnSTb0Q1IwNmxVsRM/smxiOxEkPiELormcGpc
# +2wMNanmKyc2vEdYlOylZr2b0a87IaQLNtPCcPbHtCsAHC+4yd/pp+ot6zVUrfc9
# zz6OYieqlbGzqFBBjq1bjAZWqa2UxT2shRh0wGmp
# SIG # End signature block
