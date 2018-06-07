#!/usr/bin/bash

# Regarding the caching of files, see: https://github.com/msys2/msys2/wiki/MSYS2-reinstallation

pacman -Sy --noconfirm

pacman --needed --noconfirm -S \
    mingw-w64-$(uname -m)-cmake \
    mingw-w64-$(uname -m)-clang

pacman --needed --noconfirm -S \
    mingw-w64-$(uname -m)-libgcrypt \
    mingw-w64-$(uname -m)-zlib \
    mingw-w64-$(uname -m)-libsodium

pacman --needed --noconfirm -S \
    mingw-w64-$(uname -m)-qt5
