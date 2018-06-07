#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"

export CC="gcc"
export CXX="g++"

if [ -d "/c/Argon2" ]; then
    echo "CACHE: using cached copy of libargon2."
    exit 0;
fi

mkdir tmp && cd tmp
curl -O -J -L https://github.com/P-H-C/phc-winner-argon2/archive/20171227.tar.gz
tar -xf phc-winner-argon2-20171227.tar.gz

mkdir -p "/c/Argon2"

cd phc-winner-argon2-20171227
make -j2
make install PREFIX="/c/Argon2"
cd ..

cd ..
rm -rf tmp
