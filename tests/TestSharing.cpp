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

#include <QTest>
#include <QXmlStreamReader>

#include "crypto/Crypto.h"
#include "crypto/Random.h"
#include "keeshare/KeeShareSettings.h"

#include <botan/rsa.h>

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
    auto key = stubkey();
    KeeShareSettings::ScopedCertificate original;
    original.path = "/path";
    original.certificate = KeeShareSettings::Certificate{key, "Some <!> &#_\"\" weird string"};
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

    QCOMPARE(restored.certificate.key->private_key_bits(), original.certificate.key->private_key_bits());
    QCOMPARE(restored.certificate.signer, original.certificate.signer);
    QCOMPARE(restored.trust, original.trust);
    QCOMPARE(restored.path, original.path);

    QCOMPARE(restored.certificate.key->public_key_bits(), key->public_key_bits());
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
    QVERIFY(reader.name() == "Key");
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
    if (ownCertificate.key) {
        QCOMPARE(restoredOwn.certificate, ownCertificate);
    }
    if (ownKey.key) {
        QCOMPARE(restoredOwn.key, ownKey);
    }
    QCOMPARE(restoredForeign.certificates.count(), foreignCertificates.count());
    for (int i = 0; i < foreignCertificates.count(); ++i) {
        QCOMPARE(restoredForeign.certificates[i].certificate, foreignCertificates[i].certificate);
    }
}

void TestSharing::testSettingsSerialization_data()
{
    auto sshKey0 = stubkey(0);
    KeeShareSettings::ScopedCertificate certificate0;
    certificate0.path = "/path/0";
    certificate0.certificate = KeeShareSettings::Certificate{sshKey0, "Some <!> &#_\"\" weird string"};
    certificate0.trust = KeeShareSettings::Trust::Trusted;

    KeeShareSettings::Key key0;
    key0.key = sshKey0;

    auto sshKey1 = stubkey(1);
    KeeShareSettings::ScopedCertificate certificate1;
    certificate1.path = "/path/1";
    certificate1.certificate = KeeShareSettings::Certificate{sshKey1, "Another "};
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

const QSharedPointer<Botan::RSA_PrivateKey> TestSharing::stubkey(int index)
{
    static QMap<int, QSharedPointer<Botan::RSA_PrivateKey>> keys;
    if (!keys.contains(index)) {
        keys.insert(index,
                    QSharedPointer<Botan::RSA_PrivateKey>(new Botan::RSA_PrivateKey(*randomGen()->getRng(), 2048)));
    }
    return keys[index];
}
