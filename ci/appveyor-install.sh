#!/usr/bin/bash

pacman --needed --noconfirm -Sy \
  bash pacman pacman-mirrors msys2-runtime make \
  mingw-w64-$(uname -m)-zlib \
  mingw-w64-$(uname -m)-gcc \
  mingw-w64-$(uname -m)-binutils \
  mingw-w64-$(uname -m)-cmake \
  mingw-w64-$(uname -m)-clang \
  mingw-w64-$(uname -m)-libtool

pacman -Sy --noconfirm \
  mingw-w64-$(uname -m)-libgcrypt \
  mingw-w64-$(uname -m)-qt5

#pacman -Sy --noconfirm mingw-w64-$(uname -m)-libusb

pacman -R --noconfirm libtool

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin:$PATH"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"
#export LIBRARY_PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$LIBRARY_PATH"

mkdir deps && cd deps
#curl -O -J -L https://developers.yubico.com/yubico-c/Releases/libyubikey-1.13.tar.gz
#curl -O -J -L https://developers.yubico.com/yubikey-personalization/Releases/ykpers-1.18.0.tar.gz
curl -O -J -L https://developers.yubico.com/yubikey-personalization/Releases/ykpers-1.18.0-win64.zip
curl -O -J -L https://github.com/nitroshare/qhttpengine/archive/1.0.0.tar.gz
curl -O -J -L https://download.kde.org/stable/frameworks/5.40/karchive-5.40.0.tar.xz
curl -O -J -L https://download.kde.org/stable/frameworks/5.40/extra-cmake-modules-5.40.0.tar.xz

#tar -xf libyubikey-1.13.tar.gz
#tar -xf ykpers-1.18.0.tar.gz
7z x ykpers-1.18.0-win64.zip -o/usr/local
tar -xf qhttpengine-1.0.0.tar.gz
tar -xf extra-cmake-modules-5.40.0.tar.xz
tar -xf karchive-5.40.0.tar.xz

#cd libyubikey-1.13
#./configure \
#  --prefix=/mingw64 \
#  --program-prefix=mingw-w64-$(uname -m)-
#make check
#make install
#cd ..

#cd ykpers-1.18.0
#./configure \
#  --prefix=/mingw64 \
#  --program-prefix=mingw-w64-$(uname -m)-
#make check
#make install
#cd ..

cd qhttpengine-1.0.0
mkdir build && cd build
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=${configuration} ..
make -j2
make install
cd ../..

cd extra-cmake-modules-5.40.0
mkdir build && cd build
cmake -G "MSYS Makefiles" \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=${configuration} ..
make -j2
make install
cd ../..

cd karchive-5.40.0
mkdir build && cd build
cmake -G "MSYS Makefiles" \
  -DBUILD_TESTING=OFF \
  -DBUILD_QCH=OFF \
  -DKDE_INSTALL_QMLDIR=lib/qt5/qml \
  -DKDE_INSTALL_PLUGINDIR=lib/qt5/plugins \
  -DCMAKE_BUILD_TYPE=${configuration} ..
make -j2
make install
cd ../..

cd ..
rm -rf deps
