#!/usr/bin/bash

cd /c/projects/keepassxc/build

make test ARGS+="-E testgui --output-on-failure --verbose"
make test ARGS+="-R testgui --output-on-failure --verbose"
