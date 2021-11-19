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

#include "TestKdbx2.h"

#include "config-keepassx-tests.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"

#include <QBuffer>
#include <QTest>

QTEST_GUILESS_MAIN(TestKdbx2)

void TestKdbx2::initTestCase()
{
    QVERIFY(Crypto::init());
}

/**
 * Helper method for verifying contents of the sample KDBX 2 file.
 */
void TestKdbx2::verifyKdbx2Db(QSharedPointer<Database> db)
{
    QVERIFY(db);
    QCOMPARE(db->rootGroup()->name(), QString("Format200"));
    QVERIFY(!db->metadata()->protectTitle());
    QVERIFY(db->metadata()->protectUsername());
    QVERIFY(!db->metadata()->protectPassword());
    QVERIFY(db->metadata()->protectUrl());
    QVERIFY(!db->metadata()->protectNotes());

    QCOMPARE(db->rootGroup()->entries().size(), 1);
    auto entry = db->rootGroup()->entries().at(0);

    QCOMPARE(entry->title(), QString("Sample Entry"));
    QCOMPARE(entry->username(), QString("User Name"));
    QCOMPARE(entry->attachments()->keys().size(), 2);
    QCOMPARE(entry->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));
    QCOMPARE(entry->attachments()->value("test.txt"), QByteArray("this is a test"));

    QCOMPARE(entry->historyItems().size(), 2);
    QCOMPARE(entry->historyItems().at(0)->attachments()->keys().size(), 0);
    QCOMPARE(entry->historyItems().at(1)->attachments()->keys().size(), 1);
    QCOMPARE(entry->historyItems().at(1)->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));
}

void TestKdbx2::testFormat200()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format200.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    auto db = QSharedPointer<Database>::create();
    KeePass2Reader reader;
    QVERIFY(reader.readDatabase(filename, key, db.data()));
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_2 & KeePass2::FILE_VERSION_CRITICAL_MASK);

    QVERIFY2(!reader.hasError(), reader.errorString().toStdString().c_str());
    verifyKdbx2Db(db);
}

void TestKdbx2::testFormat200Upgrade()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format200.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    auto db = QSharedPointer<Database>::create();
    KeePass2Reader reader;
    reader.readDatabase(filename, key, db.data());
    QVERIFY2(!reader.hasError(), reader.errorString().toStdString().c_str());
    QVERIFY(!db.isNull());
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_2);
    QCOMPARE(db->kdf()->uuid(), KeePass2::KDF_AES_KDBX3);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    // write KDBX 3 to upgrade it
    KeePass2Writer writer;
    QVERIFY(writer.writeDatabase(&buffer, db.data()));
    if (writer.hasError()) {
        QFAIL(qPrintable(QString("Error while writing database: %1").arg(writer.errorString())));
    }

    // read buffer back
    buffer.seek(0);
    auto targetDb = QSharedPointer<Database>::create();
    QVERIFY(reader.readDatabase(&buffer, key, targetDb.data()));
    if (reader.hasError()) {
        QFAIL(qPrintable(QString("Error while reading database: %1").arg(reader.errorString())));
    }

    // database should now be upgraded to KDBX 3 without data loss
    verifyKdbx2Db(targetDb);
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3_1);
    QCOMPARE(targetDb->kdf()->uuid(), KeePass2::KDF_AES_KDBX3);
}
