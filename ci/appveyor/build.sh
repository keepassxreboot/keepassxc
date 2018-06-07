#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/QRencode/lib:$PATH"
export PATH="/c/Program\ Files\ (x86)/qhttpengine/bin:$PATH"
export PATH="/c/Yubikey/lib:/c/Argon2/lib:$PATH"
export PATH="/c/Yubikey/include:/c/Argon2/include:$PATH"

cd /c/projects/keepassxc/build

make -j2 package;
