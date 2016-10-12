# KeePassX Reboot

[![Travis Build Status](https://travis-ci.org/keepassxreboot/keepassx.svg?branch=develop)](https://travis-ci.org/keepassxreboot/keepassx)  [![Coverage Status](https://coveralls.io/repos/github/keepassxreboot/keepassx/badge.svg?branch=develop)](https://coveralls.io/github/keepassxreboot/keepassx?branch=develop)

## About

Fork of [KeePassX](https://www.keepassx.org/) with keepasshttp support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) for Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) for Google Chrome.

KeePassHttp implementation has been forked from jdachtera's repository, which in turn was based on code from code with Francois Ferrand's [keepassx-http](https://gitorious.org/keepassx/keepassx-http/source/master) repository. 

This is a rebuild from [denk-mal's keepasshttp](https://github.com/denk-mal/keepassx.git) that brings it forward to Qt5 and KeePassX v2.x.

This reboot also aims to incorporate stalled Pull Requests, features, and bug fixes that are not being incorporated into the main KeePassX baseline.

#### Build Dependencies

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

On Debian/Ubuntu you can install them with:

```bash
sudo apt-get install build-essential cmake libmicrohttpd-dev libxi-dev libxtst-dev qtbase5-dev libqt5x11extras5-dev qttools5-dev qttools5-dev-tools libgcrypt20-dev zlib1g-dev
```

On Fedora/RHEL/CentOS you can install them with:

```bash
sudo dnf install make automake gcc gcc-c++ cmake libmicrohttpd-devel libXi-devel libXtst-devel qt5-qtbase-devel qt5-qtx11extras qt5-qttools libgcrypt-devel zlib-devel
```

#### Build Steps

To compile from source:

```bash
mkdir build
cd build
cmake -DWITH_TESTS=OFF ..
make [-jX]
```

You will have the compiled KeePassX binary inside the `./build/src/` directory.

To install this binary execute the following:

```bash
sudo make install
```

More detailed instructions available in the INSTALL file.

### Clone Repository

Clone the repository to a suitable location where you can extend and build this project.

```bash
git clone https://github.com/keepassxreboot/keepassx.git
```

**Note:** This will clone the entire contents of the repository at the HEAD revision.

To update the project from within the project's folder you can run the following command:

```bash
git pull
```

### Feature Requests

We're always looking for suggestions to improve our application. If you have a suggestion for improving an existing feature,
or would like to suggest a completely new feature for KeePassX Reboot, please use the Issues section.

### Bug Reports

Our software isn't always perfect, but we strive to always improve our work. You may file bug reports in the Issues section.

### Pull Requests

Along with our desire to hear your feedback and suggestions, we're also interested in accepting direct assistance in the form of code.

All pull requests must comply with the CONTRIBUTION requirements.

### Translations

Translations are managed on [Transifex](https://www.transifex.com/keepassx-reboot/keepassx-reboot/) which offers a web interface.
Please join an existing language team or request a new one if there is none.
