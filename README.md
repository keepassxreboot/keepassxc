# KeePassX + keepasshttp

## About

Fork of [KeePassX](https://www.keepassx.org/) with keepasshttp support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) for Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) for Google Chrome.

My intention is to keep this repository as up-to-date with the main keePassX repo as possible and, time allowing, clean-up the keepasshttp implementation enough for it to be merged with upstream.

#### Build Dependencies

The following tools must exist within your PATH:

* make
* cmake (>= 2.6.4)
* g++ or clang++

The following libraries are required:

* Qt 4 (>= 4.6)
* libgcrypt
* zlib
* QJSON

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
