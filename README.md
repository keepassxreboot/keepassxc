# <img src="https://keepassxc.org/logo.png" width="40" height="40"/> KeePassXC
[![TeamCity Build Status](https://ci.keepassxc.org/app/rest/builds/buildType:\(id:KeepassXC_TeamCityCi\)/statusIcon?guest=1)](https://ci.keepassxc.org/viewType.html?buildTypeId=KeepassXC_TeamCityCi&guest=1) [![Build status](https://ci.appveyor.com/api/projects/status/qmcar8rnqjh4oxof?svg=true)](https://ci.appveyor.com/project/droidmonkey/keepassxc) [![codecov](https://codecov.io/gh/keepassxreboot/keepassxc/branch/develop/graph/badge.svg)](https://codecov.io/gh/keepassxreboot/keepassxc)


## About KeePassXC
[KeePassXC](https://keepassxc.org) is a cross-platform community fork of
[KeePassX](https://www.keepassx.org/).
Our goal is to extend and improve it with new features and bugfixes
to provide a feature-rich, fully cross-platform and modern
open-source password manager.

## Installation
The [KeePassXC QuickStart](./docs/QUICKSTART.md) gets you started using
KeePassXC on your Windows, Mac, or Linux computer using pre-compiled binaries
from the [downloads page](https://keepassxc.org/download).

Additionally, individual Linux distributions may ship their own versions,
so please check out your distribution's package list to see if KeePassXC is available.

## Additional features compared to KeePassX
- Auto-Type on all three major platforms (Linux, Windows, macOS)
- Twofish encryption
- YubiKey challenge-response support
- TOTP generation
- CSV import
- Command line interface
- DEP and ASLR hardening
- Stand-alone password and passphrase generator
- Password strength meter
- Using website favicons as entry icons
- Merging of databases
- Automatic reload when the database changed on disk
- Browser integration with KeePassHTTP-Connector for
[Mozilla Firefox](https://addons.mozilla.org/en-US/firefox/addon/keepasshttp-connector/) and
[Google Chrome or Chromium](https://chrome.google.com/webstore/detail/keepasshttp-connector/dafgdjggglmmknipkhngniifhplpcldb), and
[passafari](https://github.com/mmichaa/passafari.safariextension/) in Safari. [[See note about KeePassHTTP]](#note-about-keepasshttp)
- Browser integration with KeePassXC-Browser using [native messaging](https://developer.chrome.com/extensions/nativeMessaging) for [Mozilla Firefox](https://addons.mozilla.org/en-US/firefox/addon/keepassxc-browser/) and [Google Chrome or Chromium](https://chrome.google.com/webstore/detail/keepassxc-browser/oboonakemofpalcgghocfoadofidjkkk)
- Many bug fixes

For a full list of features and changes, read the [CHANGELOG](CHANGELOG) document.

## Building KeePassXC

Detailed instructions are available in the [Build and Install](./INSTALL.md)
page or on the [Wiki page](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC).

## Contributing

We are always looking for suggestions how to improve our application.
If you find any bugs or have an idea for a new feature, please let us know by
opening a report in our [issue tracker](https://github.com/keepassxreboot/keepassxc/issues)
on GitHub or join us on IRC on freenode channels #keepassxc or #keepassxc-dev.

You can of course also directly contribute your own code. We are happy to accept your pull requests.

Please read the [CONTRIBUTING document](.github/CONTRIBUTING.md) for further information.

### Note about KeePassHTTP
The KeePassHTTP protocol is not a highly secure protocol.
It has a certain flaw which could allow an attacker to decrypt your passwords
should they manage to impersonate the web browser extension from a remote address.
<!--intercept communication between a KeePassHTTP server
and PassIFox/chromeIPass over a network connection -->
(See [here](https://github.com/pfn/keepasshttp/issues/258) and [here](https://github.com/keepassxreboot/keepassxc/issues/147)).

To minimize the risk, KeePassXC strictly limits communication between itself
and the browser plugin to your local computer (localhost).
This makes your passwords quite safe,
but as with all open source software, use it at your own risk!
