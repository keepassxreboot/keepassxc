#!/bin/sh

set -ex

cd build

if [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${CC}" = "clang" ] && [ ! -z ${DOCKER_IMG} ]; then
  export CC=clang-5.0;
  export CXX=clang++-5.0;
fi

if [ "${CONFIG}" = "Debug" ]; then
  cmake ${CMAKE_ARG} \
    -DCMAKE_BUILD_TYPE=${CONFIG} \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_YUBIKEY=ON \
    -DWITH_GUI_TESTS=ON \
    -DWITH_ASAN=ON \
    ..
else
  cmake ${CMAKE_ARG} \
    -DCMAKE_BUILD_TYPE=${CONFIG} \
    -DWITH_XC_HTTP=ON \
    -DWITH_XC_AUTOTYPE=ON \
    -DWITH_XC_YUBIKEY=ON \
    ..
fi

if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  make -j2 package;
else
  make -j2;
fi
