#!/bin/bash

# Install xcode
# Install homebrew
# > brew install cmake qt5 libgcrypt

if cd build; then
    QT5_DIR="/usr/local/opt/qt"

    export MACOSX_DEPLOYMENT_TARGET=10.10
    cmake .. -DCMAKE_PREFIX_PATH=$QT5_DIR 
-DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
    make -j4
fi

