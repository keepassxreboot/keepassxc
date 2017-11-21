#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin:$PATH"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/KArchive/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/qhttpengine/lib:$PATH"

cd /c/projects/keepassxc/build

make -j2 package;
