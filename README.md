# KeePassX + keepasshttp

## About

Fork of [KeePassX](https://www.keepassx.org/) with keepasshttp support for use with [PassIFox](https://addons.mozilla.org/en-us/firefox/addon/passifox/) for Mozilla Firefox and [chromeIPass](https://chrome.google.com/webstore/detail/chromeipass/ompiailgknfdndiefoaoiligalphfdae) for Google Chrome.

KeePassHttp implementation has been forked from jdachtera's repository, which in turn was based on code from code with Francois Ferrand's [keepassx-http](https://gitorious.org/keepassx/keepassx-http/source/master:) repository. 

My intention is to keep this repository as up-to-date with the main keePassX repo as possible and, time allowing, clean-up the keepasshttp implementation enough for it to be merged with upstream. I have started removing any additions to the code that were not strictly related to implemeting the keepasshttp protocol in KeePassX.

#### Build Dependencies

The following tools must exist within your PATH:

* make
* cmake (>= 2.6.4)
* g++ or clang++

The following libraries are required:

* Qt 4 (>= 4.6)
* libgcrypt
* zlib
* libmicrohttpd
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
