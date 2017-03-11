# KeePassXC [![Travis Build Status](https://travis-ci.org/keepassxreboot/keepassxc.svg?branch=develop)](https://travis-ci.org/keepassxreboot/keepassxc)  [![Coverage Status](https://coveralls.io/repos/github/keepassxreboot/keepassxc/badge.svg)](https://coveralls.io/github/keepassxreboot/keepassxc) <a href='https://pledgie.com/campaigns/33487'><img alt='KeePassXC Authenticode Certificate Campaign!' align=right  src='https://pledgie.com/campaigns/33487.png?skin_name=chrome' border='0'></a>

KeePass Cross-platform Community Edition

## About
[KeePassXC](https://keepassxc.org) is a community fork of [KeePassX](https://www.keepassx.org/) with the goal to extend and improve it with new features and bugfixes to provide a feature-rich, fully cross-platform and modern open-source password manager.


## Additional features compared to KeePassX
- Auto-Type on all three major platforms (Linux, Windows, OS X)
- Stand-alone password generator
- Password strength meter
- Yubikey 2FA support for unlocking databases
- Using website favicons as entry icons
- Merging of databases
- Automatic reload when the database changed on disk
- KeePassHTTP support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) in Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) in Google Chrome or Chromium.
- Many bug fixes

For a full list of features and changes, read the [CHANGELOG](CHANGELOG) document.

### Note about KeePassHTTP
KeePassHTTP is not a highly secure protocol and has certain flaw which allow an attacker to decrypt your passwords when they manage to intercept communication between a KeePassHTTP server and PassIFox/chromeIPass over a network connection (see [here](https://github.com/pfn/keepasshttp/issues/258) and [here](https://github.com/keepassxreboot/keepassxc/issues/147)). KeePassXC therefore strictly limits communication between itself and the browser plugin to your local computer. As long as your computer is not compromised, your passwords are fairly safe that way, but use it at your own risk!

### Installation
Pre-compiled binaries can be found on the [downloads page](https://keepassxc.org/download).  Additionally, individual Linux distributions may ship their own versions, so please check out your distribution's package list to see if KeePassXC is available.

### Building KeePassXC yourself

*More detailed instructions are available in the INSTALL file or on the [Wiki page](https://github.com/keepassxreboot/keepassx/wiki/Install-Instruction-from-Source).*

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

Once you have downloaded the source code, you can `cd` into the source code directory and build and install KeePassXC with

```
mkdir build
cd build
cmake -DWITH_TESTS=OFF ..
make -j8
sudo make install
```

To enable autotype, add `-DWITH_XC_AUTOTYPE=ON` to the `cmake` command. KeePassHTTP support is compiled in by adding `-DWITH_XC_HTTP=ON`. If these options are not specified, KeePassXC will be built without these plugins.


### Contributing

We are always looking for suggestions how to improve our application. If you find any bugs or have an idea for a new feature, please let us know by opening a report in our [issue tracker](https://github.com/keepassxreboot/keepassxc/issues) on GitHub or join us on IRC on freenode channels #keepassxc or #keepassxc-dev.

You can of course also directly contribute your own code. We are happy to accept your pull requests.

Please read the [CONTRIBUTING](.github/CONTRIBUTING.md) document for further information.
