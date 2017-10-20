# <img src="https://keepassxc.org/logo.png" width="40" height="40"/> KeePassXC [![Travis Build Status](https://travis-ci.org/keepassxreboot/keepassxc.svg?branch=develop)](https://travis-ci.org/keepassxreboot/keepassxc)  [![Coverage Status](https://coveralls.io/repos/github/keepassxreboot/keepassxc/badge.svg)](https://coveralls.io/github/keepassxreboot/keepassxc)

KeePass Cross-platform Community Edition

## About
[KeePassXC](https://keepassxc.org) is a community fork of [KeePassX](https://www.keepassx.org/) with the goal to extend and improve it with new features and bugfixes to provide a feature-rich, fully cross-platform and modern open-source password manager.


## Additional features compared to KeePassX
- Auto-Type on all three major platforms (Linux, Windows, OS X)
- Stand-alone password generator
- Password strength meter
- YubiKey HMAC-SHA1 authentication for unlocking databases
- Using website favicons as entry icons
- Merging of databases
- Automatic reload when the database changed on disk
- KeePassHTTP support for use with KeePassHTTP-Connector for [Mozilla Firefox](https://addons.mozilla.org/en-US/firefox/addon/keepasshttp-connector/) and [Google Chrome or Chromium](https://chrome.google.com/webstore/detail/keepasshttp-connector/dafgdjggglmmknipkhngniifhplpcldb), and [passafari](https://github.com/mmichaa/passafari.safariextension/) in Safari.
- Many bug fixes

For a full list of features and changes, read the [CHANGELOG](CHANGELOG) document.

### Note about KeePassHTTP
KeePassHTTP is not a highly secure protocol and has certain flaw which allow an attacker to decrypt your passwords when they manage to intercept communication between a KeePassHTTP server and PassIFox/chromeIPass over a network connection (see [here](https://github.com/pfn/keepasshttp/issues/258) and [here](https://github.com/keepassxreboot/keepassxc/issues/147)). KeePassXC therefore strictly limits communication between itself and the browser plugin to your local computer. As long as your computer is not compromised, your passwords are fairly safe that way, but use it at your own risk!

### Installation
Pre-compiled binaries can be found on the [downloads page](https://keepassxc.org/download).  Additionally, individual Linux distributions may ship their own versions, so please check out your distribution's package list to see if KeePassXC is available.

### Building KeePassXC

*More detailed instructions are available in the INSTALL file or on the [Wiki page](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC).*

First, you must download the KeePassXC [source tarball](https://keepassxc.org/download#source) or check out the latest version from our [Git repository](https://github.com/keepassxreboot/keepassxc).

To clone the project from Git, `cd` to a suitable location and run

```bash
git clone https://github.com/keepassxreboot/keepassxc.git
```

This will clone the entire contents of the repository and check out the current `develop` branch.

To update the project from within the project's folder, you can run the following command:

```bash
git pull
```

Once you have downloaded the source code, you can `cd` into the source code directory, build and install KeePassXC:

```bash
mkdir build
cd build
cmake -DWITH_TESTS=OFF ..
make -j8
sudo make install
```

cmake accepts the following options:

```
  -DWITH_XC_AUTOTYPE=[ON|OFF] Enable/Disable Auto-Type (default: ON)
  -DWITH_XC_HTTP=[ON|OFF] Enable/Disable KeePassHTTP and custom icon downloads (default: OFF)
  -DWITH_XC_YUBIKEY=[ON|OFF] Enable/Disable YubiKey HMAC-SHA1 authentication support (default: OFF)

  -DWITH_TESTS=[ON|OFF] Enable/Disable building of unit tests (default: ON)
  -DWITH_GUI_TESTS=[ON|OFF] Enable/Disable building of GUI tests (default: OFF)
  -DWITH_DEV_BUILD=[ON|OFF] Enable/Disable deprecated method warnings (default: OFF)
  -DWITH_ASAN=[ON|OFF] Enable/Disable address sanitizer checks (Linux only) (default: OFF)
  -DWITH_COVERAGE=[ON|OFF] Enable/Disable coverage tests (GCC only) (default: OFF)
```

### Contributing

We are always looking for suggestions how to improve our application. If you find any bugs or have an idea for a new feature, please let us know by opening a report in our [issue tracker](https://github.com/keepassxreboot/keepassxc/issues) on GitHub or join us on IRC on freenode channels #keepassxc or #keepassxc-dev.

You can of course also directly contribute your own code. We are happy to accept your pull requests.

Please read the [CONTRIBUTING document](.github/CONTRIBUTING.md) for further information.
