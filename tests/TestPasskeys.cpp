/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "TestPasskeys.h"
#include "browser/BrowserCbor.h"
#include "browser/BrowserMessageBuilder.h"
#include "browser/BrowserService.h"
#include "crypto/Crypto.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>
#include <botan/sodium.h>

using namespace Botan::Sodium;

QTEST_GUILESS_MAIN(TestPasskeys)

// Register request
// clang-format off
const QString PublicKeyCredentialOptions = R"(
    {
        "attestation": "none",
        "authenticatorSelection": {
            "residentKey": "preferred",
            "requireResidentKey": false,
            "userVerification": "required"
        },
        "challenge": "lVeHzVxWsr8MQxMkZF0ti6FXhdgMljqKzgA-q_zk2Mnii3eJ47VF97sqUoYktVC85WAZ1uIASm-a_lDFZwsLfw",
        "pubKeyCredParams": [
            {
                "type": "public-key",
                "alg": -7
            },
            {
                "type": "public-key",
                "alg": -257
            }
        ],
        "rp": {
            "name": "webauthn.io",
            "id": "webauthn.io"
        },
        "timeout": 60000,
        "excludeCredentials": [],
        "user": {
            "displayName": "Test User",
            "id": "VkdWemRDQlZjMlZ5",
            "name": "Test User"
        }
    }
)";

// Register response
const QString PublicKeyCredential = R"(
    {
        "authenticatorAttachment": "platform",
        "id": "yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8",
        "rawId": "cabcc52799707294f060c39d5d29b11796f9718425a813336db53f77ea052cef",
        "response": {
            "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVikdKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvBFAAAAAAECAwQFBgcIAQIDBAUGBwgAIMq8xSeZcHKU8GDDnV0psReW-XGEJagTM221P3fqBSzvpQECAyYgASFYIAbsrzRbYpFhbRlZA6ZQKsoxxJWoaeXwh-XUuDLNCIXdIlgg4u5_6Q8O6R0Hg0oDCdtCJLEL0yX_GDLhU5m3HUIE54M",
            "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoibFZlSHpWeFdzcjhNUXhNa1pGMHRpNkZYaGRnTWxqcUt6Z0EtcV96azJNbmlpM2VKNDdWRjk3c3FVb1lrdFZDODVXQVoxdUlBU20tYV9sREZad3NMZnciLCJvcmlnaW4iOiJodHRwczovL3dlYmF1dGhuLmlvIiwiY3Jvc3NPcmlnaW4iOmZhbHNlfQ"
        },
        "type": "public-key"
    }
)";

// Get request
const QString PublicKeyCredentialRequestOptions = R"(
    {
        "allowCredentials": [
            {
                "id": "yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8",
                "transports": ["internal"],
                "type": "public-key"
            }
        ],
        "challenge": "9z36vTfQTL95Lf7WnZgyte7ohGeF-XRiLxkL-LuGU1zopRmMIUA1LVwzGpyIm1fOBn1QnRa0QH27ADAaJGHysQ",
        "rpId": "webauthn.io",
        "timeout": 60000,
        "userVerification": "required"
    }
)";
// clang-format on

void TestPasskeys::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestPasskeys::init()
{
}

void TestPasskeys::testBase64WithHexStrings()
{
    const size_t bufSize = 64;
    unsigned char buf[bufSize] = {31,  141, 30,  29,  142, 73,  5,   239, 242, 84,  187, 202, 40,  54,  15,  223,
                                  201, 0,   108, 109, 209, 104, 207, 239, 160, 89,  208, 117, 134, 66,  42,  12,
                                  31,  66,  163, 248, 221, 88,  241, 164, 6,   55,  182, 97,  186, 243, 162, 162,
                                  81,  220, 55,  60,  93,  207, 170, 222, 56,  234, 227, 45,  115, 175, 138, 182};

    auto base64FromArray = browserMessageBuilder()->getBase64FromArray(reinterpret_cast<const char*>(buf), bufSize);
    QCOMPARE(base64FromArray,
             QString("H40eHY5JBe_yVLvKKDYP38kAbG3RaM_voFnQdYZCKgwfQqP43VjxpAY3tmG686KiUdw3PF3Pqt446uMtc6-Ktg"));

    auto arrayFromBase64 = browserMessageBuilder()->getArrayFromBase64(base64FromArray);
    QCOMPARE(arrayFromBase64.size(), bufSize);

    for (size_t i = 0; i < bufSize; i++) {
        QCOMPARE(static_cast<unsigned char>(arrayFromBase64.at(i)), buf[i]);
    }

    auto randomDataBase64 = browserMessageBuilder()->getRandomBytesAsBase64(24);
    QCOMPARE(randomDataBase64.isEmpty(), false);
}

void TestPasskeys::testDecodeResponseData()
{
    const auto publicKeyCredential = browserMessageBuilder()->getJsonObject(PublicKeyCredential.toUtf8());
    auto response = publicKeyCredential["response"].toObject();
    auto clientDataJson = response["clientDataJSON"].toString();
    auto attestationObject = response["attestationObject"].toString();

    QVERIFY(!clientDataJson.isEmpty());
    QVERIFY(!attestationObject.isEmpty());

    // Parse clientDataJSON
    auto clientDataByteArray = browserMessageBuilder()->getArrayFromBase64(clientDataJson);
    auto clientDataJsonObject = browserMessageBuilder()->getJsonObject(clientDataByteArray);
    QCOMPARE(clientDataJsonObject["challenge"],
             QString("lVeHzVxWsr8MQxMkZF0ti6FXhdgMljqKzgA-q_zk2Mnii3eJ47VF97sqUoYktVC85WAZ1uIASm-a_lDFZwsLfw"));
    QCOMPARE(clientDataJsonObject["origin"], QString("https://webauthn.io"));
    QCOMPARE(clientDataJsonObject["type"], QString("webauthn.create"));

    // Parse attestationObject (CBOR decoding needed)
    BrowserCbor browserCbor;
    auto attestationByteArray = browserMessageBuilder()->getArrayFromBase64(attestationObject);
    auto attestationJsonObject = browserCbor.getJsonFromCborData(attestationByteArray);

    // Parse authData
    auto authDataJsonObject = attestationJsonObject["authData"].toString();
    auto authDataArray = browserMessageBuilder()->getArrayFromBase64(authDataJsonObject);
    QVERIFY(authDataArray.size() >= 37);

    auto authData = browserPasskeys()->parseAuthData(authDataArray);
    auto credentialData = authData["credentialData"].toObject();
    auto flags = authData["flags"].toObject();
    auto publicKey = credentialData["publicKey"].toObject();

    // The attestationObject should include the same ID after decoding with the response root
    QCOMPARE(credentialData["credentialId"].toString(), publicKeyCredential["id"].toString());
    QCOMPARE(credentialData["aaguid"].toString(), QString("AQIDBAUGBwgBAgMEBQYHCA"));
    QCOMPARE(authData["rpIdHash"].toString(), QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvA"));
    QCOMPARE(flags["AT"], true);
    QCOMPARE(flags["UP"], true);
    QCOMPARE(publicKey["1"], 2);
    QCOMPARE(publicKey["3"], -7);
    QCOMPARE(publicKey["-1"], 1);
    QCOMPARE(publicKey["-2"], QString("BuyvNFtikWFtGVkDplAqyjHElahp5fCH5dS4Ms0Ihd0"));
    QCOMPARE(publicKey["-3"], QString("4u5_6Q8O6R0Hg0oDCdtCJLEL0yX_GDLhU5m3HUIE54M"));
}

void TestPasskeys::testLoadingECPrivateKeyFromPem()
{
    const auto publicKeyCredentialRequestOptions =
        browserMessageBuilder()->getJsonObject(PublicKeyCredentialRequestOptions.toUtf8());
    const auto privateKeyPem = QString("-----BEGIN PRIVATE KEY-----"
                                       "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg5DX2R6I37nMSZqCp"
                                       "XfHlE3UeitkGGE03FqGsdfxIBoOhRANCAAQG7K80W2KRYW0ZWQOmUCrKMcSVqGnl"
                                       "8Ifl1LgyzQiF3eLuf+kPDukdB4NKAwnbQiSxC9Ml/xgy4VOZtx1CBOeD"
                                       "-----END PRIVATE KEY-----");

    const auto authenticatorData =
        browserMessageBuilder()->getArrayFromBase64("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvAFAAAAAA");
    const auto clientData = browserMessageBuilder()->getArrayFromBase64(
        "eyJ0eXBlIjoid2ViYXV0aG4uZ2V0IiwiY2hhbGxlbmdlIjoiOXozNnZUZlFUTDk1TGY3V25aZ3l0ZTdvaEdlRi1YUmlMeGtMLUx1R1Uxem9wUm"
        "1NSVVBMUxWd3pHcHlJbTFmT0JuMVFuUmEwUUgyN0FEQWFKR0h5c1EiLCJvcmlnaW4iOiJodHRwczovL3dlYmF1dGhuLmlvIiwiY3Jvc3NPcmln"
        "aW4iOmZhbHNlfQ");

    const auto signature = browserPasskeys()->buildSignature(authenticatorData, clientData, privateKeyPem);
    QCOMPARE(
        browserMessageBuilder()->getBase64FromArray(signature),
        QString("MEYCIQCpbDaYJ4b2ofqWBxfRNbH3XCpsyao7Iui5lVuJRU9HIQIhAPl5moNZgJu5zmurkKK_P900Ct6wd3ahVIqCEqTeeRdE"));
}

void TestPasskeys::testLoadingRSAPrivateKeyFromPem()
{
    const auto privateKeyPem = QString("-----BEGIN PRIVATE KEY-----"
                                       "MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQC5OHjBHQaRfxxX\n4WHRmqq7e7JgT"
                                       "FRs1bd4dIOFAOZnhNE3vAg2IF5VurmeB+9ye9xh7frw8ubrL0cv\nsBWiJfN5CY3SYGRLbGTtBC0fZ6"
                                       "OhhhjwvVM1GW6nVeRU66atzuo4NBfYXJWIYECd\npRBU4+xsDL4vJnn1mj05+v/Tqp6Uo1HrEPx9+Dc"
                                       "oYJD+cw7+OQ83XeGmjD+Dtm5z\nNIyYdweaafVR4PEUlB3CYZuOq9xcpxay3ps2MuYT1zGoiQqk6fla"
                                       "d+0tBWGY8Lwp\nCVulXCv7ljNJ4gxgQtOqWX8j2hC0hBxeqNYDYbrkECid3TsMTEMcV5uaVJXULg4t"
                                       "\nn6UItA11AgMBAAECggEAC3B0WBxHuieIfllOOOC4H9/7S7fDH2f7+W2cFtQ6pqo9\nCq0WBmkYMmw"
                                       "Xx9hpHoq4TnhhHyL9WzPzuKYD0Vx4gvacV/ckkppFScnQKJ2hF+99\nLax1DbU+UImSknfDDFPYbYld"
                                       "b1CD2rpJG1i6X2fRQ6NuK+F7jE05mqcIyE+ZajK+\nIpx8XFmE+tI1EEWsn3CzxMLiTQfXyFt/drM9i"
                                       "GYfcDjYY+q5vzGU3Kxj68gjc96A\nOra79DGOmwX+4zIwo5sSzI3noHnhWPLsaRtE5jWu21Qkb+1BvB"
                                       "jPmbQfN274OQfy\n8/BNNR/NZM1mJm/8x4Mt+h5d946XlIo0AkyYZXY/UQKBgQDYI3G3BCwaYk6MDMe"
                                       "T\nIamRZo25phPtr3if47dhT2xFWJplIt35sW+6KjD6c1Qpb2aGOUh7JPmb57H54OgJ\nmojkS5tv9Y"
                                       "EQZFfgCCZoeuqBx+ArqtJdkXOiNEFS0dpt44I+eO3Do5pnwKRemH+Y\ncqJ/eMH96UMzYDO7WNsyOyo"
                                       "5UQKBgQDbYU0KbGbTrMEV4T9Q41rZ2TnWzs5moqn2\nCRtB8LOdKAZUG7FRsw5KgC1CvFn3Xuk+qphY"
                                       "GUQeJvv7FjxMRUv4BktNpXju6eUj\n3tWHzI2QOkHaeq/XibwbNomfkdyTjtLX2+v8DBHcZnCSlukxc"
                                       "JISyPqZ6CnTjXGE\nEGB+itBI5QKBgQCA+gWttOusguVkZWvivL+3aH9CPXy+5WsR3o1boE13xDu+Bm"
                                       "R3\n0A5gBTVc/t1GLJf9mMlL0vCwvD5UYoWU1YbC1OtYkCQIaBiYM8TXrCGseF2pMTJ/\na4CZVp10k"
                                       "o3J7W2XYgpgKIzHRQnQ+SeLDT0y3BjHMB9N1SaJsah7/RphQQKBgQCr\nL+4yKAzFOJUjQbVqpT8Lp5"
                                       "qeqJofNOdzef+vIOjHxafKkiF4I0UPlZ276cY6ZfGU\nWQKwHGcvMDSI5fz/d0OksySn3mvT4uhPaV8"
                                       "urMv6s7sXhY0Zn/0NLy2NOwDolBar\nIo2vDKwTVEyb1u75CWKzDemfl66ryj++Uhk6JZAKkQKBgQCc"
                                       "NYVe7m648DzD0nu9\n3lgetBTaAS1zZmMs8Cinj44v0ksfqxrRBzBZcO9kCQqiJZ7uCAaVYcQ+PwkY+"
                                       "05C\n+w1+KvdGcKM+8TQYTQM3s2B9IyKExRS/dbQf9F7stJL+k5vbt6OUerwfmbNI9R3t\ngDZ4DEfo"
                                       "pPivs9dnequ9wfaPOw=="
                                       "-----END PRIVATE KEY-----");

    const auto authenticatorData =
        browserMessageBuilder()->getArrayFromBase64("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvAFAAAAAA");
    const auto clientData = browserMessageBuilder()->getArrayFromBase64(
        "eyJ0eXBlIjoid2ViYXV0aG4uZ2V0IiwiY2hhbGxlbmdlIjoiOXozNnZUZlFUTDk1TGY3V25aZ3l0ZTdvaEdlRi1YUmlMeGtMLUx1R1Uxem9wUm"
        "1NSVVBMUxWd3pHcHlJbTFmT0JuMVFuUmEwUUgyN0FEQWFKR0h5c1EiLCJvcmlnaW4iOiJodHRwczovL3dlYmF1dGhuLmlvIiwiY3Jvc3NPcmln"
        "aW4iOmZhbHNlfQ");

    const auto signature = browserPasskeys()->buildSignature(authenticatorData, clientData, privateKeyPem);
    QCOMPARE(
        browserMessageBuilder()->getBase64FromArray(signature),
        QString("MOGw6KrerCgPf2mPig7FOTFIUDXYAU1v2uZj89_NgQTg2UddWnAB3JId3pa4zXghj8CkjjadVOI_LvweJGCEpmPQnRby71yFXnja6j"
                "Y3woX2b2klG2fB2alGZHHrVg6yVEmnAii4kYSdmoWxI7SmzLftoZfCJNFPFHujx2Pbr-6dIB02sZhtncetT0cpyWobtj9r7C5dIGfm"
                "J5n-LccP-F9gXGqtbN605VrIkC2WNztjdk3dAt5FGM_dlIwSe-vP1dKfIuNqAEbgr2IVZAUFn_ZfzUo-XbXTysksuz9JZfEopJBiUi"
                "9tjQDNvrYQFqB6wDPqkZAomkbRCohUb3TzCg"));
}

void TestPasskeys::testCreatingAttestationObjectWithEC()
{
    // Predefined values for a desired outcome
    const auto id = QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8");
    const auto predefinedFirst = QString("BuyvNFtikWFtGVkDplAqyjHElahp5fCH5dS4Ms0Ihd0");
    const auto predefinedSecond = QString("4u5_6Q8O6R0Hg0oDCdtCJLEL0yX_GDLhU5m3HUIE54M");

    const auto publicKeyCredentialOptions = browserMessageBuilder()->getJsonObject(PublicKeyCredentialOptions.toUtf8());

    auto rpIdHash = browserMessageBuilder()->getSha256HashAsBase64(QString("webauthn.io"));
    QCOMPARE(rpIdHash, QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvA"));

    TestingVariables testingVariables = {id, predefinedFirst, predefinedSecond};
    auto result = browserPasskeys()->buildAttestationObject(publicKeyCredentialOptions, "", id, testingVariables);
    QCOMPARE(
        QString(result.cborEncoded),
        QString("\xA3"
                "cfmtdnonegattStmt\xA0hauthDataX\xA4t\xA6\xEA\x92\x13\xC9\x9C/t\xB2$\x92\xB3 \xCF@&*\x94\xC1\xA9P\xA0"
                "9\x7F)%\x0B`\x84\x1E\xF0"
                "E\x00\x00\x00\x01\x01\x02\x03\x04\x05\x06\x07\b\x01\x02\x03\x04\x05\x06\x07\b\x00 \x8B\xB0\xCA"
                "6\x17\xD6\xDE\x01\x11|\xEA\x94\r\xA0R\xC0\x80_\xF3r\xFBr\xB5\x02\x03:"
                "\xBAr\x0Fi\x81\xFE\xA5\x01\x02\x03& \x01!X "
                "e\xE2\xF2\x1F:cq\xD3G\xEA\xE0\xF7\x1F\xCF\xFA\\\xABO\xF6\x86\x88\x80\t\xAE\x81\x8BT\xB2\x9B\x15\x85~"
                "\"X \\\x8E\x1E@\xDB\x97T-\xF8\x9B\xB0\xAD"
                "5\xDC\x12^\xC3\x95\x05\xC6\xDF^\x03\xCB\xB4Q\x91\xFF|\xDB\x94\xB7"));

    // Double check that the result can be decoded
    BrowserCbor browserCbor;
    auto attestationJsonObject = browserCbor.getJsonFromCborData(result.cborEncoded);

    // Parse authData
    auto authDataJsonObject = attestationJsonObject["authData"].toString();
    auto authDataArray = browserMessageBuilder()->getArrayFromBase64(authDataJsonObject);
    QVERIFY(authDataArray.size() >= 37);

    auto authData = browserPasskeys()->parseAuthData(authDataArray);
    auto credentialData = authData["credentialData"].toObject();
    auto flags = authData["flags"].toObject();
    auto publicKey = credentialData["publicKey"].toObject();

    // The attestationObject should include the same ID after decoding with the response root
    QCOMPARE(credentialData["credentialId"].toString(), QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8"));
    QCOMPARE(authData["rpIdHash"].toString(), QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvA"));
    QCOMPARE(flags["AT"], true);
    QCOMPARE(flags["UP"], true);
    QCOMPARE(publicKey["1"], WebAuthnCoseKeyType::EC2);
    QCOMPARE(publicKey["3"], WebAuthnAlgorithms::ES256);
    QCOMPARE(publicKey["-1"], 1);
    QCOMPARE(publicKey["-2"], predefinedFirst);
    QCOMPARE(publicKey["-3"], predefinedSecond);
}

void TestPasskeys::testCreatingAttestationObjectWithRSA()
{
    // Predefined values for a desired outcome
    const auto id = QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8");
    const auto predefinedModulus = QString("vUhOZnyn8yn7U-nuHlsXZ6WDWLuYvevWWnwtoHxDEQq27vlp7yAfeVvAPkcvhxRcwoCEUespoa5"
                                           "5IDbkpp2Ypd6b15KbB4C-_4gM4r2FK9gfXghLPAXsMhstYv4keNFb4ghdlY5oUU3JCqUSMyOpmd"
                                           "HeX-RikLL0wgGv_tLT2DaDiWeyQCAtiDblr6COuTAU2kTpLc3Bn35geV9Iqw4iT8DwBQ-f8vjnI"
                                           "EDANXKUiRPojfy1q7WwEl-zMv6Ke2jFHxf68u82BSy3u9DOQaa24FAHoCm8Yd0n5IazMyoxyttl"
                                           "tRt8un8myVOGxcXMiR9_kQb9pu1RRLQMQLd-icE1Qw");
    const auto predefinedExponent = QString("AQAB");

    // Force algorithm to RSA
    QJsonArray pubKeyCredParams;
    pubKeyCredParams.append(QJsonObject({{"type", "public-key"}, {"alg", -257}}));

    auto publicKeyCredentialOptions = browserMessageBuilder()->getJsonObject(PublicKeyCredentialOptions.toUtf8());
    publicKeyCredentialOptions["pubKeyCredParams"] = pubKeyCredParams;

    auto rpIdHash = browserMessageBuilder()->getSha256HashAsBase64(QString("webauthn.io"));
    QCOMPARE(rpIdHash, QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvA"));

    TestingVariables testingVariables = {id, predefinedModulus, predefinedExponent};
    auto result = browserPasskeys()->buildAttestationObject(publicKeyCredentialOptions, "", id, testingVariables);

    // Double check that the result can be decoded
    BrowserCbor browserCbor;
    auto attestationJsonObject = browserCbor.getJsonFromCborData(result.cborEncoded);

    // Parse authData
    auto authDataJsonObject = attestationJsonObject["authData"].toString();
    auto authDataArray = browserMessageBuilder()->getArrayFromBase64(authDataJsonObject);
    QVERIFY(authDataArray.size() >= 37);

    auto authData = browserPasskeys()->parseAuthData(authDataArray);
    auto credentialData = authData["credentialData"].toObject();
    auto flags = authData["flags"].toObject();
    auto publicKey = credentialData["publicKey"].toObject();

    // The attestationObject should include the same ID after decoding with the response root
    QCOMPARE(credentialData["credentialId"].toString(), QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8"));
    QCOMPARE(authData["rpIdHash"].toString(), QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvA"));
    QCOMPARE(flags["AT"], true);
    QCOMPARE(flags["UP"], true);
    QCOMPARE(publicKey["1"], WebAuthnCoseKeyType::RSA);
    QCOMPARE(publicKey["3"], WebAuthnAlgorithms::RS256);
    QCOMPARE(publicKey["-1"], predefinedModulus);
    QCOMPARE(publicKey["-2"], predefinedExponent);
}

void TestPasskeys::testRegister()
{
    // Predefined values for a desired outcome
    const auto predefinedId = QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8");
    const auto predefinedX = QString("BuyvNFtikWFtGVkDplAqyjHElahp5fCH5dS4Ms0Ihd0");
    const auto predefinedY = QString("4u5_6Q8O6R0Hg0oDCdtCJLEL0yX_GDLhU5m3HUIE54M");
    const auto origin = QString("https://webauthn.io");
    const auto testDataPublicKey = browserMessageBuilder()->getJsonObject(PublicKeyCredential.toUtf8());
    const auto testDataResponse = testDataPublicKey["response"];
    const auto publicKeyCredentialOptions = browserMessageBuilder()->getJsonObject(PublicKeyCredentialOptions.toUtf8());

    TestingVariables testingVariables = {predefinedId, predefinedX, predefinedY};
    auto result =
        browserPasskeys()->buildRegisterPublicKeyCredential(publicKeyCredentialOptions, origin, testingVariables);
    auto publicKeyCredential = result.response;
    QCOMPARE(publicKeyCredential["type"], QString("public-key"));
    QCOMPARE(publicKeyCredential["authenticatorAttachment"], QString("platform"));
    QCOMPARE(publicKeyCredential["id"], QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8"));

    auto response = publicKeyCredential["response"].toObject();
    auto attestationObject = response["attestationObject"].toString();
    auto clientDataJson = response["clientDataJSON"].toString();
    QCOMPARE(attestationObject, testDataResponse["attestationObject"].toString());

    // Parse clientDataJSON
    auto clientDataByteArray = browserMessageBuilder()->getArrayFromBase64(clientDataJson);
    auto clientDataJsonObject = browserMessageBuilder()->getJsonObject(clientDataByteArray);
    QCOMPARE(clientDataJsonObject["challenge"],
             QString("lVeHzVxWsr8MQxMkZF0ti6FXhdgMljqKzgA-q_zk2Mnii3eJ47VF97sqUoYktVC85WAZ1uIASm-a_lDFZwsLfw"));
    QCOMPARE(clientDataJsonObject["origin"], origin);
    QCOMPARE(clientDataJsonObject["type"], QString("webauthn.create"));
}

void TestPasskeys::testGet()
{
    const auto privateKeyPem = QString("-----BEGIN PRIVATE KEY-----"
                                       "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg5DX2R6I37nMSZqCp"
                                       "XfHlE3UeitkGGE03FqGsdfxIBoOhRANCAAQG7K80W2KRYW0ZWQOmUCrKMcSVqGnl"
                                       "8Ifl1LgyzQiF3eLuf+kPDukdB4NKAwnbQiSxC9Ml/xgy4VOZtx1CBOeD"
                                       "-----END PRIVATE KEY-----");
    const auto origin = QString("https://webauthn.io");
    const auto id = QString("yrzFJ5lwcpTwYMOdXSmxF5b5cYQlqBMzbbU_d-oFLO8");
    const auto publicKeyCredentialRequestOptions =
        browserMessageBuilder()->getJsonObject(PublicKeyCredentialRequestOptions.toUtf8());

    auto publicKeyCredential = browserPasskeys()->buildGetPublicKeyCredential(
        publicKeyCredentialRequestOptions, origin, id, {}, privateKeyPem);
    QVERIFY(!publicKeyCredential.isEmpty());
    QCOMPARE(publicKeyCredential["id"].toString(), id);

    auto response = publicKeyCredential["response"].toObject();
    QCOMPARE(response["authenticatorData"].toString(), QString("dKbqkhPJnC90siSSsyDPQCYqlMGpUKA5fyklC2CEHvAFAAAAAA"));
    QCOMPARE(response["clientDataJSON"].toString(),
             QString("eyJjaGFsbGVuZ2UiOiI5ejM2dlRmUVRMOTVMZjdXblpneXRlN29oR2VGLVhSaUx4a0wtTHVHVTF6b3BSbU1JVUExTFZ3ekdwe"
                     "UltMWZPQm4xUW5SYTBRSDI3QURBYUpHSHlzUSIsImNyb3NzT3JpZ2luIjpmYWxzZSwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdX"
                     "Robi5pbyIsInR5cGUiOiJ3ZWJhdXRobi5nZXQifQ"));
    QCOMPARE(
        response["signature"].toString(),
        QString("MEUCIHFv0lOOGGloi_XoH5s3QDSs__8yAp9ZTMEjNiacMpOxAiEA04LAfO6TE7j12XNxd3zHQpn4kZN82jQFPntPiPBSD5c"));

    auto clientDataJson = response["clientDataJSON"].toString();
    auto clientDataByteArray = browserMessageBuilder()->getArrayFromBase64(clientDataJson);
    auto clientDataJsonObject = browserMessageBuilder()->getJsonObject(clientDataByteArray);
    QCOMPARE(clientDataJsonObject["challenge"].toString(), publicKeyCredentialRequestOptions["challenge"].toString());
}

void TestPasskeys::testExtensions()
{
    auto extensions = QJsonObject({{"credProps", true}, {"uvm", true}});
    auto result = browserPasskeys()->buildExtensionData(extensions);

    BrowserCbor cbor;
    auto extensionJson = cbor.getJsonFromCborData(result);
    auto uvmArray = extensionJson["uvm"].toArray();
    QCOMPARE(extensionJson["credProps"].toObject()["rk"].toBool(), true);
    QCOMPARE(uvmArray.size(), 1);
    QCOMPARE(uvmArray.first().toArray().size(), 3);

    auto partial = QJsonObject({{"props", true}, {"uvm", true}});
    auto faulty = QJsonObject({{"uvx", true}});
    auto partialData = browserPasskeys()->buildExtensionData(partial);
    auto faultyData = browserPasskeys()->buildExtensionData(faulty);

    auto partialJson = cbor.getJsonFromCborData(partialData);
    QCOMPARE(partialJson["uvm"].toArray().size(), 1);

    auto faultyJson = cbor.getJsonFromCborData(faultyData);
    QCOMPARE(faultyJson.size(), 0);
}

void TestPasskeys::testParseFlags()
{
    auto registerResult = browserPasskeys()->parseFlags("\x45");
    QCOMPARE(registerResult["ED"], false);
    QCOMPARE(registerResult["AT"], true);
    QCOMPARE(registerResult["BS"], false);
    QCOMPARE(registerResult["BE"], false);
    QCOMPARE(registerResult["UV"], true);
    QCOMPARE(registerResult["UP"], true);

    auto getResult = browserPasskeys()->parseFlags("\x05"); // Only UP and UV
    QCOMPARE(getResult["ED"], false);
    QCOMPARE(getResult["AT"], false);
    QCOMPARE(getResult["BS"], false);
    QCOMPARE(getResult["BE"], false);
    QCOMPARE(getResult["UV"], true);
    QCOMPARE(getResult["UP"], true);
}

void TestPasskeys::testSetFlags()
{
    auto registerJson =
        QJsonObject({{"ED", false}, {"AT", true}, {"BS", false}, {"BE", false}, {"UV", true}, {"UP", true}});
    auto registerResult = browserPasskeys()->setFlagsFromJson(registerJson);
    QCOMPARE(registerResult, 0x45);

    auto getJson =
        QJsonObject({{"ED", false}, {"AT", false}, {"BS", false}, {"BE", false}, {"UV", true}, {"UP", true}});
    auto getResult = browserPasskeys()->setFlagsFromJson(getJson);
    QCOMPARE(getResult, 0x05);

    // With "discouraged", so UV is false
    auto discouragedJson =
        QJsonObject({{"ED", false}, {"AT", false}, {"BS", false}, {"BE", false}, {"UV", false}, {"UP", true}});
    auto discouragedResult = browserPasskeys()->setFlagsFromJson(discouragedJson);
    QCOMPARE(discouragedResult, 0x01);
}
