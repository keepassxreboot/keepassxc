#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"

if [ -d "/c/Program\ Files\ (x86)/QRencode" ]; then
    echo "CACHE: using cached copy of libqrencode."
    exit 0;
fi

mkdir tmp && cd tmp
curl -O -J -L https://fukuchi.org/works/qrencode/qrencode-4.0.0.tar.gz
tar -xf qrencode-4.0.0.tar.gz

cd qrencode-4.0.0
mkdir build && cd build
cmake -G "MSYS Makefiles" -DBUILD_SHARED_LIBS=YES -DWITH_TOOLS=NO -DCMAKE_BUILD_TYPE=${configuration} ..
make -j2
make install
cd ../..

cd ..
rm -rf tmp
