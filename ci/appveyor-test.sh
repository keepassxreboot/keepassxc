#!/usr/bin/bash

export PATH="/mingw64/bin:$PATH"
export PATH="/mingw64/lib:$PATH"

cd /c/projects/keepassxc/build

make test;
