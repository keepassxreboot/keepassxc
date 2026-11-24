/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_TESTCSVIMPORT_H
#define KEEPASSXC_TESTCSVIMPORT_H

#include <QObject>

// the CSV import (the import form, run offscreen) against files written by the CSV export
class TestCsvImport : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // What the export writes comes back
    void testEveryEntryFieldSurvivesTheCsvRoundTrip();
    void testTrickyText_data();
    void testTrickyText();
    void testHalfAnEmojiInEveryTextColumn();
    void testExactTextColumnStaysEmptyForOrdinaryText();
    void testWhichCharactersTheJsonStepKeeps_data();
    void testWhichCharactersTheJsonStepKeeps();
};

#endif // KEEPASSXC_TESTCSVIMPORT_H
