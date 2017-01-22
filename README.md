# KeePassXC - KeePass Cross-platform Community Edition

[![Travis Build Status](https://travis-ci.org/keepassxreboot/keepassxc.svg?branch=develop)](https://travis-ci.org/keepassxreboot/keepassxc)  [![Coverage Status](https://coveralls.io/repos/github/keepassxreboot/keepassxc/badge.svg)](https://coveralls.io/github/keepassxreboot/keepassxc)

## About

Fork of [KeePassX](https://www.keepassx.org/) that [aims to incorporate stalled Pull Requests, features, and bug fixes that are not being incorporated into the main KeePassX baseline](https://github.com/keepassxreboot/keepassx/issues/43).


#### Additional Reboot Features
 - keepasshttp support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) for Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) for Google Chrome.

KeePassHttp implementation has been forked from jdachtera's repository, which in turn was based on code from code with Francois Ferrand's [keepassx-http](https://gitorious.org/keepassx/keepassx-http/source/master) repository.

This is a rebuild from [denk-mal's keepasshttp](https://github.com/denk-mal/keepassx.git) that brings it forward to Qt5 and KeePassX v2.x.


### Installation

Right now KeePassXC does not have a precompiled executable or an installation package.<br/>
So you must install it from its source code.

**More detailed instructions are available in the INSTALL file or at the [Wiki page](https://github.com/keepassxreboot/keepassx/wiki/Install-Instruction-from-Source).**

First you must download the KeePassXC source code as ZIP file or with Git.

Generally you can build and install KeePassXC with the following commands from a Terminal in the KeePassXC folder
```
mkdir build
cd build
cmake -DWITH_TESTS=OFF ..
make
sudo make install
```


### Clone Repository

Clone the repository to a suitable location where you can extend and build this project.

```bash
git clone https://github.com/keepassxreboot/keepassxc.git
```

**Note:** This will clone the entire contents of the repository at the HEAD revision.

To update the project from within the project's folder you can run the following command:

```bash
git pull
```


### Contributing

We're always looking for suggestions to improve our application. If you have a suggestion for improving an existing feature,
or would like to suggest a completely new feature for KeePassX Reboot, please use the [Issues](https://github.com/keepassxreboot/keepassxc/issues) section or our [Google Groups](https://groups.google.com/forum/#!forum/keepassx-reboot) forum.

Please review the [CONTRIBUTING](.github/CONTRIBUTING.md) document for further information.
