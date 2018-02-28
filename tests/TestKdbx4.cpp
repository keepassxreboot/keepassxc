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

#include "TestKdbx4.h"
#include "TestGlobal.h"

#include "core/Metadata.h"
#include "keys/PasswordKey.h"
#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "config-keepassx-tests.h"


QTEST_GUILESS_MAIN(TestKdbx4)

void TestKdbx4::initTestCaseImpl()
{
    m_xmlDb->changeKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2));
    m_kdbxSourceDb->changeKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2));
}

Database* TestKdbx4::readXml(const QString& path, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(path);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

Database* TestKdbx4::readXml(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(buf);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

void TestKdbx4::writeXml(QBuffer* buf, Database* db, bool& hasError, QString& errorString)
{
    KdbxXmlWriter writer(KeePass2::FILE_VERSION_4);
    writer.writeDatabase(buf, db);
    hasError = writer.hasError();
    errorString = writer.errorString();
}

void TestKdbx4::readKdbx(QIODevice* device, CompositeKey const& key, QScopedPointer<Database>& db,
                         bool& hasError, QString& errorString)
{
    KeePass2Reader reader;
    db.reset(reader.readDatabase(device, key));
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_4);
}

void TestKdbx4::readKdbx(const QString& path, CompositeKey const& key, QScopedPointer<Database>& db,
                         bool& hasError, QString& errorString)
{
    KeePass2Reader reader;
    db.reset(reader.readDatabase(path, key));
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_4);
}

void TestKdbx4::writeKdbx(QIODevice* device, Database* db, bool& hasError, QString& errorString)
{
    if (db->kdf()->uuid() == KeePass2::KDF_AES_KDBX3) {
        db->changeKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2));
    }
    KeePass2Writer writer;
    hasError = writer.writeDatabase(device, db);
    hasError = writer.hasError();
    if (hasError) {
        errorString = writer.errorString();
    }
    QCOMPARE(writer.version(), KeePass2::FILE_VERSION_4);
}

Q_DECLARE_METATYPE(Uuid);
void TestKdbx4::testFormat400()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format400.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey("t"));
    KeePass2Reader reader;
    QScopedPointer<Database> db(reader.readDatabase(filename, key));
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_4);
    QVERIFY(db.data());
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->name(), QString("Format400"));
    QCOMPARE(db->metadata()->name(), QString("Format400"));
    QCOMPARE(db->rootGroup()->entries().size(), 1);
    auto entry = db->rootGroup()->entries().at(0);

    QCOMPARE(entry->title(), QString("Format400"));
    QCOMPARE(entry->username(), QString("Format400"));
    QCOMPARE(entry->attributes()->keys().size(), 6);
    QCOMPARE(entry->attributes()->value("Format400"), QString("Format400"));
    QCOMPARE(entry->attachments()->keys().size(), 1);
    QCOMPARE(entry->attachments()->value("Format400"), QByteArray("Format400\n"));
}

void TestKdbx4::testFormat400Upgrade()
{
    QFETCH(Uuid, kdfUuid);
    QFETCH(Uuid, cipherUuid);
    QFETCH(bool, addCustomData);
    QFETCH(quint32, expectedVersion);

    QScopedPointer<Database> sourceDb(new Database());
    sourceDb->metadata()->setName("Wubba lubba dub dub");
    QCOMPARE(sourceDb->kdf()->uuid(), KeePass2::KDF_AES_KDBX3);    // default is legacy AES-KDF

    CompositeKey key;
    key.addKey(PasswordKey("I am in great pain, please help me!"));
    sourceDb->setKey(key, true, true);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    // upgrade to KDBX 4 by changing KDF and Cipher
    sourceDb->changeKdf(KeePass2::uuidToKdf(kdfUuid));
    sourceDb->setCipher(cipherUuid);

    // CustomData in meta should not cause any version change
    sourceDb->metadata()->customData()->set("CustomPublicData", "Hey look, I turned myself into a pickle!");
    if (addCustomData) {
        // this, however, should
        sourceDb->rootGroup()->customData()->set("CustomGroupData", "I just killed my family! I don't care who they were!");
    }

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, sourceDb.data());
    if (writer.hasError()) {
        QFAIL(qPrintable(QString("Error while writing database: %1").arg(writer.errorString())));
    }

    // read buffer back
    buffer.seek(0);
    KeePass2Reader reader;
    QScopedPointer<Database> targetDb(reader.readDatabase(&buffer, key));
    if (reader.hasError()) {
        QFAIL(qPrintable(QString("Error while reading database: %1").arg(reader.errorString())));
    }

    QVERIFY(targetDb->rootGroup());
    QCOMPARE(targetDb->metadata()->name(), sourceDb->metadata()->name());

    QCOMPARE(reader.version(), expectedVersion);
    QCOMPARE(targetDb->cipher(), cipherUuid);
    QCOMPARE(*targetDb->metadata()->customData(), *sourceDb->metadata()->customData());
    QCOMPARE(*targetDb->rootGroup()->customData(), *sourceDb->rootGroup()->customData());
}

void TestKdbx4::testFormat400Upgrade_data()
{
    QTest::addColumn<Uuid>("kdfUuid");
    QTest::addColumn<Uuid>("cipherUuid");
    QTest::addColumn<bool>("addCustomData");
    QTest::addColumn<quint32>("expectedVersion");

    auto constexpr kdbx3 = KeePass2::FILE_VERSION_3_1 & KeePass2::FILE_VERSION_CRITICAL_MASK;
    auto constexpr kdbx4 = KeePass2::FILE_VERSION_4   & KeePass2::FILE_VERSION_CRITICAL_MASK;

    QTest::newRow("Argon2           + AES")                   << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_AES       << false << kdbx4;
    QTest::newRow("AES-KDF          + AES")                   << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_AES       << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + AES")                   << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_AES       << false << kdbx3;
    QTest::newRow("Argon2           + AES     + CustomData")  << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_AES       << true  << kdbx4;
    QTest::newRow("AES-KDF          + AES     + CustomData")  << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_AES       << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + AES     + CustomData")  << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_AES       << true  << kdbx4;

    QTest::newRow("Argon2           + ChaCha20")              << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_CHACHA20  << false << kdbx4;
    QTest::newRow("AES-KDF          + ChaCha20")              << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_CHACHA20  << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + ChaCha20")              << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_CHACHA20  << false << kdbx3;
    QTest::newRow("Argon2           + ChaCha20 + CustomData") << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_CHACHA20  << true  << kdbx4;
    QTest::newRow("AES-KDF          + ChaCha20 + CustomData") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_CHACHA20  << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + ChaCha20 + CustomData") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_CHACHA20  << true  << kdbx4;

    QTest::newRow("Argon2           + Twofish")               << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_TWOFISH   << false << kdbx4;
    QTest::newRow("AES-KDF          + Twofish")               << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_TWOFISH   << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + Twofish")               << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_TWOFISH   << false << kdbx3;
    QTest::newRow("Argon2           + Twofish  + CustomData") << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_TWOFISH   << true  << kdbx4;
    QTest::newRow("AES-KDF          + Twofish  + CustomData") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_TWOFISH   << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + Twofish  + CustomData") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_TWOFISH   << true  << kdbx4;
}

void TestKdbx4::testCustomData()
{
    Database db;

    // test public custom data
    QVariantMap publicCustomData;
    publicCustomData.insert("CD1", 123);
    publicCustomData.insert("CD2", true);
    publicCustomData.insert("CD3", "abcäöü");
    publicCustomData.insert("CD4", QByteArray::fromHex("ababa123ff"));
    db.setPublicCustomData(publicCustomData);
    QCOMPARE(db.publicCustomData(), publicCustomData);

    const QString customDataKey1 = "CD1";
    const QString customDataKey2 = "CD2";
    const QString customData1 = "abcäöü";
    const QString customData2 = "Hello World";
    const int dataSize = customDataKey1.toUtf8().size() + customDataKey1.toUtf8().size() +
            customData1.toUtf8().size() + customData2.toUtf8().size();

    // test custom database data
    db.metadata()->customData()->set(customDataKey1, customData1);
    db.metadata()->customData()->set(customDataKey2, customData2);
    QCOMPARE(db.metadata()->customData()->size(), 2);
    QCOMPARE(db.metadata()->customData()->dataSize(), dataSize);

    // test custom root group data
    Group* root = db.rootGroup();
    root->customData()->set(customDataKey1, customData1);
    root->customData()->set(customDataKey2, customData2);
    QCOMPARE(root->customData()->size(), 2);
    QCOMPARE(root->customData()->dataSize(), dataSize);

    // test copied custom group data
    auto* group = new Group();
    group->setParent(root);
    group->setUuid(Uuid::random());
    group->customData()->copyDataFrom(root->customData());
    QCOMPARE(*group->customData(), *root->customData());

    // test copied custom entry data
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setUuid(Uuid::random());
    entry->customData()->copyDataFrom(group->customData());
    QCOMPARE(*entry->customData(), *root->customData());

    // test custom data deletion
    entry->customData()->set("additional item", "foobar");
    QCOMPARE(entry->customData()->size(), 3);
    entry->customData()->remove("additional item");
    QCOMPARE(entry->customData()->size(), 2);
    QCOMPARE(entry->customData()->dataSize(), dataSize);

    // test custom data on cloned groups
    QScopedPointer<Group> clonedGroup(group->clone());
    QCOMPARE(*clonedGroup->customData(), *group->customData());

    // test custom data on cloned entries
    QScopedPointer<Entry> clonedEntry(entry->clone(Entry::CloneNoFlags));
    QCOMPARE(*clonedEntry->customData(), *entry->customData());

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    KeePass2Writer writer;
    writer.writeDatabase(&buffer, &db);

    // read buffer back
    buffer.seek(0);
    KeePass2Reader reader;
    QSharedPointer<Database> newDb(reader.readDatabase(&buffer, CompositeKey()));

    // test all custom data are read back successfully from KDBX
    QCOMPARE(newDb->publicCustomData(), publicCustomData);

    QCOMPARE(newDb->metadata()->customData()->value(customDataKey1), customData1);
    QCOMPARE(newDb->metadata()->customData()->value(customDataKey2), customData2);

    QCOMPARE(newDb->rootGroup()->customData()->value(customDataKey1), customData1);
    QCOMPARE(newDb->rootGroup()->customData()->value(customDataKey2), customData2);

    auto* newGroup = newDb->rootGroup()->children()[0];
    QCOMPARE(newGroup->customData()->value(customDataKey1), customData1);
    QCOMPARE(newGroup->customData()->value(customDataKey2), customData2);

    auto* newEntry = newDb->rootGroup()->children()[0]->entries()[0];
    QCOMPARE(newEntry->customData()->value(customDataKey1), customData1);
    QCOMPARE(newEntry->customData()->value(customDataKey2), customData2);
}
