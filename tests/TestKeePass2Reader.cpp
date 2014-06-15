/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestKeePass2Reader.h"

#include <QTest>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestKeePass2Reader)

void TestKeePass2Reader::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestKeePass2Reader::testNonAscii()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/NonAscii.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(QString::fromUtf8("\xce\x94\xc3\xb6\xd8\xb6")));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("NonAsciiTest"));
    QCOMPARE(db->compressionAlgo(), Database::CompressionNone);

    delete db;
}

void TestKeePass2Reader::testCompressed()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Compressed.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(""));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("Compressed"));
    QCOMPARE(db->compressionAlgo(), Database::CompressionGZip);

    delete db;
}

void TestKeePass2Reader::testProtectedStrings()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/ProtectedStrings.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("masterpw"));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("Protected Strings Test"));

    Entry* entry = db->rootGroup()->entries().at(0);

    QCOMPARE(entry->title(), QString("Sample Entry"));
    QCOMPARE(entry->username(), QString("Protected User Name"));
    QCOMPARE(entry->password(), QString("ProtectedPassword"));
    QCOMPARE(entry->attributes()->value("TestProtected"), QString("ABC"));
    QCOMPARE(entry->attributes()->value("TestUnprotected"), QString("DEF"));

    QVERIFY(db->metadata()->protectPassword());
    QVERIFY(entry->attributes()->isProtected("TestProtected"));
    QVERIFY(!entry->attributes()->isProtected("TestUnprotected"));

    delete db;
}

void TestKeePass2Reader::testBrokenHeaderHash()
{
    // The protected stream key has been modified in the header.
    // Make sure the database won't open.

    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/BrokenHeaderHash.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(""));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(!db);
    QVERIFY(reader.hasError());

    delete db;
}

void TestKeePass2Reader::testFormat200()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format200.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->name(), QString("Format200"));
    QVERIFY(!db->metadata()->protectTitle());
    QVERIFY(db->metadata()->protectUsername());
    QVERIFY(!db->metadata()->protectPassword());
    QVERIFY(db->metadata()->protectUrl());
    QVERIFY(!db->metadata()->protectNotes());

    QCOMPARE(db->rootGroup()->entries().size(), 1);
    Entry* entry = db->rootGroup()->entries().at(0);

    QCOMPARE(entry->title(), QString("Sample Entry"));
    QCOMPARE(entry->username(), QString("User Name"));
    QCOMPARE(entry->attachments()->keys().size(), 2);
    QCOMPARE(entry->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));
    QCOMPARE(entry->attachments()->value("test.txt"), QByteArray("this is a test"));

    QCOMPARE(entry->historyItems().size(), 2);
    QCOMPARE(entry->historyItems().at(0)->attachments()->keys().size(), 0);
    QCOMPARE(entry->historyItems().at(1)->attachments()->keys().size(), 1);
    QCOMPARE(entry->historyItems().at(1)->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));

    delete db;
}

void TestKeePass2Reader::testFormat300()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format300.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    Database* db = reader.readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->name(), QString("Format300"));
    QCOMPARE(db->metadata()->name(), QString("Test Database Format 0x00030000"));

    delete db;
}
