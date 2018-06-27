#!/usr/bin/bash

# Pacman derived dependencies
pacman --needed --noconfirm -S \
    mingw-w64-$(uname -m)-cmake \
    mingw-w64-$(uname -m)-libgcrypt \
    mingw-w64-$(uname -m)-zlib \
    mingw-w64-$(uname -m)-libsodium \
    mingw-w64-$(uname -m)-argon2 \
    mingw-w64-$(uname -m)-qt5

# Yubikey library
curl -O -J -L https://developers.yubico.com/yubikey-personalization/Releases/ykpers-1.18.1-win64.zip
7z x ykpers-1.18.1-win64.zip -o"/mingw64/" -aoa

# qrencode library
curl -O -J -L https://fukuchi.org/works/qrencode/qrencode-4.0.0.tar.gz
tar -xf qrencode-4.0.0.tar.gz

cd qrencode-4.0.0
mkdir build && cd build
cmake -G "MSYS Makefiles" -DBUILD_SHARED_LIBS=YES -DWITH_TOOLS=NO -DCMAKE_BUILD_TYPE=${configuration} ..
make -j2
make install PREFIX="/mingw64"