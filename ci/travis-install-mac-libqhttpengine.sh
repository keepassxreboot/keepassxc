#!/bin/sh

set -ex

curl -O -J -L https://github.com/nitroshare/qhttpengine/archive/1.0.0.tar.gz
tar -xf qhttpengine-1.0.0.tar.gz
cd qhttpengine-1.0.0
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=${CONFIG} -DCMAKE_PREFIX_PATH=`find /usr/local/Cellar/qt/*/lib -type d | grep cmake$`
make -j2
make install
