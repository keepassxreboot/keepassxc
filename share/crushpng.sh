#!/usr/bin/env bash

if [[ -z $1 ]]; then
  echo "You must supply a root folder!"
  exit 1
fi

find "$1" -iname '*png' -exec pngcrush -ow -brute {} \;