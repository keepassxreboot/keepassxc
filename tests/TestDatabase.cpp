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

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestDatabase)

void TestDatabase::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestDatabase::testOpen()
{
    auto db = QSharedPointer<Database>::create();
    QVERIFY(!db->isInitialized());
    QVERIFY(!db->isModified());

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    bool ok = db->open(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"), key);
    QVERIFY(ok);

    QVERIFY(db->isInitialized());
    QVERIFY(!db->isModified());

    db->metadata()->setName("test");
    QVERIFY(db->isModified());
}

void TestDatabase::testSave()
{
    QByteArray data;
    QFile sourceDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, data));
    sourceDbFile.close();

    TemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QCOMPARE(tempFile.write(data), static_cast<qint64>((data.size())));
    tempFile.close();

    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    QString error;
    bool ok = db->open(tempFile.fileName(), key, &error);
    QVERIFY(ok);

    // Test safe saves
    db->metadata()->setName("test");
    QVERIFY(db->isModified());

    // Test unsafe saves
    QVERIFY2(db->save(&error, false, false), error.toLatin1());

    QVERIFY2(db->save(&error), error.toLatin1());
    QVERIFY(!db->isModified());

    // Test save backups
    QVERIFY2(db->save(&error, true, true), error.toLatin1());
}

void TestDatabase::testEmptyRecycleBinOnDisabled()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinDisabled.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr, false));

    // Explicitly mark DB as read-write in case it was opened from a read-only drive.
    // Prevents assertion failures on CI systems when the data dir is not writable
    db->setReadOnly(false);

    QSignalSpy spyModified(db.data(), SIGNAL(databaseModified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinOnNotCreated()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinNotYetCreated.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr, false));
    db->setReadOnly(false);

    QSignalSpy spyModified(db.data(), SIGNAL(databaseModified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinOnEmpty()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinEmpty.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr, false));
    db->setReadOnly(false);

    QSignalSpy spyModified(db.data(), SIGNAL(databaseModified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinWithHierarchicalData()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinWithData.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr, false));
    db->setReadOnly(false);

    QFile originalFile(filename);
    qint64 initialSize = originalFile.size();

    db->emptyRecycleBin();
    QVERIFY(db->metadata()->recycleBin());
    QVERIFY(db->metadata()->recycleBin()->entries().empty());
    QVERIFY(db->metadata()->recycleBin()->children().empty());

    QTemporaryFile afterCleanup;
    afterCleanup.open();

    KeePass2Writer writer;
    writer.writeDatabase(&afterCleanup, db.data());
    QVERIFY(afterCleanup.size() < initialSize);
}
