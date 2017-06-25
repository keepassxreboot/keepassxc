#!/bin/bash

# Canonical path to qt5 directory
QT="/usr/local/Cellar/qt"
if [ ! -d "$QT" ]; then
    # Alternative (old) path to qt5 directory
    QT+="5"
    if [ ! -d "$QT" ]; then
        echo "Qt/Qt5 not found!"
        exit
    fi
fi
QT5_DIR="$QT/$(ls $QT | sort -r | head -n1)"
echo $QT5_DIR

# Change qt5 framework ids
for framework in $(find "$QT5_DIR/lib" -regex ".*/\(Qt[a-zA-Z]*\)\.framework/Versions/5/\1"); do
    echo "$framework"
    install_name_tool -id "$framework" "$framework"
done
