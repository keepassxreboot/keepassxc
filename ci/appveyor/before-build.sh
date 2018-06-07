#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/QRencode/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/qhttpengine/bin:$PATH"
export PATH="/c/Yubikey/lib:/c/Argon2/lib:$PATH"
export PATH="/c/Yubikey/include:/c/Argon2/include:$PATH"

cd /c/projects/keepassxc
mkdir build && cd build

if [ "${configuration}" = "Debug" ]; then
  cmake -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=${configuration} \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_BROWSER=ON \
    -DWITH_XC_YUBIKEY=ON \
    -DWITH_XC_SSHAGENT=ON \
    -DWITH_TESTS=ON \
    -DWITH_GUI_TESTS=ON \
    -DWITH_ASAN=ON \
    ..
else
  cmake -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=${configuration} \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_BROWSER=ON \
    -DWITH_XC_YUBIKEY=ON \
    -DWITH_XC_SSHAGENT=ON \
    -DWITH_TESTS=ON \
    -DWITH_GUI_TESTS=ON \
    ..
fi
