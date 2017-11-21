#!/bin/sh

set -ex

cd build

make test ARGS+="-E testgui --output-on-failure"

if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  xvfb-run -a --server-args="-screen 0 800x600x24" make test ARGS+="-R testgui --output-on-failure";
fi
