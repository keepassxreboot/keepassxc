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

#include "TestSharing.h"
#include "TestGlobal.h"
#include "stub/TestRandom.h"

#include <QBuffer>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "config-keepassx-tests.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "crypto/Random.h"
#include "crypto/ssh/OpenSSHKey.h"
#include "format/KeePass2Writer.h"
#include "keeshare/KeeShareSettings.h"
#include "keys/PasswordKey.h"

#include <format/KeePass2Reader.h>

QTEST_GUILESS_MAIN(TestSharing)

Q_DECLARE_METATYPE(KeeShareSettings::Type)
Q_DECLARE_METATYPE(KeeShareSettings::Key)
Q_DECLARE_METATYPE(KeeShareSettings::Certificate)
Q_DECLARE_METATYPE(KeeShareSettings::Trust)
Q_DECLARE_METATYPE(KeeShareSettings::ScopedCertificate)
Q_DECLARE_METATYPE(QList<KeeShareSettings::ScopedCertificate>)

void TestSharing::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestSharing::cleanupTestCase()
{
    TestRandom::teardown();
}

void TestSharing::testIdempotentDatabaseWriting()
{
    QScopedPointer<Database> db(new Database());
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("password"));
    db->setKey(key);

    Group* sharingGroup = new Group();
    sharingGroup->setName("SharingGroup");
    sharingGroup->setUuid(QUuid::createUuid());
    sharingGroup->setParent(db->rootGroup());

    Entry* entry1 = new Entry();
    entry1->setUuid(QUuid::createUuid());
    entry1->beginUpdate();
    entry1->setTitle("Entry1");
    entry1->endUpdate();
    entry1->setGroup(sharingGroup);

    Entry* entry2 = new Entry();
    entry2->setUuid(QUuid::createUuid());
    entry2->beginUpdate();
    entry2->setTitle("Entry2");
    entry2->endUpdate();
    entry2->setGroup(sharingGroup);

    // prevent from changes introduced by randomization
    TestRandom::setup(new RandomBackendNull());

    QByteArray bufferOriginal;
    {
        QBuffer device(&bufferOriginal);
        device.open(QIODevice::ReadWrite);
        KeePass2Writer writer;
        writer.writeDatabase(&device, db.data());
    }

    QByteArray bufferCopy;
    {
        QBuffer device(&bufferCopy);
        device.open(QIODevice::ReadWrite);
        KeePass2Writer writer;
        writer.writeDatabase(&device, db.data());
    }

    QCOMPARE(bufferCopy, bufferOriginal);
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

    const auto foreign = KeeShareSettings::Foreign();
    QVERIFY(foreign.certificates.isEmpty());
    const auto xmlForeign = KeeShareSettings::Foreign::deserialize(empty);
    QVERIFY(xmlForeign.certificates.isEmpty());

    const auto reference = KeeShareSettings::Reference();
    QVERIFY(reference.isNull());
    const auto xmlReference = KeeShareSettings::Reference::deserialize(empty);
    QVERIFY(xmlReference.isNull());
}

void TestSharing::testCertificateSerialization()
{
    QFETCH(KeeShareSettings::Trust, trusted);
    const OpenSSHKey& key = stubkey();
    KeeShareSettings::ScopedCertificate original;
    original.path = "/path";
    original.certificate = KeeShareSettings::Certificate{OpenSSHKey::serializeToBinary(OpenSSHKey::Public, key),
                                                         "Some <!> &#_\"\" weird string"};
    original.trust = trusted;

    QString buffer;
    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeStartElement("Certificate");
    KeeShareSettings::ScopedCertificate::serialize(writer, original);
    writer.writeEndElement();
    writer.writeEndDocument();
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    QVERIFY(reader.name() == "Certificate");
    KeeShareSettings::ScopedCertificate restored = KeeShareSettings::ScopedCertificate::deserialize(reader);

    QCOMPARE(restored.certificate.key, original.certificate.key);
    QCOMPARE(restored.certificate.signer, original.certificate.signer);
    QCOMPARE(restored.trust, original.trust);
    QCOMPARE(restored.path, original.path);

    QCOMPARE(restored.certificate.sshKey().publicParts(), key.publicParts());
}

void TestSharing::testCertificateSerialization_data()
{
    QTest::addColumn<KeeShareSettings::Trust>("trusted");
    QTest::newRow("Ask") << KeeShareSettings::Trust::Ask;
    QTest::newRow("Trusted") << KeeShareSettings::Trust::Trusted;
    QTest::newRow("Untrusted") << KeeShareSettings::Trust::Untrusted;
}

void TestSharing::testKeySerialization()
{
    const OpenSSHKey& key = stubkey();
    KeeShareSettings::Key original;
    original.key = OpenSSHKey::serializeToBinary(OpenSSHKey::Private, key);

    QString buffer;
    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeStartElement("Key");
    KeeShareSettings::Key::serialize(writer, original);
    writer.writeEndElement();
    writer.writeEndDocument();
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    QVERIFY(reader.name() == "Key");
    KeeShareSettings::Key restored = KeeShareSettings::Key::deserialize(reader);

    QCOMPARE(restored.key, original.key);
    QCOMPARE(restored.sshKey().privateParts(), key.privateParts());
    QCOMPARE(restored.sshKey().type(), key.type());
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
    QTest::newRow("1") << "Password"
                       << "/some/path" << QUuid::createUuid() << int(KeeShareSettings::Inactive);
    QTest::newRow("2") << ""
                       << "" << QUuid() << int(KeeShareSettings::SynchronizeWith);
    QTest::newRow("3") << ""
                       << "/some/path" << QUuid() << int(KeeShareSettings::ExportTo);
}

void TestSharing::testSettingsSerialization()
{
    QFETCH(bool, importing);
    QFETCH(bool, exporting);
    QFETCH(KeeShareSettings::Certificate, ownCertificate);
    QFETCH(KeeShareSettings::Key, ownKey);
    QFETCH(QList<KeeShareSettings::ScopedCertificate>, foreignCertificates);

    KeeShareSettings::Own originalOwn;
    KeeShareSettings::Foreign originalForeign;
    KeeShareSettings::Active originalActive;
    originalActive.in = importing;
    originalActive.out = exporting;
    originalOwn.certificate = ownCertificate;
    originalOwn.key = ownKey;
    originalForeign.certificates = foreignCertificates;

    const QString serializedActive = KeeShareSettings::Active::serialize(originalActive);
    KeeShareSettings::Active restoredActive = KeeShareSettings::Active::deserialize(serializedActive);

    const QString serializedOwn = KeeShareSettings::Own::serialize(originalOwn);
    KeeShareSettings::Own restoredOwn = KeeShareSettings::Own::deserialize(serializedOwn);

    const QString serializedForeign = KeeShareSettings::Foreign::serialize(originalForeign);
    KeeShareSettings::Foreign restoredForeign = KeeShareSettings::Foreign::deserialize(serializedForeign);

    QCOMPARE(restoredActive.in, importing);
    QCOMPARE(restoredActive.out, exporting);
    QCOMPARE(restoredOwn.certificate.key, ownCertificate.key);
    QCOMPARE(restoredOwn.key.key, ownKey.key);
    QCOMPARE(restoredForeign.certificates.count(), foreignCertificates.count());
    for (int i = 0; i < foreignCertificates.count(); ++i) {
        QCOMPARE(restoredForeign.certificates[i].certificate.key, foreignCertificates[i].certificate.key);
    }
}

void TestSharing::testSettingsSerialization_data()
{
    const OpenSSHKey& sshKey0 = stubkey(0);
    KeeShareSettings::ScopedCertificate certificate0;
    certificate0.path = "/path/0";
    certificate0.certificate = KeeShareSettings::Certificate{OpenSSHKey::serializeToBinary(OpenSSHKey::Public, sshKey0),
                                                             "Some <!> &#_\"\" weird string"};
    certificate0.trust = KeeShareSettings::Trust::Trusted;

    KeeShareSettings::Key key0;
    key0.key = OpenSSHKey::serializeToBinary(OpenSSHKey::Private, sshKey0);

    const OpenSSHKey& sshKey1 = stubkey(1);
    KeeShareSettings::ScopedCertificate certificate1;
    certificate1.path = "/path/1";
    certificate1.certificate =
        KeeShareSettings::Certificate{OpenSSHKey::serializeToBinary(OpenSSHKey::Public, sshKey1), "Another "};
    certificate1.trust = KeeShareSettings::Trust::Untrusted;

    QTest::addColumn<bool>("importing");
    QTest::addColumn<bool>("exporting");
    QTest::addColumn<KeeShareSettings::Certificate>("ownCertificate");
    QTest::addColumn<KeeShareSettings::Key>("ownKey");
    QTest::addColumn<QList<KeeShareSettings::ScopedCertificate>>("foreignCertificates");
    QTest::newRow("1") << false << false << KeeShareSettings::Certificate() << KeeShareSettings::Key()
                       << QList<KeeShareSettings::ScopedCertificate>();
    QTest::newRow("2") << true << false << KeeShareSettings::Certificate() << KeeShareSettings::Key()
                       << QList<KeeShareSettings::ScopedCertificate>();
    QTest::newRow("3") << true << true << KeeShareSettings::Certificate() << KeeShareSettings::Key()
                       << QList<KeeShareSettings::ScopedCertificate>({certificate0, certificate1});
    QTest::newRow("4") << false << true << certificate0.certificate << key0
                       << QList<KeeShareSettings::ScopedCertificate>();
    QTest::newRow("5") << false << false << certificate0.certificate << key0
                       << QList<KeeShareSettings::ScopedCertificate>({certificate1});
}

const OpenSSHKey& TestSharing::stubkey(int index)
{
    static QMap<int, OpenSSHKey*> keys;
    if (!keys.contains(index)) {
        OpenSSHKey* key = new OpenSSHKey(OpenSSHKey::generate(false));
        key->setParent(this);
        keys[index] = key;
    }
    return *keys[index];
}
