/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "TestKeys.h"
#include "TestGlobal.h"

#include <QBuffer>

#include "config-keepassx-tests.h"

#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "crypto/CryptoHash.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "mock/MockChallengeResponseKey.h"

QTEST_GUILESS_MAIN(TestKeys)
Q_DECLARE_METATYPE(FileKey::Type);

void TestKeys::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestKeys::testComposite()
{
    auto compositeKey1 = QSharedPointer<CompositeKey>::create();
    auto passwordKey1 = QSharedPointer<PasswordKey>::create();
    auto passwordKey2 = QSharedPointer<PasswordKey>::create("test");

    // make sure that addKey() creates a copy of the keys
    compositeKey1->addKey(passwordKey1);
    compositeKey1->addKey(passwordKey2);

    AesKdf kdf;
    kdf.setRounds(1);
    QByteArray transformed1;
    QVERIFY(compositeKey1->transform(kdf, transformed1));
    QCOMPARE(transformed1.size(), 32);

    QScopedPointer<CompositeKey> compositeKey3(new CompositeKey());
    QScopedPointer<CompositeKey> compositeKey4(new CompositeKey());

    // test clear()
    compositeKey3->addKey(QSharedPointer<PasswordKey>::create("test"));
    compositeKey3->clear();
    QCOMPARE(compositeKey3->rawKey(), compositeKey4->rawKey());
}

void TestKeys::testFileKey()
{
    QFETCH(FileKey::Type, type);
    QFETCH(QString, keyExt);
    QFETCH(bool, fileKeyOk);

    QString name = QString("FileKey").append(QTest::currentDataTag());

    KeePass2Reader reader;

    QString dbFilename = QString("%1/%2.kdbx").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString keyFilename = QString("%1/%2.%3").arg(QString(KEEPASSX_TEST_DATA_DIR), name, keyExt);

    auto compositeKey = QSharedPointer<CompositeKey>::create();
    auto fileKey = QSharedPointer<FileKey>::create();
    QString error;
    QVERIFY(fileKey->load(keyFilename, &error) == fileKeyOk);
    QVERIFY(error.isEmpty() == fileKeyOk);
    QCOMPARE(fileKey->type(), type);

    // Test for same behaviour on code path without error parameter
    auto fileKeyNoErrorParam = QSharedPointer<FileKey>::create();
    QVERIFY(fileKeyNoErrorParam->load(keyFilename) == fileKeyOk);
    QCOMPARE(fileKeyNoErrorParam->type(), type);

    QCOMPARE(fileKey->rawKey(), fileKeyNoErrorParam->rawKey());

    if (!fileKeyOk) {
        return;
    }

    QCOMPARE(fileKey->rawKey().size(), 32);

    compositeKey->addKey(fileKey);

    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(dbFilename, compositeKey, nullptr, false));
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("%1 Database").arg(name));
}

// clang-format off
void TestKeys::testFileKey_data()
{
    QTest::addColumn<FileKey::Type>("type");
    QTest::addColumn<QString>("keyExt");
    QTest::addColumn<bool>("fileKeyOk");
    QTest::newRow("Xml")             << FileKey::KeePass2XML    << QString("key")  << true;
    QTest::newRow("XmlBrokenBase64") << FileKey::KeePass2XML    << QString("key")  << false;
    QTest::newRow("XmlV2")           << FileKey::KeePass2XMLv2  << QString("keyx") << true;
    QTest::newRow("XmlV2HashFail")   << FileKey::KeePass2XMLv2  << QString("keyx") << false;
    QTest::newRow("XmlV2BrokenHex")  << FileKey::KeePass2XMLv2  << QString("keyx") << false;
    QTest::newRow("Binary")          << FileKey::FixedBinary    << QString("key")  << true;
    QTest::newRow("Hex")             << FileKey::FixedBinaryHex << QString("key")  << true;
    QTest::newRow("Hashed")          << FileKey::Hashed         << QString("key")  << true;
}
// clang-format on

void TestKeys::testCreateFileKey()
{
    QBuffer keyBuffer1;
    keyBuffer1.open(QBuffer::ReadWrite);

    FileKey::createRandom(&keyBuffer1, 128);
    QCOMPARE(keyBuffer1.size(), 128);

    QBuffer keyBuffer2;
    keyBuffer2.open(QBuffer::ReadWrite);
    FileKey::createRandom(&keyBuffer2, 64);
    QCOMPARE(keyBuffer2.size(), 64);
}

void TestKeys::testCreateAndOpenFileKey()
{
    const QString dbName("testCreateFileKey database");

    QBuffer keyBuffer;
    keyBuffer.open(QBuffer::ReadWrite);

    FileKey::createRandom(&keyBuffer);
    keyBuffer.reset();

    auto fileKey = QSharedPointer<FileKey>::create();
    QVERIFY(fileKey->load(&keyBuffer));
    auto compositeKey = QSharedPointer<CompositeKey>::create();
    compositeKey->addKey(fileKey);

    QScopedPointer<Database> dbOrg(new Database());
    QVERIFY(dbOrg->setKey(compositeKey));
    dbOrg->metadata()->setName(dbName);

    QBuffer dbBuffer;
    dbBuffer.open(QBuffer::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&dbBuffer, dbOrg.data());
    bool writeSuccess = writer.writeDatabase(&dbBuffer, dbOrg.data());
    if (writer.hasError()) {
        QFAIL(writer.errorString().toUtf8().constData());
    }
    QVERIFY(writeSuccess);
    dbBuffer.reset();

    KeePass2Reader reader;
    auto dbRead = QSharedPointer<Database>::create();
    reader.readDatabase(&dbBuffer, compositeKey, dbRead.data());
    if (reader.hasError()) {
        QFAIL(reader.errorString().toUtf8().constData());
    }
    QVERIFY(dbRead);
    QCOMPARE(dbRead->metadata()->name(), dbName);
}

void TestKeys::testFileKeyHash()
{
    QBuffer keyBuffer;
    keyBuffer.open(QBuffer::ReadWrite);

    FileKey::createRandom(&keyBuffer);

    CryptoHash cryptoHash(CryptoHash::Sha256);
    cryptoHash.addData(keyBuffer.data());

    FileKey fileKey;
    fileKey.load(&keyBuffer);

    QCOMPARE(fileKey.rawKey(), cryptoHash.result());
}

void TestKeys::testFileKeyError()
{
    bool result;
    QString errorMsg;
    const QString fileName(QString(KEEPASSX_TEST_DATA_DIR).append("/does/not/exist"));

    FileKey fileKey;
    result = fileKey.load(fileName, &errorMsg);
    QVERIFY(!result);
    QVERIFY(!errorMsg.isEmpty());
    errorMsg = "";

    result = FileKey::create(fileName, &errorMsg);
    QVERIFY(!result);
    QVERIFY(!errorMsg.isEmpty());
    errorMsg = "";
}

void TestKeys::benchmarkTransformKey()
{
    QByteArray env = qgetenv("BENCHMARK");

    if (env.isEmpty() || env == "0" || env == "no") {
        QSKIP("Benchmark skipped. Set env variable BENCHMARK=1 to enable.");
    }

    auto pwKey = QSharedPointer<PasswordKey>::create();
    pwKey->setPassword("password");
    auto compositeKey = QSharedPointer<CompositeKey>::create();
    compositeKey->addKey(pwKey);

    QByteArray seed(32, '\x4B');

    QByteArray result;
    AesKdf kdf;
    kdf.setSeed(seed);
    kdf.setRounds(1e6);

    QBENCHMARK
    {
        Q_UNUSED(compositeKey->transform(kdf, result));
    };
}

void TestKeys::testCompositeKeyComponents()
{
    auto passwordKeyEnc = QSharedPointer<PasswordKey>::create("password");
    auto fileKeyEnc = QSharedPointer<FileKey>::create();
    QString error;
    fileKeyEnc->load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"), &error);
    if (!error.isEmpty()) {
        QFAIL(qPrintable(error));
    }
    auto challengeResponseKeyEnc = QSharedPointer<MockChallengeResponseKey>::create(QByteArray(16, 0x10));

    auto compositeKeyEnc = QSharedPointer<CompositeKey>::create();
    compositeKeyEnc->addKey(passwordKeyEnc);
    compositeKeyEnc->addKey(fileKeyEnc);
    compositeKeyEnc->addChallengeResponseKey(challengeResponseKeyEnc);

    auto db1 = QSharedPointer<Database>::create();
    db1->setKey(compositeKeyEnc);

    KeePass2Writer writer;
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    QVERIFY(writer.writeDatabase(&buffer, db1.data()));

    buffer.seek(0);
    auto db2 = QSharedPointer<Database>::create();
    KeePass2Reader reader;
    auto compositeKeyDec1 = QSharedPointer<CompositeKey>::create();

    // try decryption and subsequently add key components until decryption is successful
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec1, db2.data()));
    QVERIFY(reader.hasError());

    compositeKeyDec1->addKey(passwordKeyEnc);
    buffer.seek(0);
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec1, db2.data()));
    QVERIFY(reader.hasError());

    compositeKeyDec1->addKey(fileKeyEnc);
    buffer.seek(0);
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec1, db2.data()));
    QVERIFY(reader.hasError());

    compositeKeyDec1->addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    QVERIFY(reader.readDatabase(&buffer, compositeKeyDec1, db2.data()));
    // now we should be able to open the database
    if (reader.hasError()) {
        QFAIL(qPrintable(reader.errorString()));
    }

    // try the same again, but this time with one wrong key component each time
    auto compositeKeyDec2 = QSharedPointer<CompositeKey>::create();
    compositeKeyDec2->addKey(QSharedPointer<PasswordKey>::create("wrong password"));
    compositeKeyDec2->addKey(fileKeyEnc);
    compositeKeyDec2->addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec2, db2.data()));
    QVERIFY(reader.hasError());

    auto compositeKeyDec3 = QSharedPointer<CompositeKey>::create();
    compositeKeyDec3->addKey(passwordKeyEnc);
    auto fileKeyWrong = QSharedPointer<FileKey>::create();
    fileKeyWrong->load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed2.key"), &error);
    if (!error.isEmpty()) {
        QFAIL(qPrintable(error));
    }
    compositeKeyDec3->addKey(fileKeyWrong);
    compositeKeyDec3->addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec3, db2.data()));
    QVERIFY(reader.hasError());

    auto compositeKeyDec4 = QSharedPointer<CompositeKey>::create();
    compositeKeyDec4->addKey(passwordKeyEnc);
    compositeKeyDec4->addKey(fileKeyEnc);
    compositeKeyDec4->addChallengeResponseKey(QSharedPointer<MockChallengeResponseKey>::create(QByteArray(16, 0x20)));
    buffer.seek(0);
    QVERIFY(!reader.readDatabase(&buffer, compositeKeyDec4, db2.data()));
    QVERIFY(reader.hasError());
}
