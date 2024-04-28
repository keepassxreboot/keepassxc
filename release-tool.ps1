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

function Invoke-SignFiles([string[]] $files, [X509Certificate] $cert, [string] $time) {
    if ($files.Length -eq 0) {
        return
    }

    Write-Host "Signing files using $($cert.Subject) ($($cert.Thumbprint))" -ForegroundColor Cyan
    
    foreach ($_ in $files) {
        $sig = Get-AuthenticodeSignature -FilePath "$_" -ErrorAction SilentlyContinue
        if ($sig.Status -ne "Valid") {
            Write-Host "Signing file '$_'"
            $tmp = Set-AuthenticodeSignature -Certificate $cert -FilePath "$_" -TimestampServer "$Timestamp" -HashAlgorithm "SHA256"
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

    if ($SignBuild && !$SignCert) {
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

        # Sign MSI files using AppSign key
        $files = Get-ChildItem $OutDir -Include "*.msi" -Name
        Invoke-SignFiles $files $SignCert $Timestamp

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

    $AppSignFiles = $ResolvedFiles.Where({ $_ -match "\.(msi|exe|dll)$" })
    Invoke-SignFiles $AppSignFiles $SignCert $Timestamp

    $GpgSignFiles = $ResolvedFiles.Where({ $_ -match "\.(msi|zip|gz|xz|dmg|appimage)$" })
    Invoke-GpgSignFiles $GpgSignFiles $GpgKey
}

# SIG # Begin signature block
# MIIm2gYJKoZIhvcNAQcCoIImyzCCJscCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCDuejql+mhHrYzE
# MGUrjGMbUzkTkzwhj8dkNuT2x9j8+KCCH8cwggVvMIIEV6ADAgECAhBI/JO0YFWU
# jTanyYqJ1pQWMA0GCSqGSIb3DQEBDAUAMHsxCzAJBgNVBAYTAkdCMRswGQYDVQQI
# DBJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcMB1NhbGZvcmQxGjAYBgNVBAoM
# EUNvbW9kbyBDQSBMaW1pdGVkMSEwHwYDVQQDDBhBQUEgQ2VydGlmaWNhdGUgU2Vy
# dmljZXMwHhcNMjEwNTI1MDAwMDAwWhcNMjgxMjMxMjM1OTU5WjBWMQswCQYDVQQG
# EwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMS0wKwYDVQQDEyRTZWN0aWdv
# IFB1YmxpYyBDb2RlIFNpZ25pbmcgUm9vdCBSNDYwggIiMA0GCSqGSIb3DQEBAQUA
# A4ICDwAwggIKAoICAQCN55QSIgQkdC7/FiMCkoq2rjaFrEfUI5ErPtx94jGgUW+s
# hJHjUoq14pbe0IdjJImK/+8Skzt9u7aKvb0Ffyeba2XTpQxpsbxJOZrxbW6q5KCD
# J9qaDStQ6Utbs7hkNqR+Sj2pcaths3OzPAsM79szV+W+NDfjlxtd/R8SPYIDdub7
# P2bSlDFp+m2zNKzBenjcklDyZMeqLQSrw2rq4C+np9xu1+j/2iGrQL+57g2extme
# me/G3h+pDHazJyCh1rr9gOcB0u/rgimVcI3/uxXP/tEPNqIuTzKQdEZrRzUTdwUz
# T2MuuC3hv2WnBGsY2HH6zAjybYmZELGt2z4s5KoYsMYHAXVn3m3pY2MeNn9pib6q
# RT5uWl+PoVvLnTCGMOgDs0DGDQ84zWeoU4j6uDBl+m/H5x2xg3RpPqzEaDux5mcz
# mrYI4IAFSEDu9oJkRqj1c7AGlfJsZZ+/VVscnFcax3hGfHCqlBuCF6yH6bbJDoEc
# QNYWFyn8XJwYK+pF9e+91WdPKF4F7pBMeufG9ND8+s0+MkYTIDaKBOq3qgdGnA2T
# OglmmVhcKaO5DKYwODzQRjY1fJy67sPV+Qp2+n4FG0DKkjXp1XrRtX8ArqmQqsV/
# AZwQsRb8zG4Y3G9i/qZQp7h7uJ0VP/4gDHXIIloTlRmQAOka1cKG8eOO7F/05QID
# AQABo4IBEjCCAQ4wHwYDVR0jBBgwFoAUoBEKIz6W8Qfs4q8p74Klf9AwpLQwHQYD
# VR0OBBYEFDLrkpr/NZZILyhAQnAgNpFcF4XmMA4GA1UdDwEB/wQEAwIBhjAPBgNV
# HRMBAf8EBTADAQH/MBMGA1UdJQQMMAoGCCsGAQUFBwMDMBsGA1UdIAQUMBIwBgYE
# VR0gADAIBgZngQwBBAEwQwYDVR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5jb21v
# ZG9jYS5jb20vQUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNAYIKwYBBQUHAQEE
# KDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZI
# hvcNAQEMBQADggEBABK/oe+LdJqYRLhpRrWrJAoMpIpnuDqBv0WKfVIHqI0fTiGF
# OaNrXi0ghr8QuK55O1PNtPvYRL4G2VxjZ9RAFodEhnIq1jIV9RKDwvnhXRFAZ/ZC
# J3LFI+ICOBpMIOLbAffNRk8monxmwFE2tokCVMf8WPtsAO7+mKYulaEMUykfb9gZ
# pk+e96wJ6l2CxouvgKe9gUhShDHaMuwV5KZMPWw5c9QLhTkg4IUaaOGnSDip0TYl
# d8GNGRbFiExmfS9jzpjoad+sPKhdnckcW67Y8y90z7h+9teDnRGWYpquRRPaf9xH
# +9/DUp/mBlXpnYzyOmJRvOwkDynUWICE5EV7WtgwggYaMIIEAqADAgECAhBiHW0M
# UgGeO5B5FSCJIRwKMA0GCSqGSIb3DQEBDAUAMFYxCzAJBgNVBAYTAkdCMRgwFgYD
# VQQKEw9TZWN0aWdvIExpbWl0ZWQxLTArBgNVBAMTJFNlY3RpZ28gUHVibGljIENv
# ZGUgU2lnbmluZyBSb290IFI0NjAeFw0yMTAzMjIwMDAwMDBaFw0zNjAzMjEyMzU5
# NTlaMFQxCzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxKzAp
# BgNVBAMTIlNlY3RpZ28gUHVibGljIENvZGUgU2lnbmluZyBDQSBSMzYwggGiMA0G
# CSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQCbK51T+jU/jmAGQ2rAz/V/9shTUxjI
# ztNsfvxYB5UXeWUzCxEeAEZGbEN4QMgCsJLZUKhWThj/yPqy0iSZhXkZ6Pg2A2NV
# DgFigOMYzB2OKhdqfWGVoYW3haT29PSTahYkwmMv0b/83nbeECbiMXhSOtbam+/3
# 6F09fy1tsB8je/RV0mIk8XL/tfCK6cPuYHE215wzrK0h1SWHTxPbPuYkRdkP05Zw
# mRmTnAO5/arnY83jeNzhP06ShdnRqtZlV59+8yv+KIhE5ILMqgOZYAENHNX9SJDm
# +qxp4VqpB3MV/h53yl41aHU5pledi9lCBbH9JeIkNFICiVHNkRmq4TpxtwfvjsUe
# dyz8rNyfQJy/aOs5b4s+ac7IH60B+Ja7TVM+EKv1WuTGwcLmoU3FpOFMbmPj8pz4
# 4MPZ1f9+YEQIQty/NQd/2yGgW+ufflcZ/ZE9o1M7a5Jnqf2i2/uMSWymR8r2oQBM
# dlyh2n5HirY4jKnFH/9gRvd+QOfdRrJZb1sCAwEAAaOCAWQwggFgMB8GA1UdIwQY
# MBaAFDLrkpr/NZZILyhAQnAgNpFcF4XmMB0GA1UdDgQWBBQPKssghyi47G9IritU
# pimqF6TNDDAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADATBgNV
# HSUEDDAKBggrBgEFBQcDAzAbBgNVHSAEFDASMAYGBFUdIAAwCAYGZ4EMAQQBMEsG
# A1UdHwREMEIwQKA+oDyGOmh0dHA6Ly9jcmwuc2VjdGlnby5jb20vU2VjdGlnb1B1
# YmxpY0NvZGVTaWduaW5nUm9vdFI0Ni5jcmwwewYIKwYBBQUHAQEEbzBtMEYGCCsG
# AQUFBzAChjpodHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29QdWJsaWNDb2Rl
# U2lnbmluZ1Jvb3RSNDYucDdjMCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5zZWN0
# aWdvLmNvbTANBgkqhkiG9w0BAQwFAAOCAgEABv+C4XdjNm57oRUgmxP/BP6YdURh
# w1aVcdGRP4Wh60BAscjW4HL9hcpkOTz5jUug2oeunbYAowbFC2AKK+cMcXIBD0Zd
# OaWTsyNyBBsMLHqafvIhrCymlaS98+QpoBCyKppP0OcxYEdU0hpsaqBBIZOtBajj
# cw5+w/KeFvPYfLF/ldYpmlG+vd0xqlqd099iChnyIMvY5HexjO2AmtsbpVn0OhNc
# WbWDRF/3sBp6fWXhz7DcML4iTAWS+MVXeNLj1lJziVKEoroGs9Mlizg0bUMbOalO
# hOfCipnx8CaLZeVme5yELg09Jlo8BMe80jO37PU8ejfkP9/uPak7VLwELKxAMcJs
# zkyeiaerlphwoKx1uHRzNyE6bxuSKcutisqmKL5OTunAvtONEoteSiabkPVSZ2z7
# 6mKnzAfZxCl/3dq3dUNw4rg3sTCggkHSRqTqlLMS7gjrhTqBmzu1L90Y1KWN/Y5J
# KdGvspbOrTfOXyXvmPL6E52z1NZJ6ctuMFBQZH3pwWvqURR8AgQdULUvrxjUYbHH
# j95Ejza63zdrEcxWLDX6xWls/GDnVNueKjWUH3fTv1Y8Wdho698YADR7TNx8X8z2
# Bev6SivBBOHY+uqiirZtg0y9ShQoPzmCcn63Syatatvx157YK9hlcPmVoa1oDE5/
# L9Uo2bC5a4CH2RwwggZJMIIEsaADAgECAhAGQz/MzOQzqJLMF7dGpYxlMA0GCSqG
# SIb3DQEBDAUAMFQxCzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0
# ZWQxKzApBgNVBAMTIlNlY3RpZ28gUHVibGljIENvZGUgU2lnbmluZyBDQSBSMzYw
# HhcNMjQwMjIzMDAwMDAwWhcNMjcwMjIyMjM1OTU5WjBgMQswCQYDVQQGEwJVUzER
# MA8GA1UECAwIVmlyZ2luaWExHjAcBgNVBAoMFURyb2lkTW9ua2V5IEFwcHMsIExM
# QzEeMBwGA1UEAwwVRHJvaWRNb25rZXkgQXBwcywgTExDMIICIjANBgkqhkiG9w0B
# AQEFAAOCAg8AMIICCgKCAgEAuJtEjRyetghx6Abi1cpMT88uT6nIcTe3AyUvdSkj
# CtUM8Gat0YJfqTxokb9dBzJa7j8YWOUU1Yc4EDXoYYtVRE+1UkdPAcXNMf2hNXGI
# 45iZVwhBPQZBU4QfKltzYqrjAZgDvxeYd68qImjzUfrCY3uZHwEIuCewmNMPpEgb
# djuSXDyBAKKBtaO2iqyaJpqcC39QnDKlXMicDPqqH5fI7wK7Lg9f4BwOsaO4P68I
# 3pOv7L/6E5GR9+hTj6txhxFz/yCbDxN1PUvDsGaXjMmVeP2M95fkwOFwut5yBESD
# IwAGEWUFsTJ32hSmE74+xG6rVqtueayV7U9cGURznSk9ZlTUqQOW9Z4K+pu29gTZ
# 9zVWlONIsQR7QXfGKZWF+Xik6rTujSRTTsK7QNMYzBI6b9v0nD2pEWuGZDXIO5o5
# N2HzXEFlwxCFY483yWSObHNBp9PFtiDueqv+8vrN+lsirZlDFCxI6hW+F8oYp3Xx
# HdSqxsMRTqbO6dUjH2Tyd0G5fbyT8Rid7DbP6p/apzIrdFOM0kdcKLmppYBp7BIn
# TdjbWJYhtuORIUZQbUOSM71vYCUHj7xkckiYYmkUf0XH8xx8jqgVWseBW63gCEow
# hCEYxaWt0QGyXJ6UrlV4WTUCWzxm45I5OQoofymUvdutKgr9bR3nJ5yS/c+E3Knq
# JhkCAwEAAaOCAYkwggGFMB8GA1UdIwQYMBaAFA8qyyCHKLjsb0iuK1SmKaoXpM0M
# MB0GA1UdDgQWBBQta729krTac3CUndU0S0DdDscjHTAOBgNVHQ8BAf8EBAMCB4Aw
# DAYDVR0TAQH/BAIwADATBgNVHSUEDDAKBggrBgEFBQcDAzBKBgNVHSAEQzBBMDUG
# DCsGAQQBsjEBAgEDAjAlMCMGCCsGAQUFBwIBFhdodHRwczovL3NlY3RpZ28uY29t
# L0NQUzAIBgZngQwBBAEwSQYDVR0fBEIwQDA+oDygOoY4aHR0cDovL2NybC5zZWN0
# aWdvLmNvbS9TZWN0aWdvUHVibGljQ29kZVNpZ25pbmdDQVIzNi5jcmwweQYIKwYB
# BQUHAQEEbTBrMEQGCCsGAQUFBzAChjhodHRwOi8vY3J0LnNlY3RpZ28uY29tL1Nl
# Y3RpZ29QdWJsaWNDb2RlU2lnbmluZ0NBUjM2LmNydDAjBggrBgEFBQcwAYYXaHR0
# cDovL29jc3Auc2VjdGlnby5jb20wDQYJKoZIhvcNAQEMBQADggGBAJSy5YPCbh9Z
# suDCKgDuzOWZzNza4/FrA+kT7EitDezYN3S/P0EVc0tPbgYAKfNqY+ihAMyjZHdg
# ybfBWhGzUTDo+HEipcnZ2pgwPadsw23jJ8MN1tdms9iKDakIQ2MVsB7cGFRU8QjL
# ovkPdZkyLcjuYbkiZRoNoKlhmrOOf6n1oCwXVJ9ONJijc+Lr3+4EIqZ39ET2+uI9
# Wg9Bfd9XrDZfYFEcRJjNzRpCtHb26aIzV/XiMWasHRPaII34SzD0BmaPbsLeGW1U
# GvW3tQcgVNdT/uajegmShVb+c5J5ktRSJ0cqyxmTAYaeMuA6IxG1f6kui1SAFQs2
# lzlGyEgxgiNGo7cHHN2KidhrBL3U2bGr9Tkdp3gmV+Gj3esCdQzJE4aqmUZvIvHp
# krair4qbLFZRNozAZJn2SIeQa5u2U0ZmvcAr1C7S3JVLP3t9LKE0mlFkV9pbIU97
# ND3iH3tO0Zb3SvCK/XjO1PZVb8EXsi67wbfMSWAwi2CETDonb7+gBjCCBuwwggTU
# oAMCAQICEDAPb6zdZph0fKlGNqd4LbkwDQYJKoZIhvcNAQEMBQAwgYgxCzAJBgNV
# BAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5MRQwEgYDVQQHEwtKZXJzZXkgQ2l0
# eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBOZXR3b3JrMS4wLAYDVQQDEyVVU0VS
# VHJ1c3QgUlNBIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTE5MDUwMjAwMDAw
# MFoXDTM4MDExODIzNTk1OVowfTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0
# ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UEChMPU2VjdGln
# byBMaW1pdGVkMSUwIwYDVQQDExxTZWN0aWdvIFJTQSBUaW1lIFN0YW1waW5nIENB
# MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAyBsBr9ksfoiZfQGYPyCQ
# vZyAIVSTuc+gPlPvs1rAdtYaBKXOR4O168TMSTTL80VlufmnZBYmCfvVMlJ5Lslj
# whObtoY/AQWSZm8hq9VxEHmH9EYqzcRaydvXXUlNclYP3MnjU5g6Kh78zlhJ07/z
# Obu5pCNCrNAVw3+eolzXOPEWsnDTo8Tfs8VyrC4Kd/wNlFK3/B+VcyQ9ASi8Dw1P
# s5EBjm6dJ3VV0Rc7NCF7lwGUr3+Az9ERCleEyX9W4L1GnIK+lJ2/tCCwYH64TfUN
# P9vQ6oWMilZx0S2UTMiMPNMUopy9Jv/TUyDHYGmbWApU9AXn/TGs+ciFF8e4KRmk
# KS9G493bkV+fPzY+DjBnK0a3Na+WvtpMYMyou58NFNQYxDCYdIIhz2JWtSFzEh79
# qsoIWId3pBXrGVX/0DlULSbuRRo6b83XhPDX8CjFT2SDAtT74t7xvAIo9G3aJ4oG
# 0paH3uhrDvBbfel2aZMgHEqXLHcZK5OVmJyXnuuOwXhWxkQl3wYSmgYtnwNe/YOi
# U2fKsfqNoWTJiJJZy6hGwMnypv99V9sSdvqKQSTUG/xypRSi1K1DHKRJi0E5FAMe
# KfobpSKupcNNgtCN2mu32/cYQFdz8HGj+0p9RTbB942C+rnJDVOAffq2OVgy728Y
# UInXT50zvRq1naHelUF6p4MCAwEAAaOCAVowggFWMB8GA1UdIwQYMBaAFFN5v1qq
# K0rPVIDh2JvAnfKyA2bLMB0GA1UdDgQWBBQaofhhGSAPw0F3RSiO0TVfBhIEVTAO
# BgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADATBgNVHSUEDDAKBggr
# BgEFBQcDCDARBgNVHSAECjAIMAYGBFUdIAAwUAYDVR0fBEkwRzBFoEOgQYY/aHR0
# cDovL2NybC51c2VydHJ1c3QuY29tL1VTRVJUcnVzdFJTQUNlcnRpZmljYXRpb25B
# dXRob3JpdHkuY3JsMHYGCCsGAQUFBwEBBGowaDA/BggrBgEFBQcwAoYzaHR0cDov
# L2NydC51c2VydHJ1c3QuY29tL1VTRVJUcnVzdFJTQUFkZFRydXN0Q0EuY3J0MCUG
# CCsGAQUFBzABhhlodHRwOi8vb2NzcC51c2VydHJ1c3QuY29tMA0GCSqGSIb3DQEB
# DAUAA4ICAQBtVIGlM10W4bVTgZF13wN6MgstJYQRsrDbKn0qBfW8Oyf0WqC5SVmQ
# KWxhy7VQ2+J9+Z8A70DDrdPi5Fb5WEHP8ULlEH3/sHQfj8ZcCfkzXuqgHCZYXPO0
# EQ/V1cPivNVYeL9IduFEZ22PsEMQD43k+ThivxMBxYWjTMXMslMwlaTW9JZWCLjN
# XH8Blr5yUmo7Qjd8Fng5k5OUm7Hcsm1BbWfNyW+QPX9FcsEbI9bCVYRm5LPFZgb2
# 89ZLXq2jK0KKIZL+qG9aJXBigXNjXqC72NzXStM9r4MGOBIdJIct5PwC1j53BLwE
# NrXnd8ucLo0jGLmjwkcd8F3WoXNXBWiap8k3ZR2+6rzYQoNDBaWLpgn/0aGUpk6q
# PQn1BWy30mRa2Coiwkud8TleTN5IPZs0lpoJX47997FSkc4/ifYcobWpdR9xv1tD
# XWU9UIFuq/DQ0/yysx+2mZYm9Dx5i1xkzM3uJ5rloMAMcofBbk1a0x7q8ETmMm8c
# 6xdOlMN4ZSA7D0GqH+mhQZ3+sbigZSo04N6o+TzmwTC7wKBjLPxcFgCo0MR/6hGd
# HgbGpm0yXbQ4CStJB6r97DDa8acvz7f9+tCjhNknnvsBZne5VhDhIG7GrrH5trrI
# NV0zdo7xfCAMKneutaIChrop7rRaALGMq+P5CslUXdS5anSevUiumDCCBvUwggTd
# oAMCAQICEDlMJeF8oG0nqGXiO9kdItQwDQYJKoZIhvcNAQEMBQAwfTELMAkGA1UE
# BhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2Fs
# Zm9yZDEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMSUwIwYDVQQDExxTZWN0aWdv
# IFJTQSBUaW1lIFN0YW1waW5nIENBMB4XDTIzMDUwMzAwMDAwMFoXDTM0MDgwMjIz
# NTk1OVowajELMAkGA1UEBhMCR0IxEzARBgNVBAgTCk1hbmNoZXN0ZXIxGDAWBgNV
# BAoTD1NlY3RpZ28gTGltaXRlZDEsMCoGA1UEAwwjU2VjdGlnbyBSU0EgVGltZSBT
# dGFtcGluZyBTaWduZXIgIzQwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC
# AQCkkyhSS88nh3akKRyZOMDnDtTRHOxoywFk5IrNd7BxZYK8n/yLu7uVmPslEY5a
# iAlmERRYsroiW+b2MvFdLcB6og7g4FZk7aHlgSByIGRBbMfDCPrzfV3vIZrCftcs
# w7oRmB780yAIQrNfv3+IWDKrMLPYjHqWShkTXKz856vpHBYusLA4lUrPhVCrZwMl
# obs46Q9vqVqakSgTNbkf8z3hJMhrsZnoDe+7TeU9jFQDkdD8Lc9VMzh6CRwH0SLg
# Y4anvv3Sg3MSFJuaTAlGvTS84UtQe3LgW/0Zux88ahl7brstRCq+PEzMrIoEk8ZX
# hqBzNiuBl/obm36Ih9hSeYn+bnc317tQn/oYJU8T8l58qbEgWimro0KHd+D0TAJI
# 3VilU6ajoO0ZlmUVKcXtMzAl5paDgZr2YGaQWAeAzUJ1rPu0kdDF3QFAaraoEO72
# jXq3nnWv06VLGKEMn1ewXiVHkXTNdRLRnG/kXg2b7HUm7v7T9ZIvUoXo2kRRKqLM
# AMqHZkOjGwDvorWWnWKtJwvyG0rJw5RCN4gghKiHrsO6I3J7+FTv+GsnsIX1p0OF
# 2Cs5dNtadwLRpPr1zZw9zB+uUdB7bNgdLRFCU3F0wuU1qi1SEtklz/DT0JFDEtcy
# fZhs43dByP8fJFTvbq3GPlV78VyHOmTxYEsFT++5L+wJEwIDAQABo4IBgjCCAX4w
# HwYDVR0jBBgwFoAUGqH4YRkgD8NBd0UojtE1XwYSBFUwHQYDVR0OBBYEFAMPMciR
# KpO9Y/PRXU2kNA/SlQEYMA4GA1UdDwEB/wQEAwIGwDAMBgNVHRMBAf8EAjAAMBYG
# A1UdJQEB/wQMMAoGCCsGAQUFBwMIMEoGA1UdIARDMEEwNQYMKwYBBAGyMQECAQMI
# MCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2VjdGlnby5jb20vQ1BTMAgGBmeBDAEE
# AjBEBgNVHR8EPTA7MDmgN6A1hjNodHRwOi8vY3JsLnNlY3RpZ28uY29tL1NlY3Rp
# Z29SU0FUaW1lU3RhbXBpbmdDQS5jcmwwdAYIKwYBBQUHAQEEaDBmMD8GCCsGAQUF
# BzAChjNodHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29SU0FUaW1lU3RhbXBp
# bmdDQS5jcnQwIwYIKwYBBQUHMAGGF2h0dHA6Ly9vY3NwLnNlY3RpZ28uY29tMA0G
# CSqGSIb3DQEBDAUAA4ICAQBMm2VY+uB5z+8VwzJt3jOR63dY4uu9y0o8dd5+lG3D
# IscEld9laWETDPYMnvWJIF7Bh8cDJMrHpfAm3/j4MWUN4OttUVemjIRSCEYcKsLe
# 8tqKRfO+9/YuxH7t+O1ov3pWSOlh5Zo5d7y+upFkiHX/XYUWNCfSKcv/7S3a/76T
# DOxtog3Mw/FuvSGRGiMAUq2X1GJ4KoR5qNc9rCGPcMMkeTqX8Q2jo1tT2KsAulj7
# NYBPXyhxbBlewoNykK7gxtjymfvqtJJlfAd8NUQdrVgYa2L73mzECqls0yFGcNwv
# jXVMI8JB0HqWO8NL3c2SJnR2XDegmiSeTl9O048P5RNPWURlS0Nkz0j4Z2e5Tb/M
# DbE6MNChPUitemXk7N/gAfCzKko5rMGk+al9NdAyQKCxGSoYIbLIfQVxGksnNqrg
# mByDdefHfkuEQ81D+5CXdioSrEDBcFuZCkD6gG2UYXvIbrnIZ2ckXFCNASDeB/cB
# 1PguEc2dg+X4yiUcRD0n5bCGRyoLG4R2fXtoT4239xO07aAt7nMP2RC6nZksfNd1
# H48QxJTmfiTllUqIjCfWhWYd+a5kdpHoSP7IVQrtKcMf3jimwBT7Mj34qYNiNsjD
# vgCHHKv6SkIciQPc9Vx8cNldeE7un14g5glqfCsIo0j1FfwET9/NIRx65fWOGtS5
# QDGCBmkwggZlAgEBMGgwVDELMAkGA1UEBhMCR0IxGDAWBgNVBAoTD1NlY3RpZ28g
# TGltaXRlZDErMCkGA1UEAxMiU2VjdGlnbyBQdWJsaWMgQ29kZSBTaWduaW5nIENB
# IFIzNgIQBkM/zMzkM6iSzBe3RqWMZTANBglghkgBZQMEAgEFAKCBhDAYBgorBgEE
# AYI3AgEMMQowCKACgAChAoAAMBkGCSqGSIb3DQEJAzEMBgorBgEEAYI3AgEEMBwG
# CisGAQQBgjcCAQsxDjAMBgorBgEEAYI3AgEVMC8GCSqGSIb3DQEJBDEiBCCn5BDd
# F+7Q6LMoJuJxenFHgWAZjm1CET9oBKnlZKClzjANBgkqhkiG9w0BAQEFAASCAgAS
# ypiTBQb39I43fGdH6t2OYAl53TSbJfPG99/11OYS+6nMTKhy7dHtzzgMFxBQmL/L
# P4eJJMqh1yIYEjrjhNLLddRhVP2lfsuQ1OkLVx5lS8M32I3SzpskOe+SywMLDYJy
# sYHEcZkyQX0Q2J/RGzF8/tDcltZodYEdZrQdaAKo7bGv1JcYpW7B6JZnNjquE90d
# WVNAsQ6Mc3kzkjbs2qDaRAdkOmX5uENWbNf1GgTRpud7Ic5hMyb4v9qfWAptlFuO
# pLHyuINNsBuTfzD/cGVR9qecDPIE90UnHQHZWws9U+m84CzAmqpptp4VhrAWc7Hc
# bHsbmg4tGA41ythKyERpW9YlwID6fJYMigEVmJihXdM/qRGO2XdfbPAr0C0AMPIV
# re8r86BJw1lxJJYL2gsS/ttgrnW2C8aFq+IxxXWnv/7maPG69K/jmRLQGZuLIZCl
# 7rT6hob37zZMsdnqDZ0DjJb/FGonJr7GpyeMEWPy8eVwZydMbC9hBl8HNgQ04sp7
# ouskM1nCco9DV+d1Y6Oyje6IylZjD+xgX7VfsDa2O3Lw27cfyxJBW359meYHytkJ
# oYqh4Y4fC9YlYTD3913ryqTbPaWtWjvFV+GR8biHxDoTmTRuNaeN6RDyyZJkdON7
# CnR/8X8y4C9BXdesvjfIdhHZsGwLJcZ87cnYGb7oYqGCA0swggNHBgkqhkiG9w0B
# CQYxggM4MIIDNAIBATCBkTB9MQswCQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRl
# ciBNYW5jaGVzdGVyMRAwDgYDVQQHEwdTYWxmb3JkMRgwFgYDVQQKEw9TZWN0aWdv
# IExpbWl0ZWQxJTAjBgNVBAMTHFNlY3RpZ28gUlNBIFRpbWUgU3RhbXBpbmcgQ0EC
# EDlMJeF8oG0nqGXiO9kdItQwDQYJYIZIAWUDBAICBQCgeTAYBgkqhkiG9w0BCQMx
# CwYJKoZIhvcNAQcBMBwGCSqGSIb3DQEJBTEPFw0yNDA0MjgyMjE0MzdaMD8GCSqG
# SIb3DQEJBDEyBDARWlO4ZY7Qij7x/efLB7SJrHgfXJwezYW7sskwZhfhnoQ8JQ8Q
# HfefvIk4nF1+1PkwDQYJKoZIhvcNAQEBBQAEggIAWpBgtEaYVRayRmCTjyOoKg0b
# 2vXn3dqpcpckspX4t58xHLbhapGm3Akg9N6C0xZWm9qQ9vhjoOeuLZ0Z+017JRUe
# YExYYIYcyNGlxyt/uXiBst8KiAFFzn6RwIjycQcsnOsGRBAz2E9/k7wGtdg8kqBI
# Q71cDl+seRjWVcTR4JgthphZuRTKS1Jxn3tjDNJuK+LFo4jL38ojxhhdOnb3xzZ0
# M1AQ+l2YuDxBX4H9aZsbiTfdI1mxvmPgmZbq4fjV28TUCiBhD1UYuHUPN3Ff9Fwo
# 9BMbTLvKqED8Mm9A25S4M8kVZsGt8j3EAt0AJaWbdHLpLC0l0ykDAcSiwZNYsdMu
# vN0q6z5knfhKv4M8FXQ2wu8pbPww7/4kBqqy9L8VMI8UIazG9Z/R7yhkZjEz3jgc
# a/VZMcsDn41B79/9eSx4wED7NYtc0T6DB8WFH1a2CqlORSHnRolnms+VjWerfmZP
# a9lV7Sk1gGZ+MePsWwXj7liURI/ubTtPtxWElWuYookkQMmrOJYj+IZCW4RvV3I3
# utzHUwfbBaON3Mq46ADayLxKP2SE9j3JXpl4mZeWXdYUrywt2TgQktCXT7iZOinl
# dbx1tso5uDAj1DQDiTm4Nps+UWjyo2bZB/g1ONMqxPDIuY75HryfmJDlvCMp80Tk
# cJchA5s/dVwNSWKti4Q=
# SIG # End signature block
