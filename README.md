# KeePassX

## About

KeePassX is an application for people with extremely high demands on secure personal data management.
It has a light interface, is cross platform and published under the terms of the GNU General Public License.

KeePassX saves many different information e.g. user names, passwords, urls, attachments and comments in one single database.
For a better management user-defined titles and icons can be specified for each single entry.
Furthermore the entries are sorted in groups, which are customizable as well. The integrated search function allows to search in a single group or the complete database.
KeePassX offers a little utility for secure password generation. The password generator is very customizable, fast and easy to use.
Especially someone who generates passwords frequently will appreciate this feature.

The complete database is always encrypted with the AES (aka Rijndael) encryption algorithm using a 256 bit key.
Therefore the saved information can be considered as quite safe. KeePassX uses a database format that is compatible with [KeePass Password Safe](http://keepass.info/).
This makes the use of that application even more favorable.

## Install

KeePassX can be downloaded and installed using an assortment of installers available on the main [KeePassX website](http://www.keepassx.org).
KeePassX can also be installed from the official repositories of many Linux repositories.
If you wish to build KeePassX from source, rather than rely on the pre-compiled binaries, you may wish to read up on the _From Source_ section.

### Debian

To install KeePassX from the Debian repository:

```bash
sudo apt-get install keepassx
```

### Red Hat

Install KeePassX from the Red Hat (or CentOS) repository:

```bash
sudo yum install keepassx
```

### Windows / Mac OS X

Download the installer from the KeePassX [download](https://www.keepassx.org/downloads) page.
Once downloaded, double click on the file to execute the installer.

### From Source

#### Build Dependencies

The following tools must exist within your PATH:

* make
* cmake (>= 2.8.12)
* g++ (>= 4.7) or clang++ (>= 3.0)

The following libraries are required:

* Qt 5 (>= 5.2): qtbase and qttools5
* libgcrypt (>= 1.6)
* zlib
* libxi, libxtst, qtx11extras (optional for auto-type on X11)

On Debian you can install them with:

```bash
sudo apt-get install build-essential cmake qtbase5-dev libqt5x11extras5-dev qttools5-dev qttools5-dev-tools libgcrypt20-dev zlib1g-dev libxi-dev libxtst-dev
```

#### Build Steps

To compile from source:

```bash
mkdir build
cd build
cmake ..
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
