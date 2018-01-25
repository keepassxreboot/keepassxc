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
    QCOMPARE(targetDb->kdf()->uuid(), sourceDb->kdf()->uuid());
    QCOMPARE(targetDb->cipher(), cipherUuid);
}

void TestKdbx4::testFormat400Upgrade_data()
{
    QTest::addColumn<Uuid>("kdfUuid");
    QTest::addColumn<Uuid>("cipherUuid");
    QTest::addColumn<quint32>("expectedVersion");

    auto constexpr kdbx3 = KeePass2::FILE_VERSION_3_1 & KeePass2::FILE_VERSION_CRITICAL_MASK;
    auto constexpr kdbx4 = KeePass2::FILE_VERSION_4   & KeePass2::FILE_VERSION_CRITICAL_MASK;

    QTest::newRow("Argon2           + AES")      << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_AES      << kdbx4;
    QTest::newRow("AES-KDF          + AES")      << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_AES      << kdbx4;
    QTest::newRow("AES-KDF (legacy) + AES")      << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_AES      << kdbx3;
    QTest::newRow("Argon2           + ChaCha20") << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_CHACHA20 << kdbx4;
    QTest::newRow("AES-KDF          + ChaCha20") << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_CHACHA20 << kdbx4;
    QTest::newRow("AES-KDF (legacy) + ChaCha20") << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_CHACHA20 << kdbx3;
    QTest::newRow("Argon2           + Twofish")  << KeePass2::KDF_ARGON2    << KeePass2::CIPHER_TWOFISH  << kdbx4;
    QTest::newRow("AES-KDF          + Twofish")  << KeePass2::KDF_AES_KDBX4 << KeePass2::CIPHER_TWOFISH  << kdbx4;
    QTest::newRow("AES-KDF (legacy) + Twofish")  << KeePass2::KDF_AES_KDBX3 << KeePass2::CIPHER_TWOFISH  << kdbx3;
}
