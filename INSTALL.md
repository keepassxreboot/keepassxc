Build and Install KeePassXC
=================

This document will guide you through the steps to build and install KeePassXC from source.
For more information, see also the [_Building KeePassXC_](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC) page on the wiki.

The [QuickStart Guide](https://keepassxc.org/docs/KeePassXC_GettingStarted.html) gets you started using KeePassXC on your Windows, macOS, or Linux computer using pre-compiled binaries from the [downloads page](https://keepassxc.org/download).

Build Dependencies
==================

The following tools must exist within your PATH:

* make
* cmake (>= 3.3.0)
* g++ (>= 4.7) or clang++ (>= 6.0)
* asciidoctor (>= 2.0)

The following libraries are required:

* Qt 5 (>= 5.9.5): qtbase5, qtbase5-private, libqt5svg5, qttools5, qt5-image-formats-plugins
* botan (>= 2.12)
* libargon2
* zlib
* minizip
* readline (for completion in cli)
* qtx11extras, libxi, and libxtst (for auto-type on X11)
* qrencode
* libusb-1.0, pcsc-lite (for Yubikey support on Linux)

Prepare the Building Environment
================================

* [Building Environment on Linux](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Linux)
* [Building Environment on Windows](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Windows)
* [Building Environment on MacOS](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-macOS)

Build Steps
===========
We recommend using the release tool to perform builds, please read up-to-date instructions [on our wiki](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC#building-using-the-release-tool).

To compile from source, open a **Terminal (Linux/MacOS)**, the **MSVC Tools Command Prompt (Windows)**, or **MSYS2-MinGW shell (Windows)**. For code development on Windows, you can use Visual Studio 2022, Visual Studio Code, or CLion.

1. Download the KeePassXC [source tarball](https://keepassxc.org/download#source) or check out the latest version from our [Git repository](https://github.com/keepassxreboot/keepassxc).

   To clone the project from Git, `cd` to a suitable location and run

   ```
   git clone https://github.com/keepassxreboot/keepassxc.git
   ```

   This will clone the entire contents of the repository and check out the current `develop` branch.

   To update the project from within the project's folder, you can run the following command:

   ```
   git pull
   ```

   For a stable build, it is recommended to check out the master branch.

   ```
   git checkout master
   ```

2. Navigate to the directory where you have downloaded KeePassXC and type these commands:

   ```
   mkdir build
   cd build
   cmake -DWITH_XC_ALL=ON ..
   make
   ```

Note: These steps place the compiled KeePassXC binary inside the `./build/src/` directory.

## MacOS Build Notes

If you installed Qt5 via Homebrew, you should be able to compile KeePassXC without any changes. If CMake fails to find your Qt installation, you can specify it manually by adding the following parameter:

`-DCMAKE_PREFIX_PATH=/usr/local/opt/qt/lib/cmake`

(or whatever your Qt installation path is)

When building with ASAN support on macOS, you need to use `export ASAN_OPTIONS=detect_leaks=0` before running the tests (LSAN is no supported on macOS).

## Windows Build Notes

For detailed build steps see the [Windows Build Instructions](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC#windows).

If you are using MSVC, you may have to specify your Vcpkg toolchain by adding the following CMake parameter: `-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake`

If you are using MSYS2, you have to add ```-G "MSYS Makefiles"``` at the beginning of the cmake command.

CMake Configuration Options
==========================

## Common Parameters

```
-DCMAKE_INSTALL_PREFIX=/usr/local
-DCMAKE_VERBOSE_MAKEFILE=ON
-DCMAKE_BUILD_TYPE=<RelWithDebInfo/Debug/Release>
-DWITH_GUI_TESTS=ON
```

## KeePassXC Parameters

KeePassXC comes with a variety of build options that can turn on/off features. Most notably, we allow you to build the application with all TCP/IP networking code disabled. Please note that we still require and link against Qt5's network library in order to use local named pipes on all operating systems. Each of these build options are supplied at the time of calling cmake:

```
-DWITH_XC_AUTOTYPE=[ON|OFF] Enable/Disable Auto-Type (default: ON)
-DWITH_XC_YUBIKEY=[ON|OFF] Enable/Disable YubiKey HMAC-SHA1 authentication support (default: OFF)
-DWITH_XC_BROWSER=[ON|OFF] Enable/Disable KeePassXC-Browser extension support (default: OFF)
-DWITH_XC_NETWORKING=[ON|OFF] Enable/Disable Networking support (e.g., favicon downloading) (default: OFF)
-DWITH_XC_SSHAGENT=[ON|OFF] Enable/Disable SSHAgent support (default: OFF)
-DWITH_XC_FDOSECRETS=[ON|OFF] (Linux Only) Enable/Disable Freedesktop.org Secrets Service support (default:OFF)
-DWITH_XC_KEESHARE=[ON|OFF] Enable/Disable KeeShare group synchronization extension (default: OFF)
-DWITH_XC_ALL=[ON|OFF] Enable/Disable compiling all plugins above (default: OFF)

-DWITH_XC_UPDATECHECK=[ON|OFF] Enable/Disable automatic updating checking (requires WITH_XC_NETWORKING) (default: ON)

-DWITH_TESTS=[ON|OFF] Enable/Disable building of unit tests (default: ON)
-DWITH_GUI_TESTS=[ON|OFF] Enable/Disable building of GUI tests (default: OFF)
-DWITH_DEV_BUILD=[ON|OFF] Enable/Disable deprecated method warnings (default: OFF)
-DWITH_ASAN=[ON|OFF] Enable/Disable address sanitizer checks (Linux / macOS only) (default: OFF)
-DWITH_COVERAGE=[ON|OFF] Enable/Disable coverage tests (GCC only) (default: OFF)
-DWITH_APP_BUNDLE=[ON|OFF] Enable Application Bundle for macOS (default: ON)

-DKEEPASSXC_BUILD_TYPE=[Snapshot|PreRelease|Release] Set the build type to show/hide stability warnings (default: "Snapshot")
-DKEEPASSXC_DIST_TYPE=[Snap|AppImage|Other] Specify the distribution method (default: "Other")
-DOVERRIDE_VERSION=[X.X.X] Specify a version number when building. Used with snapshot builds (default: "")
-DGIT_HEAD_OVERRIDE=[XXXXXXX] Specify the 7 digit git commit ref for this build. Used with distribution builds (default: "")
```

Installation
============

After you have successfully built KeePassXC, install the binary by executing the following:

```
sudo make install
```

Packaging
=========

You can create a package to redistribute KeePassXC (zip, deb, rpm, dmg, etc..). Refer to [keepassxc-packaging](https://github.com/keepassxreboot/keepassxc-packaging) for packaging scripts.

To package using CMake, run the following command using whichever [generators](https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html) you would like to package with.

```
cpack -G "ZIP;WIX"
```

Testing
=======

You can perform tests on the built executables with:
```
make test ARGS+="--output-on-failure"
```

On Linux, if you are not currently running on an X Server or Wayland, run the tests as follows:
```
make test ARGS+="-E test\(cli\|gui\) --output-on-failure"
xvfb-run -e errors -a --server-args="-screen 0 1024x768x24" make test ARGS+="-R test\(cli\|gui\) --output-on-failure"
```

Common parameters:
```
CTEST_OUTPUT_ON_FAILURE=1
ARGS+=-jX
ARGS+="-E testgui"
```
