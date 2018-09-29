/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include <QTest>
#include <QTextStream>
#include <QFile>
#include <QScopedPointer>
#include <QTemporaryFile>

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

    void testCommand();
    void testAdd();
    void testClip();
    void testDiceware();
    void testEdit();
    void testEstimate_data();
    void testEstimate();
    void testExtract();
    void testGenerate_data();
    void testGenerate();
    void testList();
    void testLocate();
    void testMerge();
    void testRemove();
    void testShow();

private:
    QByteArray m_dbData;
    QScopedPointer<QTemporaryFile> m_dbFile;
    QScopedPointer<QTemporaryFile> m_stdoutFile;
    QScopedPointer<QTemporaryFile> m_stderrFile;
    QScopedPointer<QTemporaryFile> m_stdinFile;
    FILE* m_stdoutHandle = stdout;
    FILE* m_stderrHandle = stderr;
    FILE* m_stdinHandle = stdin;
};

#endif //KEEPASSXC_TESTCLI_H
