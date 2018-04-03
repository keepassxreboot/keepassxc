/*
 *  Copyright (C) 2017 Vladimir Svyatski <v.unreal@gmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "TestDatabase.h"
#include "TestGlobal.h"

#include <QSignalSpy>
#include <QTemporaryFile>

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestDatabase)

void TestDatabase::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestDatabase::testEmptyRecycleBinOnDisabled()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinDisabled.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("123"));
    Database* db = Database::openDatabaseFile(filename, key);
    QVERIFY(db);

    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QCOMPARE(spyModified.count(), 0);

    delete db;
}

void TestDatabase::testEmptyRecycleBinOnNotCreated()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinNotYetCreated.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("123"));
    Database* db = Database::openDatabaseFile(filename, key);
    QVERIFY(db);

    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QCOMPARE(spyModified.count(), 0);

    delete db;
}

void TestDatabase::testEmptyRecycleBinOnEmpty()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinEmpty.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("123"));
    Database* db = Database::openDatabaseFile(filename, key);
    QVERIFY(db);

    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QCOMPARE(spyModified.count(), 0);

    delete db;
}

void TestDatabase::testEmptyRecycleBinWithHierarchicalData()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinWithData.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("123"));
    Database* db = Database::openDatabaseFile(filename, key);
    QVERIFY(db);

    QFile originalFile(filename);
    qint64 initialSize = originalFile.size();

    db->emptyRecycleBin();
    QVERIFY(db->metadata()->recycleBin());
    QVERIFY(db->metadata()->recycleBin()->entries().empty());
    QVERIFY(db->metadata()->recycleBin()->children().empty());

    QTemporaryFile afterCleanup;
    KeePass2Writer writer;
    writer.writeDatabase(&afterCleanup, db);
    QVERIFY(afterCleanup.size() < initialSize);

    delete db;
}
