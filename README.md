# <img src="https://keepassxc.org/images/keepassxc-logo.svg" width="40" height="40"/> KeePassXC
[![TeamCity Build Status](https://ci.keepassxc.org/app/rest/builds/buildType:\(project:KeepassXC\)/statusIcon)](https://ci.keepassxc.org/?guest=1)
[![codecov](https://codecov.io/gh/keepassxreboot/keepassxc/branch/develop/graph/badge.svg)](https://codecov.io/gh/keepassxreboot/keepassxc)
[![GitHub release](https://img.shields.io/github/release/keepassxreboot/keepassxc)](https://github.com/keepassxreboot/keepassxc/releases/)

[KeePassXC](https://keepassxc.org) is a modern, secure, and open-source password manager that stores and manages your most sensitive information. You can run KeePassXC on Windows, macOS, and Linux systems. KeePassXC is for people with extremely high demands of secure personal data management. It saves many different types of information, such as usernames, passwords, URLs, attachments, and notes in an offline, encrypted file that can be stored in any location, including private and public cloud solutions. For easy identification and management, user-defined titles and icons can be specified for entries. In addition, entries are sorted in customizable groups. An integrated search function allows you to use advanced patterns to easily find any entry in your database. A customizable, fast, and easy-to-use password generator utility allows you to create passwords with any combination of characters or easy to remember passphrases.

## Quick Start
The [QuickStart Guide](https://keepassxc.org/docs/KeePassXC_GettingStarted.html) gets you started using KeePassXC on your Windows, macOS, or Linux computer using pre-compiled binaries from the [downloads page](https://keepassxc.org/download). Additionally, individual Linux distributions may ship their own versions, so please check your distribution's package list to see if KeePassXC is available. Detailed documentation is available in the [User Guide](https://keepassxc.org/docs/KeePassXC_UserGuide.html).

## Features List
KeePassXC has numerous features for novice and power users alike. Our goal is to create an application that can be used by anyone while still offering advanced features to those that need them.

### Basic
* Create, open, and save databases in the KDBX format (KeePass compatible to KDBX4 and KDBX3)
* Store sensitive information in entries that are organized by groups
* Search for entries
* Password generator
* Auto-Type passwords into applications
* Browser integration with Google Chrome, Mozilla Firefox, Microsoft Edge, Chromium, Vivaldi, Brave, and Tor-Browser
* Entry icon download
* Import databases from CSV, 1Password, and KeePass1 formats

### Advanced
* Database reports (password health, HIBP, and statistics)
* Database export to CSV and HTML formats
* TOTP storage and generation
* Field references between entries
* File attachments and custom attributes
* Entry history and data restoration
* YubiKey/OnlyKey challenge-response support
* Command line interface (keepassxc-cli)
* Auto-Open databases
* KeeShare shared databases (import, export, and synchronize)
* SSH Agent
* FreeDesktop.org Secret Service (replace Gnome keyring, etc.)
* Additional encryption choices: Twofish and ChaCha20

For a full list of changes, read the [CHANGELOG](CHANGELOG.md) document. \
For a full list of keyboard shortcuts, see [KeyboardShortcuts.adoc](./docs/topics/KeyboardShortcuts.adoc)

## Building KeePassXC

Detailed instructions are available in the [Build and Install](./INSTALL.md) page and in the [Wiki](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC).

## Contributing

We are always looking for suggestions on how to improve KeePassXC. If you find any bugs or have an idea for a new feature, please let us know by opening a report in the [issue tracker](https://github.com/keepassxreboot/keepassxc/issues) on GitHub or join us on IRC in [freenode](https://webchat.freenode.net/) channels #keepassxc and #keepassxc-dev.

You may directly contribute your own code by submitting a pull request. Please read the [CONTRIBUTING](.github/CONTRIBUTING.md) document for further information.

Contributors are required to adhere to the project's [Code of Conduct](CODE-OF-CONDUCT.md).

## License

KeePassXC code is licensed under GPL-2 or GPL-3. Additional licensing for third-party files is detailed in [COPYING](./COPYING).
