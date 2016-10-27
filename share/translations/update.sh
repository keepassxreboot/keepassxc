#!/bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR/../..

echo Updating source file
lupdate-qt5 -no-ui-lines -disable-heuristic similartext -locations none -no-obsolete src -ts share/translations/keepassx_en.ts
lupdate-qt5 -no-ui-lines -disable-heuristic similartext -locations none -pluralonly src -ts share/translations/keepassx_en_plurals.ts

echo
echo Pushing English translation file to Transifex
tx push -s

echo
echo Pulling translations from Transifex
tx pull -a --minimum-perc=80
