#!/usr/bin/env bash

NC='\033[0m'
YELLOW='\033[0;33m'

# Build desktop icon
echo "Creating desktop icon PNG..."
if command -v "inkscape" &> /dev/null; then
  inkscape -z -w 256 -h 256 icons/application/scalable/apps/keepassxc.svg -e icons/application/256x256/apps/keepassxc.png
else
  echo -e "${YELLOW}Could not find inkscape; keepassxc.png not built!${NC}"
fi

# Minify SVG's
echo "Minifying SVG's..."
minify -o icons/badges --match=.svg icons/badges
minify -o icons/database --match=.svg icons/database

# Crush PNG's
echo "Crushing PNG's..."
find . -iname '*.png' -exec pngcrush -ow -brute {} \;
