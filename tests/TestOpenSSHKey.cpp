/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
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

#include "TestOpenSSHKey.h"
#include "crypto/Crypto.h"
#include "sshagent/OpenSSHKey.h"
#include <QTest>

QTEST_GUILESS_MAIN(TestOpenSSHKey)

void TestOpenSSHKey::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestOpenSSHKey::testParse()
{
    // mixed line endings and missing ones are intentional, we only require 3 lines total
    const QString keyString = QString(
        "\r\n\r"
        "-----BEGIN OPENSSH PRIVATE KEY-----\n"
        "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW"
        "QyNTUxOQAAACDdlO5F2kF2WzedrBAHBi9wBHeISzXZ0IuIqrp0EzeazAAAAKjgCfj94An4"
        "/QAAAAtzc2gtZWQyNTUxOQAAACDdlO5F2kF2WzedrBAHBi9wBHeISzXZ0IuIqrp0EzeazA"
        "AAAEBe1iilZFho8ZGAliiSj5URvFtGrgvmnEKdiLZow5hOR92U7kXaQXZbN52sEAcGL3AE"
        "d4hLNdnQi4iqunQTN5rMAAAAH29wZW5zc2hrZXktdGVzdC1wYXJzZUBrZWVwYXNzeGMBAg"
        "MEBQY=\r"
        "-----END OPENSSH PRIVATE KEY-----\r\n\r"
    );

    const QByteArray keyData = keyString.toLatin1();

    OpenSSHKey key;
    QVERIFY(key.parse(keyData));
    QVERIFY(!key.encrypted());
    QCOMPARE(key.cipherName(), QString("none"));
    QCOMPARE(key.type(), QString("ssh-ed25519"));
    QCOMPARE(key.comment(), QString("opensshkey-test-parse@keepassxc"));

    QByteArray publicKey, privateKey;
    BinaryStream publicStream(&publicKey), privateStream(&privateKey);

    QVERIFY(key.writePublic(publicStream));
    QVERIFY(key.writePrivate(privateStream));

    QVERIFY(publicKey.length() == 51);
    QVERIFY(privateKey.length() == 154);
}

void TestOpenSSHKey::testDecryptAES256CBC()
{
    const QString keyString = QString(
        "-----BEGIN OPENSSH PRIVATE KEY-----\n"
        "b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jYmMAAAAGYmNyeXB0AAAAGAAAABD2A0agtd\n"
        "oGtJiI9JvIxYbTAAAAEAAAAAEAAAAzAAAAC3NzaC1lZDI1NTE5AAAAIDPvDXmi0w1rdMoX\n"
        "fOeyZ0Q/v+wqq/tPFgJwxnW5ADtfAAAAsC3UPsf035hrF5SgZ48p55iDFPiyGfZC/C3vQx\n"
        "+THzpQo8DTUmFokdPn8wvDYGQoIcr9q0RzJuKV87eMQf3zzvZfJthtLYBlt330Deivv9AQ\n"
        "MbKdhPZ4SfwRvv0grgT2EVId3GQAPgSVBhXYQTOf2CdmbXV4kieFLTmSsBMy+v6Qn5Rqur\n"
        "PDWBwuLQgamcVDZuhrkUEqIVJZU2zAiRU2oAXsw/XOgFV6+Y5UZmLwWJQZ\n"
        "-----END OPENSSH PRIVATE KEY-----\n"
    );

    const QByteArray keyData = keyString.toLatin1();

    OpenSSHKey key;
    QVERIFY(key.parse(keyData));
    QVERIFY(key.encrypted());
    QCOMPARE(key.cipherName(), QString("aes256-cbc"));
    QVERIFY(!key.openPrivateKey("incorrectpassphrase"));
    QVERIFY(key.openPrivateKey("correctpassphrase"));
    QCOMPARE(key.type(), QString("ssh-ed25519"));
    QCOMPARE(key.comment(), QString("opensshkey-test-aes256cbc@keepassxc"));

    QByteArray publicKey, privateKey;
    BinaryStream publicStream(&publicKey), privateStream(&privateKey);

    QVERIFY(key.writePublic(publicStream));
    QVERIFY(key.writePrivate(privateStream));

    QVERIFY(publicKey.length() == 51);
    QVERIFY(privateKey.length() == 158);
}

void TestOpenSSHKey::testDecryptAES256CTR()
{
    const QString keyString = QString(
        "-----BEGIN OPENSSH PRIVATE KEY-----\n"
        "b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jdHIAAAAGYmNyeXB0AAAAGAAAABAMhIAypt\n"
        "WP4tZJBmMwq0tTAAAAEAAAAAEAAAAzAAAAC3NzaC1lZDI1NTE5AAAAIErNsS8ROy43XoWC\n"
        "nO9Sn2lEFBJYcDVtRPM1t6WB7W7OAAAAsFKXMOlPILoTmMj2JmcqzjaYAhaCezx18HDp76\n"
        "VrNxaZTd0T28EGFSkzrReeewpJWy/bWlhLoXR5fRyOSSto+iMg/pibIvIJMrD5sqxlxr/e\n"
        "c5lSeSZUzIK8Rv+ou/3EFDcY5jp8hVXqA4qNtoM/3fV52vmwlNje5d1V5Gsr4U8443+i+p\n"
        "swqksozfatkynk51uR/9QFoOJKlsL/Z3LkK1S/apYz/K331iU1f5ozFELf\n"
        "-----END OPENSSH PRIVATE KEY-----\n"
    );

    const QByteArray keyData = keyString.toLatin1();

    OpenSSHKey key;
    QVERIFY(key.parse(keyData));
    QVERIFY(key.encrypted());
    QCOMPARE(key.cipherName(), QString("aes256-ctr"));
    QVERIFY(!key.openPrivateKey("incorrectpassphrase"));
    QVERIFY(key.openPrivateKey("correctpassphrase"));
    QCOMPARE(key.type(), QString("ssh-ed25519"));
    QCOMPARE(key.comment(), QString("opensshkey-test-aes256ctr@keepassxc"));

    QByteArray publicKey, privateKey;
    BinaryStream publicStream(&publicKey), privateStream(&privateKey);

    QVERIFY(key.writePublic(publicStream));
    QVERIFY(key.writePrivate(privateStream));

    QVERIFY(publicKey.length() == 51);
    QVERIFY(privateKey.length() == 158);
}
