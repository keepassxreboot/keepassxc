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

#include "TestKdbx3.h"
#include "TestGlobal.h"

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Repair.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestKdbx3)

void TestKdbx3::initTestCaseImpl()
{
}

Database* TestKdbx3::readXml(const QString& path, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3_1);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(path);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

Database* TestKdbx3::readXml(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3_1);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(buf);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

void TestKdbx3::writeXml(QBuffer* buf, Database* db, bool& hasError, QString& errorString)
{
    KdbxXmlWriter writer(KeePass2::FILE_VERSION_3_1);
    writer.writeDatabase(buf, db);
    hasError = writer.hasError();
    errorString = writer.errorString();
}

void TestKdbx3::readKdbx(QIODevice* device,
                         CompositeKey const& key,
                         QScopedPointer<Database>& db,
                         bool& hasError,
                         QString& errorString)
{
    KeePass2Reader reader;
    db.reset(reader.readDatabase(device, key));
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3_1 & KeePass2::FILE_VERSION_CRITICAL_MASK);
}

void TestKdbx3::readKdbx(const QString& path,
                         CompositeKey const& key,
                         QScopedPointer<Database>& db,
                         bool& hasError,
                         QString& errorString)
{
    KeePass2Reader reader;
    db.reset(reader.readDatabase(path, key));
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3_1 & KeePass2::FILE_VERSION_CRITICAL_MASK);
}

void TestKdbx3::writeKdbx(QIODevice* device, Database* db, bool& hasError, QString& errorString)
{
    KeePass2Writer writer;
    hasError = writer.writeDatabase(device, db);
    hasError = writer.hasError();
    if (hasError) {
        errorString = writer.errorString();
    }
    QCOMPARE(writer.version(), KeePass2::FILE_VERSION_3_1);
}

void TestKdbx3::testFormat300()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format300.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3);
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->name(), QString("Format300"));
    QCOMPARE(db->metadata()->name(), QString("Test Database Format 0x00030000"));
}

void TestKdbx3::testNonAscii()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/NonAscii.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(QString::fromUtf8("\xce\x94\xc3\xb6\xd8\xb6")));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("NonAsciiTest"));
    QCOMPARE(db->compressionAlgo(), Database::CompressionNone);
}

void TestKdbx3::testCompressed()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Compressed.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(""));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("Compressed"));
    QCOMPARE(db->compressionAlgo(), Database::CompressionGZip);
}

void TestKdbx3::testProtectedStrings()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/ProtectedStrings.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("masterpw"));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QVERIFY(db.data());
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
}

void TestKdbx3::testBrokenHeaderHash()
{
    // The protected stream key has been modified in the header.
    // Make sure the database won't open.

    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/BrokenHeaderHash.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(""));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QVERIFY(!db.data());
    QVERIFY(reader.hasError());
}

void TestKdbx3::testKdbxRepair()
{
    QString brokenDbFilename = QString(KEEPASSX_TEST_DATA_DIR).append("/bug392.kdbx");
    // master password = test
    // entry username: testuser\x10\x20AC
    // entry password: testpw
    CompositeKey key;
    key.addKey(PasswordKey("test"));

    // test that we can't open the broken database
    bool hasError;
    QString errorString;
    QScopedPointer<Database> dbBroken;
    readKdbx(brokenDbFilename, key, dbBroken, hasError, errorString);
    QVERIFY(!dbBroken.data());
    QVERIFY(hasError);

    // test if we can repair the database
    KeePass2Repair repair;
    QFile file(brokenDbFilename);
    file.open(QIODevice::ReadOnly);
    auto result = repair.repairDatabase(&file, key);
    QCOMPARE(result.first, KeePass2Repair::RepairSuccess);
    QScopedPointer<Database> dbRepaired(result.second);
    QVERIFY(dbRepaired);

    QCOMPARE(dbRepaired->rootGroup()->entries().size(), 1);
    QCOMPARE(dbRepaired->rootGroup()->entries().at(0)->username(), QString("testuser").append(QChar(0x20AC)));
    QCOMPARE(dbRepaired->rootGroup()->entries().at(0)->password(), QString("testpw"));
}
