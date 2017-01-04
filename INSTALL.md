Install KeePassXC
=================

This document will guide you across the steps to install KeePassXC.
You can visit the online version of this document a the following link

https://github.com/keepassxreboot/keepassx/wiki/Install-Instruction-from-Source


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


Prepare the Building Environment
================================

Building Environment on Linux    ==> https://github.com/keepassxreboot/keepassx/wiki/Building-Environment-on-Linux
Building Environment on Windows  ==> https://github.com/keepassxreboot/keepassx/wiki/Building-Environment-on-Windows
Building Environment on MacOS    ==> https://github.com/keepassxreboot/keepassx/wiki/Building-Environment-on-MacOS


Build Steps
===========

To compile from source, open a **Terminal (on Linux/MacOS)** or a **MSYS2-MinGW shell (on Windows)**<br/>
**Note:** on Windows make sure you are using a **MINGW shell** by checking the label before the current path

Navigate to the path you have downloaded KeePassXC and type these commands:

```
mkdir build
cd build
cmake -DWITH_TESTS=OFF
make
```

**Note:** If you are on MacOS you must add this parameter to **Cmake**, with the Qt version you have installed<br/> `-DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.6.2/lib/cmake/`

You will have the compiled KeePassXC binary inside the `./build/src/` directory.

Common cmake parameters
```
-DCMAKE_INSTALL_PREFIX=/usr/local
-DCMAKE_VERBOSE_MAKEFILE=ON
-DCMAKE_BUILD_TYPE=<RelWithDebInfo/Debug/Release>
-DWITH_GUI_TESTS=ON
```


Installation
============

To install this binary execute the following:

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
