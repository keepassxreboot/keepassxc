#!/bin/sh

set -ex

brew update;
brew upgrade cmake;
brew ls | grep -wq libgcrypt || brew install libgcrypt;
brew ls | grep -wq zlib || brew install zlib;
brew ls | grep -wq libyubikey || brew install libyubikey;
brew ls | grep -wq ykpers || brew install ykpers;
brew ls | grep -wq qt5 || brew install qt5;
