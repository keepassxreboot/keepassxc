#!/bin/sh

set -ex

sudo apt-get -qq update;
sudo apt-get -qq install \
  cmake \
  libclang-common-3.5-dev \
  qtbase5-dev libqt5x11extras5-dev qttools5-dev qttools5-dev-tools \
  libgcrypt20-dev zlib1g-dev libyubikey-dev libykpers-1-dev \
  libxi-dev libxtst-dev xvfb;
