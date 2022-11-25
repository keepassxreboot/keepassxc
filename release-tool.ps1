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
    Invoke-Cmd "tx" "pull -af --minimum-perc=60 --parallel -r keepassxc.share-translations-keepassxc-en-ts--develop"

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
# MIIkvgYJKoZIhvcNAQcCoIIkrzCCJKsCAQExCzAJBgUrDgMCGgUAMGkGCisGAQQB
# gjcCAQSgWzBZMDQGCisGAQQBgjcCAR4wJgIDAQAABBAfzDtgWUsITrck0sYpfvNR
# AgEAAgEAAgEAAgEAAgEAMCEwCQYFKw4DAhoFAAQUccSicCrmJ6HTiKZr9ZV5mT6i
# 9sqggh6mMIIFOjCCBCKgAwIBAgIQWKLXLYzA/YnM/yHg1O3HSjANBgkqhkiG9w0B
# AQsFADB8MQswCQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVy
# MRAwDgYDVQQHEwdTYWxmb3JkMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxJDAi
# BgNVBAMTG1NlY3RpZ28gUlNBIENvZGUgU2lnbmluZyBDQTAeFw0yMTAzMTUwMDAw
# MDBaFw0yNDAzMTQyMzU5NTlaMIGhMQswCQYDVQQGEwJVUzEOMAwGA1UEEQwFMjIz
# MTUxETAPBgNVBAgMCFZpcmdpbmlhMRIwEAYDVQQHDAlGcmFuY29uaWExGzAZBgNV
# BAkMEjY2NTMgQXVkcmV5IEtheSBDdDEeMBwGA1UECgwVRHJvaWRNb25rZXkgQXBw
# cywgTExDMR4wHAYDVQQDDBVEcm9pZE1vbmtleSBBcHBzLCBMTEMwggEiMA0GCSqG
# SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCwB9L/+1zlcXOQLoYvdrYAWS9B5ui+7E9c
# XCn6wcB4NdmaRbNM3kdWc8nbjOOHeOct2jVzVu/pJR1SagI+V1R1BfzgfzuW55Yy
# iHrqXQGfL9xhqJAWSvdQRinvlkZ+WY3QxnOhzcQk+BTLYdUwq04O3jMv7vnH6fuL
# q/HXEsgDObZC7EyKEtVbWVo4nqY0tUTviJXvRI/sFDN8DvULefwZWIvF7G11NFeK
# It24+hDCzvVBKtEn7DNmFGO1CJAB7Sz4jFewV4MP1gviMAfGbSBqavyRDBOG7eda
# SVb1Zq482yoHNAs+mpIQK2SGvUKKAJK2wCDbzgpvu5sfzwStpc0hAgMBAAGjggGQ
# MIIBjDAfBgNVHSMEGDAWgBQO4TqoUzox1Yq+wbutZxoDha00DjAdBgNVHQ4EFgQU
# 7u2WZ7fqJiaM3u9SlzAwGBhoWH0wDgYDVR0PAQH/BAQDAgeAMAwGA1UdEwEB/wQC
# MAAwEwYDVR0lBAwwCgYIKwYBBQUHAwMwEQYJYIZIAYb4QgEBBAQDAgQQMEoGA1Ud
# IARDMEEwNQYMKwYBBAGyMQECAQMCMCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2Vj
# dGlnby5jb20vQ1BTMAgGBmeBDAEEATBDBgNVHR8EPDA6MDigNqA0hjJodHRwOi8v
# Y3JsLnNlY3RpZ28uY29tL1NlY3RpZ29SU0FDb2RlU2lnbmluZ0NBLmNybDBzBggr
# BgEFBQcBAQRnMGUwPgYIKwYBBQUHMAKGMmh0dHA6Ly9jcnQuc2VjdGlnby5jb20v
# U2VjdGlnb1JTQUNvZGVTaWduaW5nQ0EuY3J0MCMGCCsGAQUFBzABhhdodHRwOi8v
# b2NzcC5zZWN0aWdvLmNvbTANBgkqhkiG9w0BAQsFAAOCAQEAD2w/Tt5KyPbX2M+h
# WVwgqpKm42nk6aN2HvSp+KWlrB2t+ziL+1IRXwq7S0V7p2e1ZK8uXLzBjUDVGjBc
# ugh5hGG95MGVltxCJrr/bk1He62L7MwVxfH5b5MrE/vC/cHcSxEB1AZwZxYKjDPf
# R81biDVch++XeKmvUxfT4XGo7McJqT4K/TcLwijSb/AWsXR+r2BXEAqgsoG37kk/
# fbPKimpJ07hxd/RNYVpE33E93zCQ1Tjc1tP3DaLq8cpS6jGUY5NNOzRgp2mGcGHy
# lv6Q/xf45qNvHiqFVctdvY9of0QFjg5eYDr4rLDa+mks9f1Jd8aDWKcsfCBnlohT
# KIffbTCCBYEwggRpoAMCAQICEDlyRDr5IrdR19NsEN0xNZUwDQYJKoZIhvcNAQEM
# BQAwezELMAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFuY2hlc3RlcjEQ
# MA4GA1UEBwwHU2FsZm9yZDEaMBgGA1UECgwRQ29tb2RvIENBIExpbWl0ZWQxITAf
# BgNVBAMMGEFBQSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczAeFw0xOTAzMTIwMDAwMDBa
# Fw0yODEyMzEyMzU5NTlaMIGIMQswCQYDVQQGEwJVUzETMBEGA1UECBMKTmV3IEpl
# cnNleTEUMBIGA1UEBxMLSmVyc2V5IENpdHkxHjAcBgNVBAoTFVRoZSBVU0VSVFJV
# U1QgTmV0d29yazEuMCwGA1UEAxMlVVNFUlRydXN0IFJTQSBDZXJ0aWZpY2F0aW9u
# IEF1dGhvcml0eTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAIASZRc2
# DsPbCLPQrFcNdu3NJ9NMrVCDYeKqIE0JLWQJ3M6Jn8w9qez2z8Hc8dOx1ns3KBEr
# R9o5xrw6GbRfpr19naNjQrZ28qk7K5H44m/Q7BYgkAk+4uh0yRi0kdRiZNt/owbx
# iBhqkCI8vP4T8IcUe/bkH47U5FHGEWdGCFHLhhRUP7wz/n5snP8WnRi9UY41pqdm
# yHJn2yFmsdSbeAPAUDrozPDcvJ5M/q8FljUfV1q3/875PbcstvZU3cjnEjpNrkyK
# t1yatLcgPcp/IjSufjtoZgFE5wFORlObM2D3lL5TN5BzQ/Myw1Pv26r+dE5px2uM
# YJPexMcM3+EyrsyTO1F4lWeL7j1W/gzQaQ8bD/MlJmszbfduR/pzQ+V+DqVmsSl8
# MoRjVYnEDcGTVDAZE6zTfTen6106bDVc20HXEtqpSQvf2ICKCZNijrVmzyWIzYS4
# sT+kOQ/ZAp7rEkyVfPNrBaleFoPMuGfi6BOdzFuC00yz7Vv/3uVzrCM7LQC/NVV0
# CUnYSVgaf5I25lGSDvMmfRxNF7zJ7EMm0L9BX0CpRET0medXh55QH1dUqD79dGMv
# sVBlCeZYQi5DGky08CVHWfoEHpPUJkZKUIGy3r54t/xnFeHJV4QeD2PW6WK61l9V
# LupcxigIBCU5uA4rqfJMlxwHPw1S9e3vL4IPAgMBAAGjgfIwge8wHwYDVR0jBBgw
# FoAUoBEKIz6W8Qfs4q8p74Klf9AwpLQwHQYDVR0OBBYEFFN5v1qqK0rPVIDh2JvA
# nfKyA2bLMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MBEGA1UdIAQK
# MAgwBgYEVR0gADBDBgNVHR8EPDA6MDigNqA0hjJodHRwOi8vY3JsLmNvbW9kb2Nh
# LmNvbS9BQUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDA0BggrBgEFBQcBAQQoMCYw
# JAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmNvbW9kb2NhLmNvbTANBgkqhkiG9w0B
# AQwFAAOCAQEAGIdR3HQhPZyK4Ce3M9AuzOzw5steEd4ib5t1jp5y/uTW/qofnJYt
# 7wNKfq70jW9yPEM7wD/ruN9cqqnGrvL82O6je0P2hjZ8FODN9Pc//t64tIrwkZb+
# /UNkfv3M0gGhfX34GRnJQisTv1iLuqSiZgR2iJFODIkUzqJNyTKzuugUGrxx8Vvw
# QQuYAAoiAxDlDLH5zZI3Ge078eQ6tvlFEyZ1r7uq7z97dzvSxAKRPRkA0xdcOds/
# exgNRc2ThZYvXd9ZFk8/Ub3VRRg/7UqO6AZhdCMWtQ1QcydER38QXYkqa4UxFMTo
# qWpMgLxqeM+4f452cpkMnf7XkQgWoaNflTCCBfUwggPdoAMCAQICEB2iSDBvmyYY
# 0ILgln0z02owDQYJKoZIhvcNAQEMBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
# EwpOZXcgSmVyc2V5MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhl
# IFVTRVJUUlVTVCBOZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgUlNBIENlcnRp
# ZmljYXRpb24gQXV0aG9yaXR5MB4XDTE4MTEwMjAwMDAwMFoXDTMwMTIzMTIzNTk1
# OVowfDELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQ
# MA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMSQwIgYD
# VQQDExtTZWN0aWdvIFJTQSBDb2RlIFNpZ25pbmcgQ0EwggEiMA0GCSqGSIb3DQEB
# AQUAA4IBDwAwggEKAoIBAQCGIo0yhXoYn0nwli9jCB4t3HyfFM/jJrYlZilAhlRG
# dDFixRDtsocnppnLlTDAVvWkdcapDlBipVGREGrgS2Ku/fD4GKyn/+4uMyD6DBmJ
# qGx7rQDDYaHcaWVtH24nlteXUYam9CflfGqLlR5bYNV+1xaSnAAvaPeX7Wpyvjg7
# Y96Pv25MQV0SIAhZ6DnNj9LWzwa0VwW2TqE+V2sfmLzEYtYbC43HZhtKn52BxHJA
# teJf7wtF/6POF6YtVbC3sLxUap28jVZTxvC6eVBJLPcDuf4vZTXyIuosB69G2flG
# HNyMfHEo8/6nxhTdVZFuihEN3wYklX0Pp6F8OtqGNWHTAgMBAAGjggFkMIIBYDAf
# BgNVHSMEGDAWgBRTeb9aqitKz1SA4dibwJ3ysgNmyzAdBgNVHQ4EFgQUDuE6qFM6
# MdWKvsG7rWcaA4WtNA4wDgYDVR0PAQH/BAQDAgGGMBIGA1UdEwEB/wQIMAYBAf8C
# AQAwHQYDVR0lBBYwFAYIKwYBBQUHAwMGCCsGAQUFBwMIMBEGA1UdIAQKMAgwBgYE
# VR0gADBQBgNVHR8ESTBHMEWgQ6BBhj9odHRwOi8vY3JsLnVzZXJ0cnVzdC5jb20v
# VVNFUlRydXN0UlNBQ2VydGlmaWNhdGlvbkF1dGhvcml0eS5jcmwwdgYIKwYBBQUH
# AQEEajBoMD8GCCsGAQUFBzAChjNodHRwOi8vY3J0LnVzZXJ0cnVzdC5jb20vVVNF
# UlRydXN0UlNBQWRkVHJ1c3RDQS5jcnQwJQYIKwYBBQUHMAGGGWh0dHA6Ly9vY3Nw
# LnVzZXJ0cnVzdC5jb20wDQYJKoZIhvcNAQEMBQADggIBAE1jUO1HNEphpNveaiqM
# m/EAAB4dYns61zLC9rPgY7P7YQCImhttEAcET7646ol4IusPRuzzRl5ARokS9At3
# WpwqQTr81vTr5/cVlTPDoYMot94v5JT3hTODLUpASL+awk9KsY8k9LOBN9O3ZLCm
# I2pZaFJCX/8E6+F0ZXkI9amT3mtxQJmWunjxucjiwwgWsatjWsgVgG10Xkp1fqW4
# w2y1z99KeYdcx0BNYzX2MNPPtQoOCwR/oEuuu6Ol0IQAkz5TXTSlADVpbL6fICUQ
# DRn7UJBhvjmPeo5N9p8OHv4HURJmgyYZSJXOSsnBf/M6BZv5b9+If8AjntIeQ3pF
# McGcTanwWbJZGehqjSkEAnd8S0vNcL46slVaeD68u28DECV3FTSK+TbMQ5Lkuk/x
# YpMoJVcp+1EZx6ElQGqEV8aynbG8HArafGd+fS7pKEwYfsR7MUFxmksp7As9V1DS
# yt39ngVR5UR43QHesXWYDVQk/fBO4+L4g71yuss9Ou7wXheSaG3IYfmm8SoKC6W5
# 9J7umDIFhZ7r+YMp08Ysfb06dy6LN0KgaoLtO0qqlBCk4Q34F8W2WnkzGJLjtXX4
# oemOCiUe5B7xn1qHI/+fpFGe+zmAEc3btcSnqIBv5VPU4OOiwtJbGvoyJi1qV3Ac
# PKRYLqPzW0sH3DJZ84enGm1YMIIG7DCCBNSgAwIBAgIQMA9vrN1mmHR8qUY2p3gt
# uTANBgkqhkiG9w0BAQwFADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBK
# ZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRS
# VVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlv
# biBBdXRob3JpdHkwHhcNMTkwNTAyMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjB9MQsw
# CQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQH
# EwdTYWxmb3JkMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxJTAjBgNVBAMTHFNl
# Y3RpZ28gUlNBIFRpbWUgU3RhbXBpbmcgQ0EwggIiMA0GCSqGSIb3DQEBAQUAA4IC
# DwAwggIKAoICAQDIGwGv2Sx+iJl9AZg/IJC9nIAhVJO5z6A+U++zWsB21hoEpc5H
# g7XrxMxJNMvzRWW5+adkFiYJ+9UyUnkuyWPCE5u2hj8BBZJmbyGr1XEQeYf0RirN
# xFrJ29ddSU1yVg/cyeNTmDoqHvzOWEnTv/M5u7mkI0Ks0BXDf56iXNc48RaycNOj
# xN+zxXKsLgp3/A2UUrf8H5VzJD0BKLwPDU+zkQGObp0ndVXRFzs0IXuXAZSvf4DP
# 0REKV4TJf1bgvUacgr6Unb+0ILBgfrhN9Q0/29DqhYyKVnHRLZRMyIw80xSinL0m
# /9NTIMdgaZtYClT0Bef9Maz5yIUXx7gpGaQpL0bj3duRX58/Nj4OMGcrRrc1r5a+
# 2kxgzKi7nw0U1BjEMJh0giHPYla1IXMSHv2qyghYh3ekFesZVf/QOVQtJu5FGjpv
# zdeE8NfwKMVPZIMC1Pvi3vG8Aij0bdonigbSlofe6GsO8Ft96XZpkyAcSpcsdxkr
# k5WYnJee647BeFbGRCXfBhKaBi2fA179g6JTZ8qx+o2hZMmIklnLqEbAyfKm/31X
# 2xJ2+opBJNQb/HKlFKLUrUMcpEmLQTkUAx4p+hulIq6lw02C0I3aa7fb9xhAV3Pw
# caP7Sn1FNsH3jYL6uckNU4B9+rY5WDLvbxhQiddPnTO9GrWdod6VQXqngwIDAQAB
# o4IBWjCCAVYwHwYDVR0jBBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0O
# BBYEFBqh+GEZIA/DQXdFKI7RNV8GEgRVMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMB
# Af8ECDAGAQH/AgEAMBMGA1UdJQQMMAoGCCsGAQUFBwMIMBEGA1UdIAQKMAgwBgYE
# VR0gADBQBgNVHR8ESTBHMEWgQ6BBhj9odHRwOi8vY3JsLnVzZXJ0cnVzdC5jb20v
# VVNFUlRydXN0UlNBQ2VydGlmaWNhdGlvbkF1dGhvcml0eS5jcmwwdgYIKwYBBQUH
# AQEEajBoMD8GCCsGAQUFBzAChjNodHRwOi8vY3J0LnVzZXJ0cnVzdC5jb20vVVNF
# UlRydXN0UlNBQWRkVHJ1c3RDQS5jcnQwJQYIKwYBBQUHMAGGGWh0dHA6Ly9vY3Nw
# LnVzZXJ0cnVzdC5jb20wDQYJKoZIhvcNAQEMBQADggIBAG1UgaUzXRbhtVOBkXXf
# A3oyCy0lhBGysNsqfSoF9bw7J/RaoLlJWZApbGHLtVDb4n35nwDvQMOt0+LkVvlY
# Qc/xQuUQff+wdB+PxlwJ+TNe6qAcJlhc87QRD9XVw+K81Vh4v0h24URnbY+wQxAP
# jeT5OGK/EwHFhaNMxcyyUzCVpNb0llYIuM1cfwGWvnJSajtCN3wWeDmTk5Sbsdyy
# bUFtZ83Jb5A9f0VywRsj1sJVhGbks8VmBvbz1kteraMrQoohkv6ob1olcGKBc2Ne
# oLvY3NdK0z2vgwY4Eh0khy3k/ALWPncEvAQ2ted3y5wujSMYuaPCRx3wXdahc1cF
# aJqnyTdlHb7qvNhCg0MFpYumCf/RoZSmTqo9CfUFbLfSZFrYKiLCS53xOV5M3kg9
# mzSWmglfjv33sVKRzj+J9hyhtal1H3G/W0NdZT1QgW6r8NDT/LKzH7aZlib0PHmL
# XGTMze4nmuWgwAxyh8FuTVrTHurwROYybxzrF06Uw3hlIDsPQaof6aFBnf6xuKBl
# KjTg3qj5PObBMLvAoGMs/FwWAKjQxH/qEZ0eBsambTJdtDgJK0kHqv3sMNrxpy/P
# t/360KOE2See+wFmd7lWEOEgbsausfm2usg1XTN2jvF8IAwqd661ogKGuinutFoA
# sYyr4/kKyVRd1LlqdJ69SK6YMIIG9jCCBN6gAwIBAgIRAJA5f5rSSjoT8r2RXwg4
# qUMwDQYJKoZIhvcNAQEMBQAwfTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0
# ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UEChMPU2VjdGln
# byBMaW1pdGVkMSUwIwYDVQQDExxTZWN0aWdvIFJTQSBUaW1lIFN0YW1waW5nIENB
# MB4XDTIyMDUxMTAwMDAwMFoXDTMzMDgxMDIzNTk1OVowajELMAkGA1UEBhMCR0Ix
# EzARBgNVBAgTCk1hbmNoZXN0ZXIxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDEs
# MCoGA1UEAwwjU2VjdGlnbyBSU0EgVGltZSBTdGFtcGluZyBTaWduZXIgIzMwggIi
# MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCQsnE/eeHUuYoXzMOXwpCUcu1a
# Om8BQ39zWiifJHygNUAG+pSvCqGDthPkSxUGXmqKIDRxe7slrT9bCqQfL2x9LmFR
# 0IxZNz6mXfEeXYC22B9g480Saogfxv4Yy5NDVnrHzgPWAGQoViKxSxnS8JbJRB85
# XZywlu1aSY1+cuRDa3/JoD9sSq3VAE+9CriDxb2YLAd2AXBF3sPwQmnq/ybMA0Qf
# FijhanS2nEX6tjrOlNEfvYxlqv38wzzoDZw4ZtX8fR6bWYyRWkJXVVAWDUt0cu6g
# KjH8JgI0+WQbWf3jOtTouEEpdAE/DeATdysRPPs9zdDn4ZdbVfcqA23VzWLazpwe
# /OpwfeZ9S2jOWilh06BcJbOlJ2ijWP31LWvKX2THaygM2qx4Qd6S7w/F7KvfLW8a
# VFFsM7ONWWDn3+gXIqN5QWLP/Hvzktqu4DxPD1rMbt8fvCKvtzgQmjSnC//+HV6k
# 8+4WOCs/rHaUQZ1kHfqA/QDh/vg61MNeu2lNcpnl8TItUfphrU3qJo5t/KlImD7y
# Rg1psbdu9AXbQQXGGMBQ5Pit/qxjYUeRvEa1RlNsxfThhieThDlsdeAdDHpZiy7L
# 9GQsQkf0VFiFN+XHaafSJYuWv8at4L2xN/cf30J7qusc6es9Wt340pDVSZo6HYMa
# V38cAcLOHH3M+5YVxQIDAQABo4IBgjCCAX4wHwYDVR0jBBgwFoAUGqH4YRkgD8NB
# d0UojtE1XwYSBFUwHQYDVR0OBBYEFCUuaDxrmiskFKkfot8mOs8UpvHgMA4GA1Ud
# DwEB/wQEAwIGwDAMBgNVHRMBAf8EAjAAMBYGA1UdJQEB/wQMMAoGCCsGAQUFBwMI
# MEoGA1UdIARDMEEwNQYMKwYBBAGyMQECAQMIMCUwIwYIKwYBBQUHAgEWF2h0dHBz
# Oi8vc2VjdGlnby5jb20vQ1BTMAgGBmeBDAEEAjBEBgNVHR8EPTA7MDmgN6A1hjNo
# dHRwOi8vY3JsLnNlY3RpZ28uY29tL1NlY3RpZ29SU0FUaW1lU3RhbXBpbmdDQS5j
# cmwwdAYIKwYBBQUHAQEEaDBmMD8GCCsGAQUFBzAChjNodHRwOi8vY3J0LnNlY3Rp
# Z28uY29tL1NlY3RpZ29SU0FUaW1lU3RhbXBpbmdDQS5jcnQwIwYIKwYBBQUHMAGG
# F2h0dHA6Ly9vY3NwLnNlY3RpZ28uY29tMA0GCSqGSIb3DQEBDAUAA4ICAQBz2u1o
# csvCuUChMbu0A6MtFHsk57RbFX2o6f2t0ZINfD02oGnZ85ow2qxp1nRXJD9+DzzZ
# 9cN5JWwm6I1ok87xd4k5f6gEBdo0wxTqnwhUq//EfpZsK9OU67Rs4EVNLLL3Ozta
# tcH714l1bZhycvb3Byjz07LQ6xm+FSx4781FoADk+AR2u1fFkL53VJB0ngtPTcSq
# E4+XrwE1K8ubEXjp8vmJBDxO44ISYuu0RAx1QcIPNLiIncgi8RNq2xgvbnitxAW0
# 6IQIkwf5fYP+aJg05Hflsc6MlGzbA20oBUd+my7wZPvbpAMxEHwa+zwZgNELcLlV
# X0e+OWTOt9ojVDLjRrIy2NIphskVXYCVrwL7tNEunTh8NeAPHO0bR0icImpVgtny
# ughlA+XxKfNIigkBTKZ58qK2GpmU65co4b59G6F87VaApvQiM5DkhFP8KvrAp5eo
# 6rWNes7k4EuhM6sLdqDVaRa3jma/X/ofxKh/p6FIFJENgvy9TZntyeZsNv53Q5m4
# aS18YS/to7BJ/lu+aSSR/5P8V2mSS9kFP22GctOi0MBk0jpCwRoD+9DtmiG4P6+m
# slFU1UzFyh8SjVfGOe1c/+yfJnatZGZn6Kow4NKtt32xakEnbgOKo3TgigmCbr/j
# 9re8ngspGGiBoZw/bhZZSxQJCZrmrr9gFd2G9TGCBYIwggV+AgEBMIGQMHwxCzAJ
# BgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcT
# B1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDEkMCIGA1UEAxMbU2Vj
# dGlnbyBSU0EgQ29kZSBTaWduaW5nIENBAhBYotctjMD9icz/IeDU7cdKMAkGBSsO
# AwIaBQCgeDAYBgorBgEEAYI3AgEMMQowCKACgAChAoAAMBkGCSqGSIb3DQEJAzEM
# BgorBgEEAYI3AgEEMBwGCisGAQQBgjcCAQsxDjAMBgorBgEEAYI3AgEVMCMGCSqG
# SIb3DQEJBDEWBBQyqMslxaPRHhE8POQX8uLV4mnwLjANBgkqhkiG9w0BAQEFAASC
# AQBhQUgt7fRTbF1rGUv7z9sdfZzNQiLWg2LYMURLZAWcZRFBW6RoP6rTSbpquyRZ
# Bs4BlK7JkxCHBXrCeYl5qMx7b6N7twsgyz8OR+EPnYIkEoaKafeqO6B/Q0NhhdOW
# vd0wK2YsD5Sb7135a0trAQtS+fnhRr9y9LgMHePBq3iJAo8BWtcUYF5eBbLmJjZU
# yzu6aUlXgVakBm8fso0NqLNAVn0vQizJHqsnK610+zCVlzPQ/2HflRpwLgF4X1kQ
# Jewj42T2kjzn1Worzcsj3v7WJgbuqThnVR1NIhi+bhfpkrCr4iC/+4QIZZLkzUHl
# cS3DhzvxAQ62whQsUAB9z2HBoYIDTDCCA0gGCSqGSIb3DQEJBjGCAzkwggM1AgEB
# MIGSMH0xCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIx
# EDAOBgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDElMCMG
# A1UEAxMcU2VjdGlnbyBSU0EgVGltZSBTdGFtcGluZyBDQQIRAJA5f5rSSjoT8r2R
# Xwg4qUMwDQYJYIZIAWUDBAICBQCgeTAYBgkqhkiG9w0BCQMxCwYJKoZIhvcNAQcB
# MBwGCSqGSIb3DQEJBTEPFw0yMjEwMjMxMzE4MTZaMD8GCSqGSIb3DQEJBDEyBDAi
# pcegfL2b7n0V2o/qV4vNL3exKvlIIxuSCCqMkqibj2h04kPtwOkjhJ064uMHSwcw
# DQYJKoZIhvcNAQEBBQAEggIAWMkT++gXvrUNBmS62Sw9TekX6fEKJIOFwLHO5wzh
# AdBv/NavsMLB+PNrKizKLL02+Q9v+kyKaeFFlReWa50S+meM2L+wW5YRMGggBKRB
# Xhos4qL0ZffKPDbrjmCW0+HdRj408yyNCNB5aPSS6ZLjPpSa6mqVyySfnSdZnyaC
# zXYQ2Y4qD3JGSk1MbRvCYB+jCaMM4unyJAS4IA6nWQ97184KLm5U2ktn9ygeWLlG
# ujQ2plQ7HuHD+/rMSqesQT6OcwGtERYyfDs+hndpONjKBIulbJJDM1mN6uLQpkfZ
# f/TPTZQBDx6EA1oUZ3Evx4cReQFZJjnVlsAJBnKmu3mHheisdlxuFv1DZfu2OD/M
# nqY2DeCCgmeC212fosI8ZHUupaKRXVjfcVNElt34lK+3FMzYSKr6rCxiFEXbjq8u
# WTG45ZMmcLzs7l9Yaz0eTc642SyBa+5OoTTXs3t9G5z9lVbGonOhfGVbJM+l8JNc
# txM+CnQt/OOcjTMDKcjOwG9gcxHjYQhpK8PKiXmPmgpaGYn5vCL5fLvR+s+vTsm4
# DJjUTHY87VVXt2IwOu45n1+RBJynewLeaXkwo+79R+/Dn/xoqVVGLRRU6c3yCIiW
# qKmsdlIziAr/Fou7jzKcaPFhVJ/NMsI/2c8bkfi6Baoh+go3j39nA3/oDtb4vHWk
# Y2E=
# SIG # End signature block
