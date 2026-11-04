/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2026 Felix Geyer <debfx@fobos.de>
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

#include "TestKeePass1Reader.h"

#include <QBuffer>
#include <QDataStream>
#include <QTest>

#include "config-keepassx-tests.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass1.h"
#include "format/KeePass1Reader.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestKeePass1Reader)

namespace
{
    QByteArray kdbField(quint16 type, const QByteArray& value = {})
    {
        QByteArray field;
        QDataStream stream(&field, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << type << static_cast<quint32>(value.size());
        field.append(value);
        return field;
    }

    QByteArray
    kdbMetastream(const QByteArray& name, const QByteArray& data, const QByteArray& uuid = QByteArray(16, 'm'))
    {
        return kdbField(1, uuid) + kdbField(2, QByteArray(4, '\0')) + kdbField(4, QByteArray("Meta-Info\0", 10))
               + kdbField(5, QByteArray("$\0", 2)) + kdbField(6, QByteArray("SYSTEM\0", 7)) + kdbField(8, name + '\0')
               + kdbField(13, QByteArray("bin-stream\0", 11)) + kdbField(14, data) + kdbField(0xffff);
    }
} // namespace

void TestKeePass1Reader::initTestCase()
{
    QVERIFY(Crypto::init());

    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb");

    KeePass1Reader reader;
    m_db = reader.readDatabase(filename, "masterpw", nullptr);
    QVERIFY(m_db);
    QVERIFY(!reader.hasError());
    QLocale::setDefault(QLocale::c());
}

void TestKeePass1Reader::testBasic()
{
    QVERIFY(m_db->deletedObjects().isEmpty());

    QCOMPARE(m_db->rootGroup()->children().size(), 2);

    Group* group1 = m_db->rootGroup()->children().at(0);
    QVERIFY(!group1->uuid().isNull());
    QCOMPARE(group1->name(), QString("Internet"));
    QCOMPARE(group1->children().size(), 2);
    QCOMPARE(group1->entries().size(), 2);
    QCOMPARE(group1->iconNumber(), 1);

    Entry* entry11 = group1->entries().at(0);
    QVERIFY(!entry11->uuid().isNull());
    QCOMPARE(entry11->title(), QString("Test entry"));
    QCOMPARE(entry11->iconNumber(), 1);
    QCOMPARE(entry11->username(), QString("I"));
    QCOMPARE(entry11->url(), QString("http://example.com/"));
    QCOMPARE(entry11->password(), QString("secretpassword"));
    QCOMPARE(entry11->notes(), QString("Lorem ipsum\ndolor sit amet"));
    QVERIFY(entry11->timeInfo().expires());
    QCOMPARE(entry11->timeInfo().expiryTime(), genDT(2012, 5, 9, 10, 32));
    QCOMPARE(entry11->attachments()->keys().size(), 1);
    QCOMPARE(entry11->attachments()->keys().at(0), QString("attachment.txt"));
    QCOMPARE(entry11->attachments()->value("attachment.txt"), QByteArray("hello world\n"));

    Entry* entry12 = group1->entries().at(1);
    QCOMPARE(entry12->title(), QString(""));
    QCOMPARE(entry12->iconNumber(), 0);
    QCOMPARE(entry12->username(), QString(""));
    QCOMPARE(entry12->url(), QString(""));
    QCOMPARE(entry12->password(), QString(""));
    QCOMPARE(entry12->notes(), QString(""));
    QVERIFY(!entry12->timeInfo().expires());
    QCOMPARE(entry12->attachments()->keys().size(), 0);

    Group* group11 = group1->children().at(0);
    QCOMPARE(group11->name(), QString("Subgroup 1"));
    QCOMPARE(group11->children().size(), 1);

    Group* group111 = group11->children().at(0);
    QCOMPARE(group111->name(), QString("Unexpanded"));
    QCOMPARE(group111->children().size(), 1);

    Group* group1111 = group111->children().at(0);
    QCOMPARE(group1111->name(), QString("abc"));
    QCOMPARE(group1111->children().size(), 0);

    Group* group12 = group1->children().at(1);
    QCOMPARE(group12->name(), QString("Subgroup 2"));
    QCOMPARE(group12->children().size(), 0);

    Group* group2 = m_db->rootGroup()->children().at(1);
    QCOMPARE(group2->name(), QString("eMail"));
    QCOMPARE(group2->entries().size(), 1);
    QCOMPARE(group2->iconNumber(), 19);

    reopenDatabase(m_db, "masterpw", QString());
}

void TestKeePass1Reader::testEntryFields_data()
{
    QTest::addColumn<QByteArray>("entryFields");
    QTest::addColumn<bool>("success");

    const auto uuidField = kdbField(1, QByteArray(16, 'b'));
    // Both groups exist; the final group field must determine the entry's parent.
    const auto groupFields =
        kdbField(2, QByteArray::fromHex("00000000")) + kdbField(2, QByteArray::fromHex("01000000"));
    QTest::newRow("duplicate-group-last-is-used") << uuidField + groupFields << true;
    QTest::newRow("missing-uuid") << groupFields << false;
    QTest::newRow("missing-group") << uuidField << false;
    QTest::newRow("missing-both") << QByteArray() << false;
}

void TestKeePass1Reader::testEntryFields()
{
    QFETCH(QByteArray, entryFields);
    QFETCH(bool, success);

    const QByteArray content = entryFields + kdbField(4, QByteArray("entry\0", 6)) + kdbField(0xffff);

    KeePass1Reader reader;
    QSharedPointer<Database> db;
    readTestDatabase(content, 1, reader, db);
    if (!success) {
        QVERIFY(!db);
        QVERIFY(reader.hasError());
        QCOMPARE(reader.errorString(), QStringLiteral("Incomplete entry: No UUID or group ID"));
        return;
    }
    QVERIFY2(db, qPrintable(reader.errorString()));
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->entriesRecursive().size(), 1);
    const auto entry = db->rootGroup()->entriesRecursive().first();
    QCOMPARE(entry->title(), QStringLiteral("entry"));
    QCOMPARE(entry->group(), db->rootGroup()->children().at(1));
}

void TestKeePass1Reader::testDeletedMetastreamUuidAliases()
{
    const QByteArray firstUuid(16, 'a'), lastUuid(16, 'b');
    // This metastream has two UUIDs and is deleted before the next metastream is processed.
    QByteArray content = kdbField(1, firstUuid) + kdbMetastream("KPX_GROUP_TREE_STATE", QByteArray(4, '\0'), lastUuid);

    const auto icon = m_db->metadata()->customIcon(m_db->metadata()->customIconsOrder().first()).data;
    QByteArray iconData;
    QDataStream icons(&iconData, QIODevice::WriteOnly);
    icons.setByteOrder(QDataStream::LittleEndian);
    icons << static_cast<quint32>(1) << static_cast<quint32>(2) << static_cast<quint32>(0)
          << static_cast<quint32>(icon.size());
    icons.writeRawData(icon.constData(), static_cast<int>(icon.size()));
    // Look up both UUIDs: the old reader removed only one, depending on hash iteration order.
    for (const auto& uuid : {firstUuid, lastUuid}) {
        icons.writeRawData(uuid.constData(), 16);
        icons << static_cast<quint32>(0);
    }
    content += kdbMetastream("KPX_CUSTOM_ICONS_4", iconData);

    KeePass1Reader reader;
    QSharedPointer<Database> db;
    // ASan/Valgrind detects a stale UUID lookup accessing the deleted entry.
    readTestDatabase(content, 2, reader, db);
    QVERIFY2(db, qPrintable(reader.errorString()));
    QVERIFY(!reader.hasError());
    QVERIFY(db->rootGroup()->entriesRecursive().isEmpty());
    QCOMPARE(db->metadata()->customIconsOrder().size(), 1);
}

void TestKeePass1Reader::testMetastreamBounds_data()
{
    QTest::addColumn<QByteArray>("name");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("valid");

    const auto words = [](std::initializer_list<quint32> values) {
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        for (auto value : values) {
            stream << value;
        }
        return bytes;
    };
    const QByteArray tree("KPX_GROUP_TREE_STATE");
    QTest::newRow("tree-empty") << tree << words({0}) << true;
    QTest::newRow("tree-one-record") << tree << words({1, 0}) + QByteArray(1, '\0') << true;
    QTest::newRow("tree-missing-count") << tree << QByteArray(3, '\0') << false;
    QTest::newRow("tree-truncated-record") << tree << words({1, 0}) << false;
    QTest::newRow("tree-trailing-byte") << tree << words({0}) + QByteArray(1, '\0') << false;
    // 0xcccccccd * 5 wraps to 1 in 32-bit arithmetic.
    QTest::newRow("tree-count-wrap") << tree << words({0xcccccccd}) + QByteArray(1, '\0') << false;
    QTest::newRow("tree-maximum-count") << tree << words({0xffffffff}) << false;

    const QByteArray icons("KPX_CUSTOM_ICONS_4");
    QTest::newRow("icons-empty") << icons << words({0, 0, 0}) << true;
    QTest::newRow("icons-truncated-header") << icons << QByteArray(11, '\0') << false;
    QTest::newRow("icons-missing-size") << icons << words({1, 0, 0}) << false;
    QTest::newRow("icons-truncated-image") << icons << words({1, 0, 0, 1}) << false;
    // The icon starts at offset 16; adding 0xfffffff0 used to wrap to zero.
    QTest::newRow("icons-size-wrap") << icons << words({1, 0, 0, 0xfffffff0}) << false;
    QTest::newRow("icons-maximum-size") << icons << words({1, 0, 0, 0xffffffff}) << false;
    QTest::newRow("icons-entry-count-wrap") << icons << words({0, 0x40000000, 0}) << false;
    QTest::newRow("icons-entry-offset-wrap") << icons << words({0, 0x33333333, 0}) << false;
    QTest::newRow("icons-group-count-wrap") << icons << words({0, 0, 0x20000000}) << false;
    QTest::newRow("icons-group-offset-wrap") << icons << words({0, 0, 0x1fffffff}) << false;
    QTest::newRow("icons-truncated-entry") << icons << words({0, 1, 0}) + QByteArray(19, '\0') << false;
    QTest::newRow("icons-truncated-group") << icons << words({0, 0, 1}) + QByteArray(7, '\0') << false;
}

void TestKeePass1Reader::testMetastreamBounds()
{
    QFETCH(QByteArray, name);
    QFETCH(QByteArray, data);
    QFETCH(bool, valid);
    if (!valid) {
        QTest::ignoreMessage(QtWarningMsg,
                             name == "KPX_GROUP_TREE_STATE" ? "Unable to parse group tree state metastream."
                                                            : "Unable to parse custom icons metastream.");
    }
    KeePass1Reader reader;
    QSharedPointer<Database> db;
    readTestDatabase(kdbMetastream(name, data), 1, reader, db);
    // Malformed optional metadata is ignored, while the database itself is still imported.
    QVERIFY2(db, qPrintable(reader.errorString()));
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->children().size(), 2);
    QVERIFY(db->rootGroup()->entriesRecursive().isEmpty());
    QVERIFY(db->metadata()->customIconsOrder().isEmpty());
    if (valid && name == "KPX_GROUP_TREE_STATE" && data.size() > 4) {
        QVERIFY(!db->rootGroup()->children().first()->isExpanded());
    }
}

void TestKeePass1Reader::readTestDatabase(const QByteArray& entries,
                                          quint32 entryCount,
                                          KeePass1Reader& reader,
                                          QSharedPointer<Database>& db)
{
    QByteArray content;
    for (const auto& id : {QByteArray::fromHex("00000000"), QByteArray::fromHex("01000000")}) {
        content += kdbField(1, id) + kdbField(8, QByteArray(2, '\0')) + kdbField(0xffff);
    }
    content += entries;

    const QByteArray masterSeed(16, 's'), iv(16, 'i'), transformSeed(32, 't');
    AesKdf kdf(true);
    QVERIFY(kdf.setRounds(1));
    QVERIFY(kdf.setSeed(transformSeed));
    QByteArray transformedKey;
    QVERIFY(kdf.transform(CryptoHash::hash(QByteArray("test"), CryptoHash::Sha256), transformedKey));
    const auto key = CryptoHash::hash(masterSeed + transformedKey, CryptoHash::Sha256);
    SymmetricCipher cipher;
    QVERIFY(cipher.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Encrypt, key, iv));
    auto ciphertext = content;
    QVERIFY(cipher.finish(ciphertext));

    QByteArray file;
    QDataStream header(&file, QIODevice::WriteOnly);
    header.setByteOrder(QDataStream::LittleEndian);
    header << KeePass1::SIGNATURE_1 << KeePass1::SIGNATURE_2 << static_cast<quint32>(KeePass1::Rijndael)
           << KeePass1::FILE_VERSION;
    header.writeRawData(masterSeed.constData(), 16);
    header.writeRawData(iv.constData(), 16);
    header << static_cast<quint32>(2) << entryCount;
    const auto hash = CryptoHash::hash(content, CryptoHash::Sha256);
    header.writeRawData(hash.constData(), 32);
    header.writeRawData(transformSeed.constData(), 32);
    header << static_cast<quint32>(1);
    file.append(ciphertext);

    QBuffer buffer(&file);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    db = reader.readDatabase(&buffer, "test", nullptr);
}

void TestKeePass1Reader::testMasterKey()
{
    QVERIFY(m_db->isInitialized());
    QCOMPARE(m_db->kdf()->rounds(), 713);
}

void TestKeePass1Reader::testCustomIcons()
{
    QCOMPARE(m_db->metadata()->customIconsOrder().size(), 1);
    QUuid uuid = m_db->metadata()->customIconsOrder().at(0);
    QVERIFY(m_db->metadata()->hasCustomIcon(uuid));
    QByteArray icon = m_db->metadata()->customIcon(uuid).data;

    QVERIFY(icon.startsWith(
        "\x89PNG\r\n\x1A\n\x00\x00\x00\rIHDR\x00\x00\x00\x10\x00\x00\x00\x10\b\x06\x00\x00\x00\x1F\xF3\xFF"));
}

void TestKeePass1Reader::testGroupExpanded()
{
    QCOMPARE(m_db->rootGroup()->children().at(0)->isExpanded(), true);
    QCOMPARE(m_db->rootGroup()->children().at(0)->children().at(0)->isExpanded(), true);
    QCOMPARE(m_db->rootGroup()->children().at(0)->children().at(0)->children().at(0)->isExpanded(), false);
}

void TestKeePass1Reader::testAutoType()
{
    Group* group = m_db->rootGroup()->children().at(0)->children().at(0);
    QCOMPARE(group->entries().size(), 2);

    Entry* entry1 = group->entries().at(0);
    QCOMPARE(entry1->notes(), QString("last line"));
    QCOMPARE(entry1->defaultAutoTypeSequence(), QString("{USERNAME}{ENTER}"));
    QCOMPARE(entry1->autoTypeAssociations()->size(), 5);
    QCOMPARE(entry1->autoTypeAssociations()->get(0).sequence, QString(""));
    QCOMPARE(entry1->autoTypeAssociations()->get(0).window, QString("a window"));
    QCOMPARE(entry1->autoTypeAssociations()->get(1).sequence, QString(""));
    QCOMPARE(entry1->autoTypeAssociations()->get(1).window, QString("a second window"));
    QCOMPARE(entry1->autoTypeAssociations()->get(2).sequence, QString("{PASSWORD}{ENTER}"));
    QCOMPARE(entry1->autoTypeAssociations()->get(2).window, QString("Window Nr 1a"));
    QCOMPARE(entry1->autoTypeAssociations()->get(3).sequence, QString("{PASSWORD}{ENTER}"));
    QCOMPARE(entry1->autoTypeAssociations()->get(3).window, QString("Window Nr 1b"));
    QCOMPARE(entry1->autoTypeAssociations()->get(4).sequence, QString(""));
    QCOMPARE(entry1->autoTypeAssociations()->get(4).window, QString("Window 2"));

    Entry* entry2 = group->entries().at(1);
    QCOMPARE(entry2->notes(), QString("start line\nend line"));
    QCOMPARE(entry2->defaultAutoTypeSequence(), QString(""));
    QCOMPARE(entry2->autoTypeAssociations()->size(), 2);
    QCOMPARE(entry2->autoTypeAssociations()->get(0).sequence, QString(""));
    QCOMPARE(entry2->autoTypeAssociations()->get(0).window, QString("Main Window"));
    QCOMPARE(entry2->autoTypeAssociations()->get(1).sequence, QString(""));
    QCOMPARE(entry2->autoTypeAssociations()->get(1).window, QString("Test Window"));
}

void TestKeePass1Reader::testFileKey()
{
    QFETCH(QString, type);

    QString name = QString("FileKey").append(type);

    KeePass1Reader reader;

    QString dbFilename = QString("%1/%2.kdb").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString keyFilename = QString("%1/%2.key").arg(QString(KEEPASSX_TEST_DATA_DIR), name);

    auto db = reader.readDatabase(dbFilename, QString(), keyFilename);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->children().size(), 1);
    QCOMPARE(db->rootGroup()->children().at(0)->name(), name);

    reopenDatabase(db, QString(), keyFilename);
}

void TestKeePass1Reader::testFileKey_data()
{
    QTest::addColumn<QString>("type");
    QTest::newRow("Binary") << QString("Binary");
    QTest::newRow("Hex") << QString("Hex");
    QTest::newRow("Hashed") << QString("Hashed");
}

void TestKeePass1Reader::testCompositeKey()
{
    QString name = "CompositeKey";

    KeePass1Reader reader;

    QString dbFilename = QString("%1/%2.kdb").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString keyFilename = QString("%1/FileKeyHex.key").arg(QString(KEEPASSX_TEST_DATA_DIR));

    auto db = reader.readDatabase(dbFilename, "mypassword", keyFilename);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->children().size(), 1);
    QCOMPARE(db->rootGroup()->children().at(0)->name(), name);

    reopenDatabase(db, "mypassword", keyFilename);
}

void TestKeePass1Reader::testTwofish()
{
    QString name = "Twofish";

    KeePass1Reader reader;

    QString dbFilename = QString("%1/%2.kdb").arg(QString(KEEPASSX_TEST_DATA_DIR), name);

    auto db = reader.readDatabase(dbFilename, "masterpw", nullptr);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->children().size(), 1);
    QCOMPARE(db->rootGroup()->children().at(0)->name(), name);
}

#ifdef Q_OS_WIN
void TestKeePass1Reader::testCP1252Password()
{
    QString name = "CP-1252";

    KeePass1Reader reader;

    QString dbFilename = QString("%1/%2.kdb").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString password = QString::fromUtf8("\xe2\x80\x9e\x70\x61\x73\x73\x77\x6f\x72\x64\xe2\x80\x9d");

    auto db = reader.readDatabase(dbFilename, password, nullptr);
    QVERIFY(db);
    QVERIFY(!reader.hasError());
    QCOMPARE(db->rootGroup()->children().size(), 1);
    QCOMPARE(db->rootGroup()->children().at(0)->name(), name);
}
#endif

void TestKeePass1Reader::cleanupTestCase()
{
}

QDateTime TestKeePass1Reader::genDT(int year, int month, int day, int hour, int min)
{
    QDate date(year, month, day);
    QTime time(hour, min, 0);
    return {date, time, Qt::UTC};
}

void TestKeePass1Reader::reopenDatabase(QSharedPointer<Database> db,
                                        const QString& password,
                                        const QString& keyfileName)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, db.data());
    QVERIFY(!writer.hasError());
    QVERIFY(buffer.seek(0));

    auto key = QSharedPointer<CompositeKey>::create();
    if (!password.isEmpty()) {
        key->addKey(QSharedPointer<PasswordKey>::create(password));
    }
    if (!keyfileName.isEmpty()) {
        auto fileKey = QSharedPointer<FileKey>::create();
        QVERIFY(fileKey->load(keyfileName));
        key->addKey(fileKey);
    }

    KeePass2Reader reader;
    auto newDb = QSharedPointer<Database>::create();
    QVERIFY(reader.readDatabase(&buffer, key, newDb.data()));
    QVERIFY(!reader.hasError());
}
