/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTCLI_H
#define KEEPASSXC_TESTCLI_H

#include "core/Database.h"
#include "util/TemporaryFile.h"

#include <QByteArray>
#include <QFile>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QTemporaryFile>
#include <QTest>

#include <stdio.h>

class TestCli : public QObject
{
    Q_OBJECT

private:
    QSharedPointer<Database> readTestDatabase() const;

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testBatchCommands();
    void testAdd();
    void testAddGroup();
    void testAnalyze();
    void testClip();
    void testCommandParsing_data();
    void testCommandParsing();
    void testCreate();
    void testDiceware();
    void testEdit();
    void testEstimate_data();
    void testEstimate();
    void testExport();
    void testGenerate_data();
    void testGenerate();
    void testImport();
    void testKeyFileOption();
    void testNoPasswordOption();
    void testHelp();
    void testInteractiveCommands();
    void testList();
    void testLocate();
    void testMerge();
    void testMove();
    void testOpen();
    void testRemove();
    void testRemoveGroup();
    void testRemoveQuiet();
    void testShow();
    void testInvalidDbFiles();
    void testYubiKeyOption();

private:
    QByteArray m_dbData;
    QByteArray m_dbData2;
    QByteArray m_xmlData;
    QByteArray m_yubiKeyProtectedDbData;
    QByteArray m_keyFileProtectedDbData;
    QByteArray m_keyFileProtectedNoPasswordDbData;
    QScopedPointer<TemporaryFile> m_dbFile;
    QScopedPointer<TemporaryFile> m_dbFile2;
    QScopedPointer<TemporaryFile> m_xmlFile;
    QScopedPointer<TemporaryFile> m_keyFileProtectedDbFile;
    QScopedPointer<TemporaryFile> m_keyFileProtectedNoPasswordDbFile;
    QScopedPointer<TemporaryFile> m_yubiKeyProtectedDbFile;
    QScopedPointer<TemporaryFile> m_stdoutFile;
    QScopedPointer<TemporaryFile> m_stderrFile;
    QScopedPointer<TemporaryFile> m_stdinFile;
};

#endif // KEEPASSXC_TESTCLI_H
