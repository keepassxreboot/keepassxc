#!/usr/bin/bash

export PATH="/mingw64/bin:$PATH"
export PATH="/mingw64/lib:$PATH"

cd /c/projects/keepassxc/build

qmake --version
make test ARGS+="-E testgui --output-on-failure --verbose"
make test ARGS+="-R testgui --output-on-failure --verbose"
