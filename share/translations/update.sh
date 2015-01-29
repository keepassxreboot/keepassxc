#!/bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR/../..

lupdate -no-ui-lines -disable-heuristic similartext -locations none -no-obsolete src -ts share/translations/keepassx_en.ts
lupdate -no-ui-lines -disable-heuristic similartext -locations none -pluralonly src -ts share/translations/keepassx_en_plurals.ts
