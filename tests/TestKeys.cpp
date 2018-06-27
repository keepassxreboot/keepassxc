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
    QScopedPointer<CompositeKey> compositeKey1(new CompositeKey());
    QScopedPointer<PasswordKey> passwordKey1(new PasswordKey());
    QScopedPointer<PasswordKey> passwordKey2(new PasswordKey("test"));

    // make sure that addKey() creates a copy of the keys
    compositeKey1->addKey(*passwordKey1);
    compositeKey1->addKey(*passwordKey2);

    AesKdf kdf;
    kdf.setRounds(1);
    QByteArray transformed1;
    QVERIFY(compositeKey1->transform(kdf, transformed1));
    QCOMPARE(transformed1.size(), 32);

    // make sure the subkeys are copied
    QScopedPointer<CompositeKey> compositeKey2(compositeKey1->clone());
    QByteArray transformed2;
    QVERIFY(compositeKey2->transform(kdf, transformed2));
    QCOMPARE(transformed2.size(), 32);
    QCOMPARE(transformed1, transformed2);

    QScopedPointer<CompositeKey> compositeKey3(new CompositeKey());
    QScopedPointer<CompositeKey> compositeKey4(new CompositeKey());

    // test clear()
    compositeKey3->addKey(PasswordKey("test"));
    compositeKey3->clear();
    QCOMPARE(compositeKey3->rawKey(), compositeKey4->rawKey());

    // test assignment operator
    compositeKey3->addKey(PasswordKey("123"));
    *compositeKey4 = *compositeKey3;
    QCOMPARE(compositeKey3->rawKey(), compositeKey4->rawKey());

    // test self-assignment
    *compositeKey3 = *compositeKey3;
    QCOMPARE(compositeKey3->rawKey(), compositeKey4->rawKey());
}

void TestKeys::testFileKey()
{
    QFETCH(FileKey::Type, type);
    QFETCH(QString, typeString);

    QString name = QString("FileKey").append(typeString);

    KeePass2Reader reader;

    QString dbFilename = QString("%1/%2.kdbx").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString keyFilename = QString("%1/%2.key").arg(QString(KEEPASSX_TEST_DATA_DIR), name);

    CompositeKey compositeKey;
    FileKey fileKey;
    QVERIFY(fileKey.load(keyFilename));
    QCOMPARE(fileKey.rawKey().size(), 32);

    QCOMPARE(fileKey.type(), type);

    compositeKey.addKey(fileKey);

    QScopedPointer<Database> db(reader.readDatabase(dbFilename, compositeKey));
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->metadata()->name(), QString("%1 Database").arg(name));
}

// clang-format off
void TestKeys::testFileKey_data()
{
    QTest::addColumn<FileKey::Type>("type");
    QTest::addColumn<QString>("typeString");
    QTest::newRow("Xml")             << FileKey::KeePass2XML    << QString("Xml");
    QTest::newRow("XmlBrokenBase64") << FileKey::Hashed         << QString("XmlBrokenBase64");
    QTest::newRow("Binary")          << FileKey::FixedBinary    << QString("Binary");
    QTest::newRow("Hex")             << FileKey::FixedBinaryHex << QString("Hex");
    QTest::newRow("Hashed")          << FileKey::Hashed         << QString("Hashed");
}
// clang-format on

void TestKeys::testCreateFileKey()
{
    QBuffer keyBuffer1;
    keyBuffer1.open(QBuffer::ReadWrite);

    FileKey::create(&keyBuffer1, 128);
    QCOMPARE(keyBuffer1.size(), 128);

    QBuffer keyBuffer2;
    keyBuffer2.open(QBuffer::ReadWrite);
    FileKey::create(&keyBuffer2, 64);
    QCOMPARE(keyBuffer2.size(), 64);
}

void TestKeys::testCreateAndOpenFileKey()
{
    const QString dbName("testCreateFileKey database");

    QBuffer keyBuffer;
    keyBuffer.open(QBuffer::ReadWrite);

    FileKey::create(&keyBuffer);
    keyBuffer.reset();

    FileKey fileKey;
    QVERIFY(fileKey.load(&keyBuffer));
    CompositeKey compositeKey;
    compositeKey.addKey(fileKey);

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
    QScopedPointer<Database> dbRead(reader.readDatabase(&dbBuffer, compositeKey));
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

    FileKey::create(&keyBuffer);

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

    PasswordKey pwKey;
    pwKey.setPassword("password");
    CompositeKey compositeKey;
    compositeKey.addKey(pwKey);

    QByteArray seed(32, '\x4B');

    QByteArray result;
    AesKdf kdf;
    kdf.setSeed(seed);
    kdf.setRounds(1e6);

    QBENCHMARK
    {
        Q_UNUSED(compositeKey.transform(kdf, result));
    };
}

void TestKeys::testCompositeKeyComponents()
{
    PasswordKey passwordKeyEnc("password");
    FileKey fileKeyEnc;
    QString error;
    fileKeyEnc.load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed.key"), &error);
    if (!error.isNull()) {
        QFAIL(qPrintable(error));
    }
    auto challengeResponseKeyEnc = QSharedPointer<MockChallengeResponseKey>::create(QByteArray(16, 0x10));

    CompositeKey compositeKeyEnc;
    compositeKeyEnc.addKey(passwordKeyEnc);
    compositeKeyEnc.addKey(fileKeyEnc);
    compositeKeyEnc.addChallengeResponseKey(challengeResponseKeyEnc);

    QScopedPointer<Database> db1(new Database());
    db1->setKey(compositeKeyEnc);

    KeePass2Writer writer;
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    QVERIFY(writer.writeDatabase(&buffer, db1.data()));

    buffer.seek(0);
    QScopedPointer<Database> db2;
    KeePass2Reader reader;
    CompositeKey compositeKeyDec1;

    // try decryption and subsequently add key components until decryption is successful
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec1));
    QVERIFY(reader.hasError());

    compositeKeyDec1.addKey(passwordKeyEnc);
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec1));
    QVERIFY(reader.hasError());

    compositeKeyDec1.addKey(fileKeyEnc);
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec1));
    QVERIFY(reader.hasError());

    compositeKeyDec1.addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec1));
    // now we should be able to open the database
    if (reader.hasError()) {
        QFAIL(qPrintable(reader.errorString()));
    }

    // try the same again, but this time with one wrong key component each time
    CompositeKey compositeKeyDec2;
    compositeKeyDec2.addKey(PasswordKey("wrong password"));
    compositeKeyDec2.addKey(fileKeyEnc);
    compositeKeyDec2.addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec2));
    QVERIFY(reader.hasError());

    CompositeKey compositeKeyDec3;
    compositeKeyDec3.addKey(passwordKeyEnc);
    FileKey fileKeyWrong;
    fileKeyWrong.load(QString("%1/%2").arg(QString(KEEPASSX_TEST_DATA_DIR), "FileKeyHashed2.key"), &error);
    if (!error.isNull()) {
        QFAIL(qPrintable(error));
    }
    compositeKeyDec3.addKey(fileKeyWrong);
    compositeKeyDec3.addChallengeResponseKey(challengeResponseKeyEnc);
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec3));
    QVERIFY(reader.hasError());

    CompositeKey compositeKeyDec4;
    compositeKeyDec4.addKey(passwordKeyEnc);
    compositeKeyDec4.addKey(fileKeyEnc);
    compositeKeyDec4.addChallengeResponseKey(QSharedPointer<MockChallengeResponseKey>::create(QByteArray(16, 0x20)));
    buffer.seek(0);
    db2.reset(reader.readDatabase(&buffer, compositeKeyDec4));
    QVERIFY(reader.hasError());
}
