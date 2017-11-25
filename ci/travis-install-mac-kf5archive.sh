#!/bin/sh

# Note:
#   We could use homebrew, but those bottles come with all the bells and whistles enabled
#   and thus, take too long to install.
# For example:
#   brew tap kde-mac/kde;
#   brew install kde-mac/kde/kf5-karchive;

set -ex

curl -O -J -L https://download.kde.org/stable/frameworks/5.40/karchive-5.40.0.tar.xz
curl -O -J -L https://download.kde.org/stable/frameworks/5.40/extra-cmake-modules-5.40.0.tar.xz

tar -xf extra-cmake-modules-5.40.0.tar.xz
tar -xf karchive-5.40.0.tar.xz

export CMAKE_PREFIX_PATH=`find /usr/local/Cellar/qt/*/lib -type d | grep cmake$`

cd extra-cmake-modules-5.40.0
mkdir build && cd build
cmake \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=${CONFIG} ..
make -j2
make install
cd ../..

cd karchive-5.40.0
mkdir build && cd build
cmake \
  -DBUILD_TESTING=OFF \
  -DBUILD_QCH=OFF \
  -DKDE_INSTALL_QMLDIR=lib/qt5/qml \
  -DKDE_INSTALL_PLUGINDIR=lib/qt5/plugins \
  -DZLIB_LIBRARY=`find /usr/local/Cellar/zlib | grep libz.dylib$` \
  -DZLIB_INCLUDE_DIR=`find /usr/local/Cellar/zlib -d | grep /include$` \
  -DCMAKE_BUILD_TYPE=${CONFIG} ..
make -j2
make install
cd ../..
