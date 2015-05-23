#!/bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR/../..

echo Updating source file
lupdate -no-ui-lines -disable-heuristic similartext -locations none -no-obsolete src -ts share/translations/keepassx_en.ts
lupdate -no-ui-lines -disable-heuristic similartext -locations none -pluralonly src -ts share/translations/keepassx_en_plurals.ts

echo Pulling translations from Transifex
tx pull -a --minimum-perc=80
