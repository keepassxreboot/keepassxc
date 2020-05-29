#!/usr/bin/env bash

NC='\033[0m'
RED='\033[0;31m'
YELLOW='\033[0;33m'

if [[ -z "$1" ]]; then
  echo -e "${RED}You must include an SVG file to convert!${NC}"
  exit 1
fi

outfile=$2
if [[ -z "outfile" ]]; then
  outfile="logo.ico"
fi

if ! command -v "inkscape" &> /dev/null; then
  echo -e "${YELLOW}Could not find inkscape; $outfile not built!${NC}"
  exit 0
fi

echo "Generating $outfile from $1..."
size_list=(16 24 32 48 64 128 256)
for size in ${size_list[@]}; do
    inkscape -z -e $size.png -w $size -h $size "$1" >/dev/null 2>/dev/null
done

images=`printf "%s.png " "${size_list[@]}"`
convert $images $outfile

rm $images
