/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestSharing.h"

#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>

#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/FileWatcher.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "crypto/Random.h"
#include "keeshare/KeeShare.h"
#include "keeshare/KeeShareSettings.h"
#include "keeshare/ShareObserver.h"
#include "util/TemporaryFile.h"

#include <botan/rsa.h>

QTEST_GUILESS_MAIN(TestSharing)

Q_DECLARE_METATYPE(KeeShareSettings::Type)
Q_DECLARE_METATYPE(KeeShareSettings::Key)
Q_DECLARE_METATYPE(KeeShareSettings::Certificate)

void TestSharing::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
    Config::createConfigFromFile(TemporaryFile::createTempConfigFile(), {});
    KeeShare::init(this);
}

void TestSharing::testNullObjects()
{
    const QString empty;
    QXmlStreamReader reader(empty);

    const auto nullKey = KeeShareSettings::Key();
    QVERIFY(nullKey.isNull());
    const auto xmlKey = KeeShareSettings::Key::deserialize(reader);
    QVERIFY(xmlKey.isNull());

    const auto certificate = KeeShareSettings::Certificate();
    QVERIFY(certificate.isNull());
    const auto xmlCertificate = KeeShareSettings::Certificate::deserialize(reader);
    QVERIFY(xmlCertificate.isNull());

    const auto own = KeeShareSettings::Own();
    QVERIFY(own.isNull());
    const auto xmlOwn = KeeShareSettings::Own::deserialize(empty);
    QVERIFY(xmlOwn.isNull());

    const auto active = KeeShareSettings::Active();
    QVERIFY(active.isNull());
    const auto xmlActive = KeeShareSettings::Active::deserialize(empty);
    QVERIFY(xmlActive.isNull());

    const auto reference = KeeShareSettings::Reference();
    QVERIFY(reference.isNull());
    const auto xmlReference = KeeShareSettings::Reference::deserialize(empty);
    QVERIFY(xmlReference.isNull());
}

void TestSharing::testKeySerialization()
{
    auto key = stubkey();
    KeeShareSettings::Key original;
    original.key = key;

    QString buffer;
    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeStartElement("Key");
    KeeShareSettings::Key::serialize(writer, original);
    writer.writeEndElement();
    writer.writeEndDocument();
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    QVERIFY(reader.name().toString() == "Key");
    KeeShareSettings::Key restored = KeeShareSettings::Key::deserialize(reader);

    QCOMPARE(restored.key->private_key_bits(), original.key->private_key_bits());
    QCOMPARE(restored.key->algo_name(), original.key->algo_name());
}

void TestSharing::testReferenceSerialization()
{
    QFETCH(QString, password);
    QFETCH(QString, path);
    QFETCH(QUuid, uuid);
    QFETCH(int, type);
    KeeShareSettings::Reference original;
    original.password = password;
    original.path = path;
    original.uuid = uuid;
    original.type = static_cast<KeeShareSettings::Type>(type);

    const QString serialized = KeeShareSettings::Reference::serialize(original);
    const KeeShareSettings::Reference restored = KeeShareSettings::Reference::deserialize(serialized);

    QCOMPARE(restored.password, original.password);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.uuid, original.uuid);
    QCOMPARE(int(restored.type), int(original.type));
}

void TestSharing::testReferenceSerialization_data()
{
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QUuid>("uuid");
    QTest::addColumn<int>("type");
    QTest::newRow("1") << "Password" << "/some/path" << QUuid::createUuid() << int(KeeShareSettings::Inactive);
    QTest::newRow("2") << "" << "" << QUuid() << int(KeeShareSettings::SynchronizeWith);
    QTest::newRow("3") << "" << "/some/path" << QUuid() << int(KeeShareSettings::ExportTo);
}

void TestSharing::testReferenceGroupUuid_data()
{
    QTest::addColumn<QString>("groupElement");
    QTest::addColumn<QUuid>("expectedUuid");

    QTest::newRow("missing") << "" << QUuid();
    QTest::newRow("empty") << "<Group/>" << QUuid();
    QTest::newRow("invalid-base64") << "<Group>!!!</Group>" << QUuid();
    QTest::newRow("wrong-length") << "<Group>YWJj</Group>" << QUuid();
    QTest::newRow("null") << "<Group>AAAAAAAAAAAAAAAAAAAAAA==</Group>" << QUuid();
    const QUuid uuid("{01234567-89ab-cdef-0123-456789abcdef}");
    QTest::newRow("valid") << QString("<Group>%1</Group>").arg(QString::fromLatin1(uuid.toRfc4122().toBase64()))
                           << uuid;
}

void TestSharing::testReferenceGroupUuid()
{
    QFETCH(QString, groupElement);
    QFETCH(QUuid, expectedUuid);

    const QString xml = QString("<KeeShare><Type><Import/></Type>%1"
                                "<Path>c2hhcmUua2RieA==</Path><Password>cG9jLXNoYXJl</Password>"
                                "<KeepGroups>False</KeepGroups></KeeShare>")
                            .arg(groupElement);
    Group group;
    group.customData()->set("KeeShare/Reference", xml.toUtf8().toBase64());
    const auto first = KeeShare::referenceOf(&group);
    const auto second = KeeShare::referenceOf(&group);

    QCOMPARE(first.uuid, expectedUuid);
    QCOMPARE(first.path, QString("share.kdbx"));
    QCOMPARE(first.password, QString("poc-share"));
    QCOMPARE(first.type, KeeShareSettings::Type(KeeShareSettings::ImportFrom));
    QVERIFY(!first.keepGroups);
    QCOMPARE(first.isValid(), !expectedUuid.isNull());
    // Parsing the same reference must not generate a new identity on each scan.
    QVERIFY(first == second);
}

void TestSharing::testReferenceValidity_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QUuid>("uuid");
    QTest::addColumn<bool>("valid");

    const auto uuid = QUuid::createUuid();
    QTest::newRow("inactive") << int(KeeShareSettings::Inactive) << "share.kdbx" << uuid << false;
    QTest::newRow("empty-path") << int(KeeShareSettings::ImportFrom) << "" << uuid << false;
    QTest::newRow("null-uuid") << int(KeeShareSettings::ImportFrom) << "share.kdbx" << QUuid() << false;
    QTest::newRow("import") << int(KeeShareSettings::ImportFrom) << "share.kdbx" << uuid << true;
    QTest::newRow("export") << int(KeeShareSettings::ExportTo) << "share.kdbx" << uuid << true;
    QTest::newRow("synchronize") << int(KeeShareSettings::SynchronizeWith) << "share.kdbx" << uuid << true;
}

void TestSharing::testReferenceValidity()
{
    QFETCH(int, type);
    QFETCH(QString, path);
    QFETCH(QUuid, uuid);
    QFETCH(bool, valid);

    KeeShareSettings::Reference reference;
    reference.type = static_cast<KeeShareSettings::Type>(type);
    reference.path = path;
    reference.uuid = uuid;
    QCOMPARE(reference.isValid(), valid);
}

void TestSharing::testObserverRejectsInvalidReference()
{
    auto db = QSharedPointer<Database>::create();
    KeeShareSettings::Reference reference;
    reference.type = KeeShareSettings::ImportFrom;
    reference.path = "share.kdbx";
    reference.uuid = QUuid();
    KeeShare::setReferenceTo(db->rootGroup(), reference);

    KeeShareSettings::Active active;
    active.in = true;
    KeeShare::setActive(active);
    ShareObserver observer(db);
    int messages = 0;
    connect(&observer, &ShareObserver::sharingMessage, this, [&messages] { ++messages; });
    emit db->groupDataChanged(db->rootGroup());
    QCOMPARE(messages, 0);
    QVERIFY(observer.findChildren<FileWatcher*>().isEmpty());
}

void TestSharing::testObserverReinitializeReentry()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto db = QSharedPointer<Database>::create();
    db->setFilePath(directory.filePath("outer.kdbx"));

    KeeShareSettings::Active active;
    active.in = true;
    KeeShare::setActive(active);
    ShareObserver observer(db);

    KeeShareSettings::Reference reference;
    reference.type = KeeShareSettings::ImportFrom;
    reference.path = "missing-first.kdbx";
    KeeShare::setReferenceTo(db->rootGroup(), reference);

    int messages = 0;
    connect(&observer, &ShareObserver::sharingMessage, this, [&] {
        if (++messages == 1) {
            // Change the reference so a nested scan would attempt another import.
            reference.path = "missing-second.kdbx";
            KeeShare::setReferenceTo(db->rootGroup(), reference);
            emit db->groupDataChanged(db->rootGroup());
        }
    });

    emit db->groupDataChanged(db->rootGroup());
    QCOMPARE(messages, 1);
    // The scoped guard must allow subsequent, non-recursive scans.
    emit db->groupDataChanged(db->rootGroup());
    QCOMPARE(messages, 2);
}

void TestSharing::testSettingsSerialization()
{
    QFETCH(bool, importing);
    QFETCH(bool, exporting);
    QFETCH(KeeShareSettings::Certificate, ownCertificate);
    QFETCH(KeeShareSettings::Key, ownKey);

    KeeShareSettings::Own originalOwn;
    KeeShareSettings::Active originalActive;
    originalActive.in = importing;
    originalActive.out = exporting;
    originalOwn.certificate = ownCertificate;
    originalOwn.key = ownKey;

    const QString serializedActive = KeeShareSettings::Active::serialize(originalActive);
    KeeShareSettings::Active restoredActive = KeeShareSettings::Active::deserialize(serializedActive);

    const QString serializedOwn = KeeShareSettings::Own::serialize(originalOwn);
    KeeShareSettings::Own restoredOwn = KeeShareSettings::Own::deserialize(serializedOwn);

    QCOMPARE(restoredActive.in, importing);
    QCOMPARE(restoredActive.out, exporting);
    if (ownCertificate.key) {
        QCOMPARE(restoredOwn.certificate, ownCertificate);
    }
    if (ownKey.key) {
        QCOMPARE(restoredOwn.key, ownKey);
    }
}

void TestSharing::testSettingsSerialization_data()
{
    auto sshKey0 = stubkey(0);

    KeeShareSettings::Certificate certificate0;
    certificate0.key = sshKey0;
    certificate0.signer = "signer";

    KeeShareSettings::Key key0;
    key0.key = sshKey0;

    QTest::addColumn<bool>("importing");
    QTest::addColumn<bool>("exporting");
    QTest::addColumn<KeeShareSettings::Certificate>("ownCertificate");
    QTest::addColumn<KeeShareSettings::Key>("ownKey");

    QTest::newRow("1") << false << false << KeeShareSettings::Certificate() << KeeShareSettings::Key();
    QTest::newRow("2") << true << false << KeeShareSettings::Certificate() << KeeShareSettings::Key();
    QTest::newRow("3") << true << true << KeeShareSettings::Certificate() << KeeShareSettings::Key();
    QTest::newRow("4") << false << true << certificate0 << key0;
    QTest::newRow("5") << false << false << certificate0 << key0;
}

const QSharedPointer<Botan::RSA_PrivateKey> TestSharing::stubkey(int index)
{
    static QMap<int, QSharedPointer<Botan::RSA_PrivateKey>> keys;
    if (!keys.contains(index)) {
        keys.insert(index,
                    QSharedPointer<Botan::RSA_PrivateKey>(new Botan::RSA_PrivateKey(*randomGen()->getRng(), 2048)));
    }
    return keys[index];
}
