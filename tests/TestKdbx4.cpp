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

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "mock/MockChallengeResponseKey.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_SET_MAIN_SOURCE_PATH

    TestKdbx4Argon2 argon2Test;
    TestKdbx4AesKdf aesKdfTest;
    return QTest::qExec(&argon2Test, argc, argv) | QTest::qExec(&aesKdfTest, argc, argv);
}

void TestKdbx4Argon2::initTestCaseImpl()
{
    m_xmlDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D)));
    m_kdbxSourceDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D)));
}

QSharedPointer<Database>
TestKdbx4Argon2::readXml(const QString& path, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(path);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

QSharedPointer<Database> TestKdbx4Argon2::readXml(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(buf);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

void TestKdbx4Argon2::writeXml(QBuffer* buf, Database* db, bool& hasError, QString& errorString)
{
    KdbxXmlWriter writer(KeePass2::FILE_VERSION_4);
    writer.writeDatabase(buf, db);
    hasError = writer.hasError();
    errorString = writer.errorString();
}

void TestKdbx4Argon2::readKdbx(QIODevice* device,
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
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_4);
}

void TestKdbx4Argon2::readKdbx(const QString& path,
                               QSharedPointer<const CompositeKey> key,
                               QSharedPointer<Database> db,
                               bool& hasError,
                               QString& errorString)
{
    KeePass2Reader reader;
    reader.readDatabase(path, key, db.data());
    hasError = reader.hasError();
    if (hasError) {
        errorString = reader.errorString();
    }
    QCOMPARE(reader.version(), KeePass2::FILE_VERSION_4);
}

void TestKdbx4Argon2::writeKdbx(QIODevice* device, Database* db, bool& hasError, QString& errorString)
{
    if (db->kdf()->uuid() == KeePass2::KDF_AES_KDBX3) {
        db->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D)));
    }
    KeePass2Writer writer;
    hasError = writer.writeDatabase(device, db);
    hasError = writer.hasError();
    if (hasError) {
        errorString = writer.errorString();
    }
    QCOMPARE(writer.version(), KeePass2::FILE_VERSION_4);
}

Q_DECLARE_METATYPE(QUuid)
void TestKdbx4Argon2::testFormat400()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/Format400.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("t"));
    KeePass2Reader reader;
    auto db = QSharedPointer<Database>::create();
    reader.readDatabase(filename, key, db.data());
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

void TestKdbx4Argon2::testFormat400Upgrade()
{
    QFETCH(QUuid, kdfUuid);
    QFETCH(QUuid, cipherUuid);
    QFETCH(bool, addCustomData);
    QFETCH(quint32, expectedVersion);

    QScopedPointer<Database> sourceDb(new Database());
    sourceDb->changeKdf(fastKdf(sourceDb->kdf()));
    sourceDb->metadata()->setName("Wubba lubba dub dub");
    QCOMPARE(sourceDb->kdf()->uuid(), KeePass2::KDF_AES_KDBX3); // default is legacy AES-KDF

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("I am in great pain, please help me!"));
    sourceDb->setKey(key, true, true);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    // upgrade to KDBX 4 by changing KDF and Cipher
    sourceDb->changeKdf(fastKdf(KeePass2::uuidToKdf(kdfUuid)));
    sourceDb->setCipher(cipherUuid);

    // CustomData in meta should not cause any version change
    sourceDb->metadata()->customData()->set("CustomPublicData", "Hey look, I turned myself into a pickle!");
    if (addCustomData) {
        // this, however, should
        sourceDb->rootGroup()->customData()->set("CustomGroupData",
                                                 "I just killed my family! I don't care who they were!");
    }

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, sourceDb.data());
    if (writer.hasError()) {
        QFAIL(qPrintable(QString("Error while writing database: %1").arg(writer.errorString())));
    }

    // read buffer back
    buffer.seek(0);
    KeePass2Reader reader;
    auto targetDb = QSharedPointer<Database>::create();
    reader.readDatabase(&buffer, key, targetDb.data());
    if (reader.hasError()) {
        QFAIL(qPrintable(QString("Error while reading database: %1").arg(reader.errorString())));
    }

    QVERIFY(targetDb->rootGroup());
    QCOMPARE(targetDb->metadata()->name(), sourceDb->metadata()->name());

    QCOMPARE(reader.version(), expectedVersion);
    QCOMPARE(targetDb->cipher(), cipherUuid);
    QCOMPARE(targetDb->metadata()->customData()->value("CustomPublicData"),
             sourceDb->metadata()->customData()->value("CustomPublicData"));
    QCOMPARE(targetDb->rootGroup()->customData()->value("CustomGroupData"),
             sourceDb->rootGroup()->customData()->value("CustomGroupData"));
}

// clang-format off
void TestKdbx4Argon2::testFormat400Upgrade_data()
{
    QTest::addColumn<QUuid>("kdfUuid");
    QTest::addColumn<QUuid>("cipherUuid");
    QTest::addColumn<bool>("addCustomData");
    QTest::addColumn<quint32>("expectedVersion");

    auto constexpr kdbx3 = KeePass2::FILE_VERSION_3_1 & KeePass2::FILE_VERSION_CRITICAL_MASK;
    auto constexpr kdbx4 = KeePass2::FILE_VERSION_4   & KeePass2::FILE_VERSION_CRITICAL_MASK;

    QTest::newRow("Argon2d          + AES")                  << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_AES256  << false << kdbx4;
    QTest::newRow("Argon2id         + AES")                  << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_AES256  << false << kdbx4;
    QTest::newRow("AES-KDF          + AES")                  << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_AES256  << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + AES")                  << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_AES256  << false << kdbx3;
    QTest::newRow("Argon2d          + AES     + CustomData") << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_AES256  << true  << kdbx4;
    QTest::newRow("Argon2id         + AES     + CustomData") << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_AES256  << true  << kdbx4;
    QTest::newRow("AES-KDF          + AES     + CustomData") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_AES256  << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + AES     + CustomData") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_AES256  << true  << kdbx4;

    QTest::newRow("Argon2d          + ChaCha20")              << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_CHACHA20 << false << kdbx4;
    QTest::newRow("Argon2id         + ChaCha20")              << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_CHACHA20 << false << kdbx4;
    QTest::newRow("AES-KDF          + ChaCha20")              << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_CHACHA20 << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + ChaCha20")              << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_CHACHA20 << false << kdbx3;
    QTest::newRow("Argon2d          + ChaCha20 + CustomData") << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_CHACHA20 << true  << kdbx4;
    QTest::newRow("Argon2id         + ChaCha20 + CustomData") << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_CHACHA20 << true  << kdbx4;
    QTest::newRow("AES-KDF          + ChaCha20 + CustomData") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_CHACHA20 << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + ChaCha20 + CustomData") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_CHACHA20 << true  << kdbx4;

    QTest::newRow("Argon2d          + Twofish")               << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_TWOFISH  << false << kdbx4;
    QTest::newRow("Argon2id         + Twofish")               << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_TWOFISH  << false << kdbx4;
    QTest::newRow("AES-KDF          + Twofish")               << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_TWOFISH  << false << kdbx4;
    QTest::newRow("AES-KDF (legacy) + Twofish")               << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_TWOFISH  << false << kdbx3;
    QTest::newRow("Argon2d          + Twofish  + CustomData") << KeePass2::KDF_ARGON2D   << KeePass2::CIPHER_TWOFISH  << true  << kdbx4;
    QTest::newRow("Argon2id         + Twofish  + CustomData") << KeePass2::KDF_ARGON2ID  << KeePass2::CIPHER_TWOFISH  << true  << kdbx4;
    QTest::newRow("AES-KDF          + Twofish  + CustomData") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_TWOFISH  << true  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + Twofish  + CustomData") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_TWOFISH  << true  << kdbx4;
}
// clang-format on

void TestKdbx4Argon2::testUpgradeMasterKeyIntegrity()
{
    QFETCH(QString, upgradeAction);
    QFETCH(quint32, expectedVersion);

    // prepare composite key
    auto passwordKey = QSharedPointer<PasswordKey>::create("turXpGMQiUE6CkPvWacydAKsnp4cxz");

    QByteArray fileKeyBytes("Ma6hHov98FbPeyAL22XhcgmpJk8xjQ");
    QBuffer fileKeyBuffer(&fileKeyBytes);
    fileKeyBuffer.open(QBuffer::ReadOnly);
    auto fileKey = QSharedPointer<FileKey>::create();
    fileKey->load(&fileKeyBuffer);

    auto crKey = QSharedPointer<MockChallengeResponseKey>::create(QByteArray("azdJnbVCFE76vV6t9RJ2DS6xvSS93k"));

    auto compositeKey = QSharedPointer<CompositeKey>::create();
    compositeKey->addKey(passwordKey);
    compositeKey->addKey(fileKey);
    compositeKey->addChallengeResponseKey(crKey);

    QScopedPointer<Database> db(new Database());
    db->changeKdf(fastKdf(db->kdf()));
    QCOMPARE(db->kdf()->uuid(), KeePass2::KDF_AES_KDBX3); // default is legacy AES-KDF
    db->setKey(compositeKey);

    // upgrade the database by a specific method
    if (upgradeAction == "none") {
        // do nothing
    } else if (upgradeAction == "meta-customdata") {
        db->metadata()->customData()->set("abc", "def");
    } else if (upgradeAction == "kdf-aes-kdbx3") {
        db->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX3)));
    } else if (upgradeAction == "kdf-argon2") {
        db->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D)));
    } else if (upgradeAction == "kdf-aes-kdbx4") {
        db->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX4)));
    } else if (upgradeAction == "public-customdata") {
        db->publicCustomData().insert("abc", "def");
    } else if (upgradeAction == "rootgroup-customdata") {
        db->rootGroup()->customData()->set("abc", "def");
    } else if (upgradeAction == "group-customdata") {
        auto group = new Group();
        group->setParent(db->rootGroup());
        group->setUuid(QUuid::createUuid());
        group->customData()->set("abc", "def");
    } else if (upgradeAction == "rootentry-customdata") {
        auto entry = new Entry();
        entry->setGroup(db->rootGroup());
        entry->setUuid(QUuid::createUuid());
        entry->customData()->set("abc", "def");
    } else if (upgradeAction == "entry-customdata") {
        auto group = new Group();
        group->setParent(db->rootGroup());
        group->setUuid(QUuid::createUuid());
        auto entry = new Entry();
        entry->setGroup(group);
        entry->setUuid(QUuid::createUuid());
        entry->customData()->set("abc", "def");
    } else {
        QFAIL(qPrintable(QString("Unknown action: %s").arg(upgradeAction)));
    }

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    KeePass2Writer writer;
    QVERIFY(writer.writeDatabase(&buffer, db.data()));

    // paranoid check that we cannot decrypt the database without a key
    buffer.seek(0);
    KeePass2Reader reader;
    auto db2 = QSharedPointer<Database>::create();
    reader.readDatabase(&buffer, QSharedPointer<CompositeKey>::create(), db2.data());
    QVERIFY(reader.hasError());

    // check that we can read back the database with the original composite key,
    // i.e., no components have been lost on the way
    buffer.seek(0);
    db2 = QSharedPointer<Database>::create();
    reader.readDatabase(&buffer, compositeKey, db2.data());
    if (reader.hasError()) {
        QFAIL(qPrintable(reader.errorString()));
    }
    QCOMPARE(reader.version(), expectedVersion & KeePass2::FILE_VERSION_CRITICAL_MASK);
    if (expectedVersion != KeePass2::FILE_VERSION_3) {
        QVERIFY(db2->kdf()->uuid() != KeePass2::KDF_AES_KDBX3);
    }
}

void TestKdbx4Argon2::testUpgradeMasterKeyIntegrity_data()
{
    QTest::addColumn<QString>("upgradeAction");
    QTest::addColumn<quint32>("expectedVersion");

    QTest::newRow("Upgrade: none") << QString("none") << KeePass2::FILE_VERSION_3;
    QTest::newRow("Upgrade: none (meta-customdata)") << QString("meta-customdata") << KeePass2::FILE_VERSION_3;
    QTest::newRow("Upgrade: none (explicit kdf-aes-kdbx3)") << QString("kdf-aes-kdbx3") << KeePass2::FILE_VERSION_3;
    QTest::newRow("Upgrade (explicit): kdf-argon2") << QString("kdf-argon2") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (explicit): kdf-aes-kdbx4") << QString("kdf-aes-kdbx4") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (implicit): public-customdata") << QString("public-customdata") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (implicit): rootgroup-customdata")
        << QString("rootgroup-customdata") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (implicit): group-customdata") << QString("group-customdata") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (implicit): rootentry-customdata")
        << QString("rootentry-customdata") << KeePass2::FILE_VERSION_4;
    QTest::newRow("Upgrade (implicit): entry-customdata") << QString("entry-customdata") << KeePass2::FILE_VERSION_4;
}

void TestKdbx4Argon2::testCustomData()
{
    Database db;

    // test public custom data
    QVariantMap publicCustomData;
    publicCustomData.insert("CD1", 123);
    publicCustomData.insert("CD2", true);
    publicCustomData.insert("CD3", "abcäöü");
    db.setPublicCustomData(publicCustomData);
    publicCustomData.insert("CD4", QByteArray::fromHex("ababa123ff"));
    db.publicCustomData().insert("CD4", publicCustomData.value("CD4"));
    QCOMPARE(db.publicCustomData(), publicCustomData);

    const QString customDataKey1 = "CD1";
    const QString customDataKey2 = "CD2";
    const QString customData1 = "abcäöü";
    const QString customData2 = "Hello World";

    // test custom database data
    db.metadata()->customData()->set(customDataKey1, customData1);
    db.metadata()->customData()->set(customDataKey2, customData2);
    auto lastModified = db.metadata()->customData()->value(CustomData::LastModified);
    const int dataSize = customDataKey1.toUtf8().size() + customDataKey1.toUtf8().size() + customData1.toUtf8().size()
                         + customData2.toUtf8().size() + lastModified.toUtf8().size()
                         + CustomData::LastModified.toUtf8().size();
    QCOMPARE(db.metadata()->customData()->size(), 3);
    QCOMPARE(db.metadata()->customData()->dataSize(), dataSize);

    // test custom root group data
    Group* root = db.rootGroup();
    root->customData()->set(customDataKey1, customData1);
    root->customData()->set(customDataKey2, customData2);
    QCOMPARE(root->customData()->size(), 3);
    QCOMPARE(root->customData()->dataSize(), dataSize);

    // test copied custom group data
    auto* group = new Group();
    group->setParent(root);
    group->setUuid(QUuid::createUuid());
    group->customData()->copyDataFrom(root->customData());
    QCOMPARE(*group->customData(), *root->customData());

    // test copied custom entry data
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setUuid(QUuid::createUuid());
    entry->customData()->copyDataFrom(group->customData());
    QCOMPARE(*entry->customData(), *root->customData());

    // test custom data deletion
    entry->customData()->set("additional item", "foobar");
    QCOMPARE(entry->customData()->size(), 4);
    entry->customData()->remove("additional item");
    QCOMPARE(entry->customData()->size(), 3);
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
    auto newDb = QSharedPointer<Database>::create();
    reader.readDatabase(&buffer, QSharedPointer<CompositeKey>::create(), newDb.data());

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

void TestKdbx4AesKdf::initTestCaseImpl()
{
    m_xmlDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX4)));
    m_kdbxSourceDb->changeKdf(fastKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX4)));
}
