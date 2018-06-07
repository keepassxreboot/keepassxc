#!/usr/bin/bash

export PATH="/mingw64/bin:/usr/bin:/usr/local/bin:$PATH"
export PATH="/mingw64/lib:/usr/lib:/usr/local/lib:$PATH"

if [ -d "/c/Yubikey" ]; then
    echo "CACHE: using cached copy of ykpers."
    exit 0;
fi

mkdir tmp && cd tmp
curl -O -J -L https://developers.yubico.com/yubikey-personalization/Releases/ykpers-1.18.1-win64.zip
mkdir -p "/c/Yubikey"
7z x ykpers-1.18.1-win64.zip -o"/c/Yubikey"

cd ..
rm -rf tmp
