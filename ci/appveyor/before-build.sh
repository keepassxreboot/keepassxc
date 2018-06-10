#!/usr/bin/bash

cd /c/projects/keepassxc
mkdir build && cd build

if [ "${configuration}" = "Debug" ]; then
  cmake -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=${configuration} \
    -DWITH_XC_ALL=ON \
    -DWITH_TESTS=ON \
    -DWITH_GUI_TESTS=ON \
    -DWITH_ASAN=ON \
    ..
else
  cmake -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=${configuration} \
    -DWITH_XC_ALL=ON \
    -DWITH_TESTS=ON \
    -DWITH_GUI_TESTS=ON \
    ..
fi
