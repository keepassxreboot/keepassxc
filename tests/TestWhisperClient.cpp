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

#include "TestWhisperClient.h"

#include "core/Entry.h"
#include "crypto/Crypto.h"
#include "crypto/SymmetricCipher.h"
#include "networking/WhisperClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

QTEST_GUILESS_MAIN(TestWhisperClient)

namespace
{
    QByteArray decrypt(const QByteArray& key, const QByteArray& payload)
    {
        QByteArray data = payload.mid(12);
        SymmetricCipher cipher;
        if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, payload.left(12))
            || !cipher.finish(data)) {
            return {};
        }
        return data;
    }
} // namespace

void TestWhisperClient::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestWhisperClient::testBuildPayloadSingleField()
{
    Entry entry;
    entry.setPassword("hunter2");

    WhisperClient::Fields fields;
    fields.password = true;
    QCOMPARE(WhisperClient::buildPayload(&entry, fields), QString("hunter2"));
}

void TestWhisperClient::testBuildPayloadMultipleFields()
{
    Entry entry;
    entry.setUsername("alice");
    entry.setPassword("hunter2");
    entry.setUrl("https://example.test");
    entry.setNotes("hello");

    WhisperClient::Fields fields;
    fields.username = true;
    fields.password = true;
    fields.url = true;
    fields.notes = true;
    QCOMPARE(WhisperClient::buildPayload(&entry, fields),
             QString(R"({"username":"alice","password":"hunter2","url":"https://example.test","notes":"hello"})"));
}

void TestWhisperClient::testBuildPayloadEscapesJson()
{
    Entry entry;
    entry.setUsername("al\"ice\\");
    entry.setNotes("line1\nline2\t😀");

    WhisperClient::Fields fields;
    fields.username = true;
    fields.notes = true;

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(WhisperClient::buildPayload(&entry, fields).toUtf8(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(doc["username"].toString(), QString("al\"ice\\"));
    QCOMPARE(doc["notes"].toString(), QString("line1\nline2\t😀"));
}

void TestWhisperClient::testBuildPayloadSkipsEmptyAndUnselected()
{
    Entry entry;
    entry.setUsername("alice");
    entry.setNotes("a note");

    WhisperClient::Fields fields;
    fields.username = true;
    fields.password = true;
    fields.notes = true;
    QCOMPARE(WhisperClient::buildPayload(&entry, fields), QString(R"({"username":"alice","notes":"a note"})"));

    WhisperClient::Fields onlyEmpty;
    onlyEmpty.password = true;
    QVERIFY(WhisperClient::buildPayload(&entry, onlyEmpty).isEmpty());
}

void TestWhisperClient::testBuildPayloadResolvesPlaceholders()
{
    Entry entry;
    entry.setUsername("alice");
    entry.setPassword("{USERNAME}-secret");

    WhisperClient::Fields fields;
    fields.password = true;
    QCOMPARE(WhisperClient::buildPayload(&entry, fields), QString("alice-secret"));
}

void TestWhisperClient::testBuildPayloadUnicode()
{
    Entry entry;
    entry.setPassword("pässwörd-😀");

    WhisperClient::Fields fields;
    fields.password = true;
    QCOMPARE(WhisperClient::buildPayload(&entry, fields), QString("pässwörd-😀"));
}

void TestWhisperClient::testEncryptRoundTrip()
{
    const QString plaintext = R"({"username":"alice","password":"pässwörd-😀"})";
    QByteArray key;
    QByteArray payload;
    QVERIFY(WhisperClient::encrypt(plaintext, key, payload));

    const auto utf8 = plaintext.toUtf8();
    QCOMPARE(key.size(), 32);
    QCOMPARE(payload.size(), 12 + utf8.size() + 16);
    QVERIFY(!payload.contains(utf8));
    QCOMPARE(decrypt(key, payload), utf8);

    QByteArray key2;
    QByteArray payload2;
    QVERIFY(WhisperClient::encrypt(plaintext, key2, payload2));
    QVERIFY(key != key2);
    QVERIFY(payload.left(12) != payload2.left(12));
}

void TestWhisperClient::testEncryptTamperedPayloadFails()
{
    QByteArray key;
    QByteArray payload;
    QVERIFY(WhisperClient::encrypt("hunter2", key, payload));

    payload[12] = static_cast<char>(payload[12] ^ 0x01);
    QVERIFY(decrypt(key, payload).isEmpty());
}

void TestWhisperClient::testParseSecretId()
{
    QCOMPARE(WhisperClient::parseSecretId(R"({"id":"0b9a3e6c-4f1d-4c3a-9f2e-7d8b6a5c4e3f"})"),
             QString("0b9a3e6c-4f1d-4c3a-9f2e-7d8b6a5c4e3f"));
}

void TestWhisperClient::testParseSecretIdInvalid_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::newRow("empty") << QByteArray();
    QTest::newRow("not json") << QByteArray("<html>oops</html>");
    QTest::newRow("missing id") << QByteArray(R"({"secret":"x"})");
    QTest::newRow("empty id") << QByteArray(R"({"id":""})");
    QTest::newRow("not a uuid") << QByteArray(R"({"id":"../../evil?x=1"})");
}

void TestWhisperClient::testParseSecretIdInvalid()
{
    QFETCH(QByteArray, body);
    QVERIFY(WhisperClient::parseSecretId(body).isEmpty());
}

void TestWhisperClient::testBuildShareUrl()
{
    // 0xff bytes need '/' and '=' padding in standard base64, neither may appear
    const QByteArray key(32, '\xff');
    const QString id = "0b9a3e6c-4f1d-4c3a-9f2e-7d8b6a5c4e3f";
    const QString expected =
        "https://whisper.example.test/get_secret?shared_secret_id=" + id + "#k=" + QString(42, '_') + "8";

    QCOMPARE(WhisperClient::buildShareUrl("https://whisper.example.test", id, key).toString(), expected);
    QCOMPARE(WhisperClient::buildShareUrl("https://whisper.example.test/", id, key).toString(), expected);
}

void TestWhisperClient::testShareRequiresHttps()
{
    WhisperClient client;
    QSignalSpy failed(&client, &WhisperClient::shareFailed);
    client.shareSecret("http://whisper.example.test", "hunter2", 3600, true);
    QCOMPARE(failed.count(), 1);
}
