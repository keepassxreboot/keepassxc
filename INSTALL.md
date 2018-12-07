Build and Install KeePassXC
=================

This document will guide you through the steps to build and install KeePassXC from source.
You can visit the online version of this document at the following link:

https://github.com/keepassxreboot/keepassx/wiki/Install-Instruction-from-Source

The [KeePassXC QuickStart](./docs/QUICKSTART.md) gets you started using KeePassXC on your
Windows, Mac, or Linux computer using the pre-built binaries.

Build Dependencies
==================

The following tools must exist within your PATH:

* make
* cmake (>= 2.8.12)
* g++ (>= 4.7) or clang++ (>= 3.0)

The following libraries are required:

* Qt 5 (>= 5.2): qtbase and qttools5
* libgcrypt (>= 1.6)
* zlib
* libmicrohttpd
* libxi, libxtst, qtx11extras (optional for auto-type on X11)
* libsodium (>= 1.0.12, optional for KeePassXC-Browser support)
* libargon2

Prepare the Building Environment
================================

* [Building Environment on Linux](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Linux)
* [Building Environment on Windows](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Windows)
* [Building Environment on MacOS](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-OS-X)

Build Steps
===========
We recommend using the release tool to perform builds, please read up-to-date instructions [on our wiki](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC#building-using-the-release-tool).

To compile from source, open a **Terminal (on Linux/MacOS)** or a **MSYS2-MinGW shell (on Windows)**<br/>
**Note:** on Windows make sure you are using a **MINGW shell** by checking the label before the current path

First, download the KeePassXC [source tarball](https://keepassxc.org/download#source)
or check out the latest version from our [Git repository](https://github.com/keepassxreboot/keepassxc).

To clone the project from Git, `cd` to a suitable location and run

```bash
git clone https://github.com/keepassxreboot/keepassxc.git
```

This will clone the entire contents of the repository and check out the current `develop` branch.

To update the project from within the project's folder, you can run the following command:

```bash
git pull
```

For a stable build, it is recommended to checkout the master branch.

```bash
git checkout master
```

Navigate to the directory where you have downloaded KeePassXC and type these commands:

```
cd directory-where-sources-live
mkdir build
cd build
cmake -DWITH_XC_ALL=ON ..
make
```

If you are on Windows, you may have to add ```-G "MSYS Makefiles"``` to the beginning of the cmake command. See the [Windows Build Instructions](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC#windows) for more information.

These steps place the compiled KeePassXC binary inside the `./build/src/` directory.
(Note the cmake notes/options below.)

**Cmake Notes:**

* Common cmake parameters

	```
	-DCMAKE_INSTALL_PREFIX=/usr/local
	-DCMAKE_VERBOSE_MAKEFILE=ON
	-DCMAKE_BUILD_TYPE=<RelWithDebInfo/Debug/Release>
	-DWITH_GUI_TESTS=ON
	```

* cmake accepts the following options:

	```
	  -DWITH_XC_AUTOTYPE=[ON|OFF] Enable/Disable Auto-Type (default: ON)
	  -DWITH_XC_YUBIKEY=[ON|OFF] Enable/Disable YubiKey HMAC-SHA1 authentication support (default: OFF)
	  -DWITH_XC_BROWSER=[ON|OFF] Enable/Disable KeePassXC-Browser extension support (default: OFF)
	  -DWITH_XC_NETWORKING=[ON|OFF] Enable/Disable Networking support (favicon download) (default: OFF)
	  -DWITH_XC_SSHAGENT=[ON|OFF] Enable/Disable SSHAgent support (default: OFF)
	  -DWITH_XC_KEESHARE=[ON|OFF] Enable/Disable KeeShare group syncronization extension (default: OFF)
	  -DWITH_XC_TOUCHID=[ON|OFF] (macOS Only) Enable/Disable Touch ID unlock (default:OFF)
	  -DWITH_XC_UPDATECHECK=[ON|OFF] Enable/Disable checking for updates when KeePassXC starts. (default: OFF)
	  -DWITH_XC_ALL=[ON|OFF] Enable/Disable compiling all plugins above (default: OFF)
	  -DWITH_XC_KEESHARE_SECURE=[ON|OFF] Enable/Disable KeeShare secure containers, requires libquazip5 (default: OFF)
	  -DWITH_TESTS=[ON|OFF] Enable/Disable building of unit tests (default: ON)
	  -DWITH_GUI_TESTS=[ON|OFF] Enable/Disable building of GUI tests (default: OFF)
	  -DWITH_DEV_BUILD=[ON|OFF] Enable/Disable deprecated method warnings (default: OFF)
	  -DWITH_ASAN=[ON|OFF] Enable/Disable address sanitizer checks (Linux / macOS only) (default: OFF)
	  -DWITH_COVERAGE=[ON|OFF] Enable/Disable coverage tests (GCC only) (default: OFF)
	  -DWITH_APP_BUNDLE=[ON|OFF] Enable Application Bundle for macOS (default: ON)
	```

* If you are on MacOS you must add this parameter to **Cmake**, with the Qt version you have installed<br/> `-DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.6.2/lib/cmake/`

:exclamation: When building with ASan support on macOS, you need to use `export ASAN_OPTIONS=detect_leaks=0` before running the tests (no LSan support in macOS).

Installation
============

After you have successfully built KeePassXC, install the binary by executing the following:

```bash
sudo make install
```

You can specify the destination dir with
```
DESTDIR=X
```


Packaging
=========

You can create a package to redistribute KeePassXC (zip, deb, rpm, dmg, etc..)
```
make package
```


Testing
=======

You can perform test on the executable
```
make test
```

Common parameters:
```
CTEST_OUTPUT_ON_FAILURE=1
ARGS+=-jX
ARGS+="-E testgui"
```
