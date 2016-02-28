# KeePassX v2.0.2 + keepasshttp

## About

Fork of [KeePassX](https://www.keepassx.org/) with keepasshttp support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) for Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) for Google Chrome.

KeePassHttp implementation has been forked from jdachtera's repository, which in turn was based on code from code with Francois Ferrand's [keepassx-http](https://gitorious.org/keepassx/keepassx-http/source/master:) repository. 

This is a rebuild from [denk-mal's keepasshttp](https://github.com/denk-mal/keepassx.git) that brings it forward to Qt5 and KeePassX v2.0.2.

*NOTE: I have not been able to get the tests to build properly yet.*

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
* libxtst (optional for auto-type on X11)
* libxtst, qtx11extras (optional for auto-type on X11)

On Debian you can install them with:

```bash
sudo apt-get install build-essential cmake qtbase5-dev libqt5x11extras5-dev qttools5-dev qttools5-dev-tools libgcrypt20-dev zlib1g-dev
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

## Contribute

Coordination of work between developers is handled through the [KeePassX development](https://www.keepassx.org/dev/) site.
Requests for enhancements, or reports of bugs encountered, can also be reported through the KeePassX development site.
However, members of the open-source community are encouraged to submit pull requests directly through GitHub.

### Clone Repository

Clone the repository to a suitable location where you can extend and build this project.

```bash
git clone https://github.com/keepassx/keepassx.git
```

**Note:** This will clone the entire contents of the repository at the HEAD revision.

To update the project from within the project's folder you can run the following command:

```bash
git pull
```

### Feature Requests

We're always looking for suggestions to improve our application. If you have a suggestion for improving an existing feature,
or would like to suggest a completely new feature for KeePassX, please file a ticket on the [KeePassX development](https://www.keepassx.org/dev/) site.

### Bug Reports

Our software isn't always perfect, but we strive to always improve our work. You may file bug reports on the [KeePassX development](https://www.keepassx.org/dev/) site.

### Pull Requests

Along with our desire to hear your feedback and suggestions, we're also interested in accepting direct assistance in the form of code.

Issue merge requests against our [GitHub repository](https://github.com/keepassx/keepassx).

### Translations

Translations are managed on [Transifex](https://www.transifex.com/projects/p/keepassx/) which offers a web interface.
Please join an existing language team or request a new one if there is none.
