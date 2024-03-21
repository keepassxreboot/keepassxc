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

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"
#include <QTest>

QTEST_GUILESS_MAIN(TestKdbx3)

void TestKdbx3::initTestCaseImpl()
{
    m_xmlDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX3)));
    m_kdbxSourceDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX3)));
}

QSharedPointer<Database> TestKdbx3::readXml(const QString& path, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3_1);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(path);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

QSharedPointer<Database> TestKdbx3::readXml(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString)
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
                         QSharedPointer<const CompositeKey> key,
                         QSharedPointer<Database> db,
                         bool& hasError,
                         QString& errorString)
{
    KeePass2Reader reader;
    reader.readDatabase(device, key, db.data());
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3_1);
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
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    KeePass2Reader reader;
    auto db = QSharedPointer<Database>::create();
    QVERIFY(reader.readDatabase(filename, key, db.data()));
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_3);
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->name(), QString("Format300"));
    QCOMPARE(db->metadata()->name(), QString("Test Database Format 0x00030000"));
}

void TestKdbx3::testNonAscii()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/NonAscii.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(QString::fromUtf8("\xce\x94\xc3\xb6\xd8\xb6")));
    KeePass2Reader reader;
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("NonAsciiTest"));
    QCOMPARE(db->compressionAlgorithm(), Database::CompressionNone);
}

void TestKdbx3::testCompressed()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Compressed.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(""));
    KeePass2Reader reader;
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("Compressed"));
    QCOMPARE(db->compressionAlgorithm(), Database::CompressionGZip);
}

void TestKdbx3::testProtectedStrings()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/ProtectedStrings.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("masterpw"));
    KeePass2Reader reader;
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));
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
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(""));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(!db->open(filename, key, nullptr));
}

void TestKdbx3::testAttachmentIndexStability()
{
    QScopedPointer<Database> db(new Database());
    db->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX3)));
    auto compositeKey = QSharedPointer<CompositeKey>::create();
    db->setKey(compositeKey);
    QVERIFY(!db->uuid().isNull());

    auto root = db->rootGroup();

    auto group1 = new Group();
    group1->setUuid(QUuid::createUuid());
    QVERIFY(!group1->uuid().isNull());
    group1->setParent(root);

    auto attachment1 = QByteArray("qwerty");
    auto attachment2 = QByteArray("asdf");
    auto attachment3 = QByteArray("zxcv");

    auto entry1 = new Entry();
    entry1->setUuid(QUuid::createUuid());
    QVERIFY(!entry1->uuid().isNull());
    auto uuid1 = entry1->uuid();
    entry1->attachments()->set("a", attachment1);
    QCOMPARE(entry1->attachments()->keys().size(), 1);
    QCOMPARE(entry1->attachments()->values().size(), 1);
    entry1->setGroup(root);

    auto entry2 = new Entry();
    entry2->setUuid(QUuid::createUuid());
    QVERIFY(!entry2->uuid().isNull());
    auto uuid2 = entry2->uuid();
    entry2->attachments()->set("a", attachment1);
    entry2->attachments()->set("b", attachment2);
    QCOMPARE(entry2->attachments()->keys().size(), 2);
    QCOMPARE(entry2->attachments()->values().size(), 2);
    entry2->setGroup(group1);

    auto entry3 = new Entry();
    entry3->setUuid(QUuid::createUuid());
    QVERIFY(!entry3->uuid().isNull());
    auto uuid3 = entry3->uuid();
    entry3->attachments()->set("a", attachment1);
    entry3->attachments()->set("b", attachment2);
    entry3->attachments()->set("x", attachment3);
    entry3->attachments()->set("y", attachment3);
    QCOMPARE(entry3->attachments()->keys().size(), 4);
    QCOMPARE(entry3->attachments()->values().size(), 3);
    entry3->setGroup(group1);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    KeePass2Writer writer;
    QVERIFY(writer.writeDatabase(&buffer, db.data()));
    QCOMPARE(writer.version(), KeePass2::FILE_VERSION_3_1);

    buffer.seek(0);
    KeePass2Reader reader;

    // Re-read database and check that all attachments are still correctly assigned
    auto db2 = QSharedPointer<Database>::create();
    reader.readDatabase(&buffer, QSharedPointer<CompositeKey>::create(), db2.data());
    QVERIFY(!reader.hasError());
    QVERIFY(!db->uuid().isNull());

    auto a1 = db2->rootGroup()->findEntryByUuid(uuid1)->attachments();
    QCOMPARE(a1->keys().size(), 1);
    QCOMPARE(a1->values().size(), 1);
    QCOMPARE(a1->value("a"), attachment1);

    auto a2 = db2->rootGroup()->findEntryByUuid(uuid2)->attachments();
    QCOMPARE(a2->keys().size(), 2);
    QCOMPARE(a2->values().size(), 2);
    QCOMPARE(a2->value("a"), attachment1);
    QCOMPARE(a2->value("b"), attachment2);

    auto a3 = db2->rootGroup()->findEntryByUuid(uuid3)->attachments();
    QCOMPARE(a3->keys().size(), 4);
    QCOMPARE(a3->values().size(), 3);
    QCOMPARE(a3->value("a"), attachment1);
    QCOMPARE(a3->value("b"), attachment2);
    QCOMPARE(a3->value("x"), attachment3);
    QCOMPARE(a3->value("y"), attachment3);
}
