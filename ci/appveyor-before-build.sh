#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin:$PATH"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/KArchive/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/qhttpengine/lib:$PATH"

cd /c/projects/keepassxc
mkdir build && cd build

if [ "${configuration}" = "Debug" ]; then
  cmake -G "MSYS Makefiles" \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_YUBIKEY=ON \
    -DWITH_ASAN=ON \
    -DCMAKE_BUILD_TYPE=${configuration} ..
else
  cmake -G "MSYS Makefiles" \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_YUBIKEY=ON \
    -DCMAKE_BUILD_TYPE=${configuration} ..
fi
