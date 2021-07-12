/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "TestSignature.h"

#include <QTest>

#include "crypto/Crypto.h"
#include "crypto/Random.h"
#include "keeshare/Signature.h"

#include <botan/pem.h>
#include <botan/rsa.h>

QTEST_GUILESS_MAIN(TestSignature)
static const char* rsa_2_private = "-----BEGIN RSA PRIVATE KEY-----\n"
                                   "MIIEowIBAAKCAQEAwGdladnqFfcDy02Gubx4sdBT8NYEg2YKXfcKLSwca5gV4X7I\n"
                                   "z/+QR51LAfPCkj+QjWpj3DD1/6P7s6jOJ4BNd6CSDukv18DOsIsFn2D+zLmVoir2\n"
                                   "lki3sTsmiEz65KvHE8EnQ8IzZCqZDC40tZOcz2bnkZrmcsEibKoxYsmQJk95NwdR\n"
                                   "teFymp1qH3zq85xdNWw2u6c5CKzLgI5BjInttO98iSxL0KuY/JmzMx0gTbRiqc0x\n"
                                   "22EODtdVsBoNL/pt8v6Q+WLpRg4/Yq7YurAngxk4h38NWvufj2vJvbcRqX4cupcu\n"
                                   "92T9SWwSwZmd4Xy3bt+AUlq4XRMa1MlKfPvXmwIDAQABAoIBAGbWnRD/xaup1OBU\n"
                                   "dr9N6qD3/fXLHqxw3PeudEUCv8oOhxt43bK3IZH1k8LeXFA5I3VCuU9W6BWUu5Mi\n"
                                   "ldXtMPrQ22CW6NiEGLWqCP5QJMCeLUl5d0WKZoyXVhgiNTQGUKjRY8BGy5stXZJy\n"
                                   "HAA1fuooUXu09Jm/ezvjl/P6Uk722nZns4g6cc8aUSQDSVoeuCvwDaix5o4Z4RGY\n"
                                   "4biIKGj5qYxoe+rbgYH/2zlEcAiSJIuqjYY+Xk4IdB89DYZBYnO/xPkRaDeiY2xl\n"
                                   "QM7Inr7PQC8PWJc9zYYvlGnnmIRCkO15mWau70N1Y1rUAsyW61g2GyFhdsIIODH/\n"
                                   "878Kc9ECgYEA+2JaUqRWr6dqE+uVPpGkbGiAaRQ79olTcRmxXCnM+Y3c88z9G7kC\n"
                                   "2S5UKPRDl7EzwmMJqqb8BZbdSWoAxO4++F6ylSz7TqowPw+13Wxwm3wApvr2Q1Mo\n"
                                   "rkq4ltgyHMR+iXvKqOYa2GqZNmRwh7JGLIJ7Y0Z77nwBkkgDc/3ey8MCgYEAw+/N\n"
                                   "fxv2t+r6VKxEtjdy3sfn8LLjiWqghPngJzcYH9NdB8fmMN1WHqX075hbKjI9TyJw\n"
                                   "77p8onjZI0opLexHHUmepEa6Ijo1zynJJ7XPXnyruiTXXqz49io6lFOLcXi/i+DZ\n"
                                   "B2vQcMGWA4qwJxz7KA6EZ/HimjuysV1guvlKf0kCgYA6+JGTvXWQc0eRMLysFuJp\n"
                                   "hAJLpDGE3iYy7AINSskI6dyhXL8rl7UxWYroqJSKq0knGrCT1eRdM0zqAfH4QKOJ\n"
                                   "BD4EfK7ff1EeGgNh1CR+dRJ6GXlXxdRPPrwattDaqsW8Xsvl30UA69DRT7KOQqXv\n"
                                   "nxRu74P3KCP+OuKEfVOcnQKBgQC+/2r1Zj/5huBhW9BbQ/ABBSOuueMeGEfDeIUu\n"
                                   "FQG6PGKqbA2TQp9pnuMGECGGH5UuJ+epeMN36Y/ZW7iKoJGuFg7EGoHlTZMYj6Yb\n"
                                   "xJoRhDwuZy1eiATkicOyxUHf6hHme9dz6YA1+i+O4knWxuR5ZrVhUiRPrrQBO4JI\n"
                                   "oSwiqQKBgHblgOVfOJrG3HDg6bo+qmxQsGFRCD0mehsg9YpokuZVX0UJtGx/RJHU\n"
                                   "vIBL00An6YcNfPTSlNJeG83APtk/tdgsXvQd3pmIkeY78x6xWKSieZXv4lyDv7lX\n"
                                   "r28lCTj2Ez2gEzEohZgf4V1uzBvTdJefarpQ00ep34UZ9FsNfUwD\n"
                                   "-----END RSA PRIVATE KEY-----\n";
static const char* rsa_2_public = "-----BEGIN RSA PUBLIC KEY-----\n"
                                  "MIIBCgKCAQEAwGdladnqFfcDy02Gubx4sdBT8NYEg2YKXfcKLSwca5gV4X7Iz/+Q\n"
                                  "R51LAfPCkj+QjWpj3DD1/6P7s6jOJ4BNd6CSDukv18DOsIsFn2D+zLmVoir2lki3\n"
                                  "sTsmiEz65KvHE8EnQ8IzZCqZDC40tZOcz2bnkZrmcsEibKoxYsmQJk95NwdRteFy\n"
                                  "mp1qH3zq85xdNWw2u6c5CKzLgI5BjInttO98iSxL0KuY/JmzMx0gTbRiqc0x22EO\n"
                                  "DtdVsBoNL/pt8v6Q+WLpRg4/Yq7YurAngxk4h38NWvufj2vJvbcRqX4cupcu92T9\n"
                                  "SWwSwZmd4Xy3bt+AUlq4XRMa1MlKfPvXmwIDAQAB\n"
                                  "-----END RSA PUBLIC KEY-----\n";
static const char* rsa_2_sign = "4fda1b39f93f174cdc79ac2bd6118155359830c90e2c39b60a1a548852f2c87a"
                                "cd61b2a378259a38befad35dbf208a2c1332ab74faf2cee2ff2e8be49c4c5f41"
                                "dc10e5a5fafb53d3c54e2b7640d7bfee6bb0f24c5a1fb934150a144c2b465fe4"
                                "8a1579e666a097fb1609ae9abc5760f6e6d6e73acb610fb11dd1c409ca284a72"
                                "0be64dd56a28ab257e8721f5bade58816382581ac08d932098dd200d836fe897"
                                "f78a5f02095ac3b21cca2a47b2afd282ce075c6450cba8c85b08b58c5bacb75d"
                                "e1a73bdec4321193d4a3ce653d8e3aa8a4f2beac6a44497328f8855f7e28e15d"
                                "f63b21f8bc7204bf6e202c9cb08be050379be5ad88d8e695a38440a50e75dfdf";
static const char* rsa_1_private = "-----BEGIN RSA PRIVATE KEY-----\n"
                                   "MIIEpAIBAAKCAQEAsCHtJicDPWnvHSIKbnTZaJkIB9vgE0pmLdK580JUqBuonVbB\n"
                                   "y1QTy0ZQ7/TtqvLPgwPK88TR46OLO/QGCzo2+XxgJ85uy0xfuyUYRmSuw0drsErN\n"
                                   "mH8vU91lSBxsGDp9LtBbgHKoR23vMWZ34IxFRc55XphrIH48ijsMaL6bXBwF/3tD\n"
                                   "9T3lm2MpP1huyVNnIY9+GRRWCy4f9LMj/UGu/n4RtwwfpOZBBRwYkq5QkzA9lPm/\n"
                                   "VzF3MP1rKTMkvAw+Nfb383mkmc6MRnsa6uh6iDa9aVB7naegM13UJQX/PY1Ks6pO\n"
                                   "XDpy/MQ7iCh+HmYNq5dRmARyaNl9xIXJNhz1cQIDAQABAoIBAQCnEUc1LUQxeM5K\n"
                                   "wANNCqE+SgoIClPdeHC7fmrLh1ttqe6ib6ybBUFRS31yXs0hnfefunVEDKlaV8K2\n"
                                   "N52UAMAsngFHQNRvGh6kEWeZPd9Xc+N98TZbNCjcT+DGKc+Om8wqH5DrodZlCq4c\n"
                                   "GaoT4HnE4TjWtZTH2XXrWF9I66PKFWf070R44nvyVcvaZi4pC2YmURRPuGF6K1iK\n"
                                   "dH8zM6HHG1UGu2W6hLNn+K01IulG0Lb8eWNaNYMmtQWaxyp7I2IWkkecUs3nCuiR\n"
                                   "byFOoomCjdh8r9yZFvwxjGUhgtkALN9GCU0Mwve+s11IB2gevruN+q9/Qejbyfdm\n"
                                   "IlgLAeTRAoGBANRcVzW9CYeobCf+U9hKJFEOur8XO+J2mTMaELA0EjWpTJFAeIT7\n"
                                   "KeRpCRG4/vOSklxxRF6vP1EACA4Z+5BlN+FTipHHs+bSEgqkPZiiANDH7Zot5Iqv\n"
                                   "1q0fRyldNRZNZK7DWp08BPNVWGA/EnEuKJiURxnxBaxNXbUyMCdjxvMvAoGBANRT\n"
                                   "utbrqS/bAa/DcHKn3V6DRqBl3TDOfvCNjiKC84a67F2uXgzLIdMktr4d1NyCZVJd\n"
                                   "7/zVgWORLIdg1eAi6rYGoOvNV39wwga7CF+m9sBY0wAaKYCELe6L26r4aQHVCX6n\n"
                                   "rnIgUv+4o4itmU2iP0r3wlmDC9pDRQP82vfvQPlfAoGASwhleANW/quvq2HdViq8\n"
                                   "Mje2HBalfhrRfpDTHK8JUBSFjTzuWG42GxJRtgVbb8x2ElujAKGDCaetMO5VSGu7\n"
                                   "Fs5hw6iAFCpdXY0yhl+XUi2R8kwM2EPQ4lKO3jqkq0ClNmqn9a5jQWcCVt9yMLNS\n"
                                   "fLbHeI8EpiCf34ngIcrLXNkCgYEAzlcEZuKkC46xB+dNew8pMTUwSKZVm53BfPKD\n"
                                   "44QRN6imFbBjU9mAaJnwQbfp6dWKs834cGPolyM4++MeVfB42iZ88ksesgmZdUMD\n"
                                   "szkl6O0pOJs0I+HQZVdjRbadDZvD22MHQ3+oST1dJ3FVXz3Cdo9qPuT8esMO6f4r\n"
                                   "qfDH2s8CgYAXC/lWWHQ//PGP0pH4oiEXisx1K0X1u0xMGgrChxBRGRiKZUwNMIvJ\n"
                                   "TqUu7IKizK19cLHF/NBvxHYHFw+m7puNjn6T1RtRCUjRZT7Dx1VHfVosL9ih5DA8\n"
                                   "tpbZA5KGKcvHtB5DDgT0MHwzBZnb4Q//Rhovzn+HXZPsJTTgHHy3NQ==\n"
                                   "-----END RSA PRIVATE KEY-----\n";
static const char* rsa_1_public = "-----BEGIN RSA PUBLIC KEY-----\n"
                                  "MIIBCgKCAQEAsCHtJicDPWnvHSIKbnTZaJkIB9vgE0pmLdK580JUqBuonVbBy1QT\n"
                                  "y0ZQ7/TtqvLPgwPK88TR46OLO/QGCzo2+XxgJ85uy0xfuyUYRmSuw0drsErNmH8v\n"
                                  "U91lSBxsGDp9LtBbgHKoR23vMWZ34IxFRc55XphrIH48ijsMaL6bXBwF/3tD9T3l\n"
                                  "m2MpP1huyVNnIY9+GRRWCy4f9LMj/UGu/n4RtwwfpOZBBRwYkq5QkzA9lPm/VzF3\n"
                                  "MP1rKTMkvAw+Nfb383mkmc6MRnsa6uh6iDa9aVB7naegM13UJQX/PY1Ks6pOXDpy\n"
                                  "/MQ7iCh+HmYNq5dRmARyaNl9xIXJNhz1cQIDAQAB\n"
                                  "-----END RSA PUBLIC KEY-----\n";

static QByteArray data("Some trivial test with a longer .... ................................. longer text");

QSharedPointer<Botan::Private_Key> loadPrivateKey(const QString& pem)
{
    try {
        std::string label;
        auto der = Botan::PEM_Code::decode(pem.toStdString(), label);
        auto key = new Botan::RSA_PrivateKey(
            Botan::AlgorithmIdentifier("RSA", Botan::AlgorithmIdentifier::USE_NULL_PARAM), der);
        return QSharedPointer<Botan::Private_Key>(key);
    } catch (std::exception& e) {
        qWarning("Failed to load key: %s", e.what());
        return {};
    }
}

QSharedPointer<Botan::Public_Key> loadPublicKey(const QString& pem)
{
    try {
        std::string label;
        auto der = Botan::PEM_Code::decode(pem.toStdString(), label);
        auto key =
            new Botan::RSA_PublicKey(Botan::AlgorithmIdentifier("RSA", Botan::AlgorithmIdentifier::USE_NULL_PARAM),
                                     std::vector<uint8_t>(der.begin(), der.end()));
        return QSharedPointer<Botan::Public_Key>(key);
    } catch (std::exception& e) {
        qWarning("Failed to load key: %s", e.what());
        return {};
    }
}

void TestSignature::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestSignature::testSigningOpenSSH_RSA_PrivateOnly()
{
    auto rsaKey = loadPrivateKey(rsa_2_private);
    QVERIFY(rsaKey);

    QString sign;
    Signature::create(data, rsaKey, sign);
    QCOMPARE(sign, QString("rsa|%1").arg(QString::fromLatin1(rsa_2_sign)));

    const bool verified = Signature::verify(data, rsaKey, sign);
    QCOMPARE(verified, true);
}

void TestSignature::testSigningOpenSSH_RSA()
{
    auto privateKey = loadPrivateKey(rsa_2_private);
    QVERIFY(privateKey);

    QString sign;
    Signature::create(data, privateKey, sign);
    QVERIFY(!sign.isEmpty());

    auto publicKey = loadPublicKey(rsa_2_public);
    QVERIFY(publicKey);

    const bool verified = Signature::verify(data, publicKey, sign);
    QCOMPARE(verified, true);
}

void TestSignature::testSigningGenerated_RSA_PrivateOnly()
{
    QSharedPointer<Botan::Private_Key> key(new Botan::RSA_PrivateKey(*randomGen()->getRng(), 2048));

    QString sign;
    Signature::create(data, key, sign);
    QVERIFY(!sign.isEmpty());

    const bool verified = Signature::verify(data, key, sign);
    QCOMPARE(verified, true);
}

void TestSignature::testSigningTest_RSA_PrivateOnly()
{
    auto rsaKey = loadPrivateKey(rsa_2_private);
    QVERIFY(rsaKey);

    QString sign;
    Signature::create(data, rsaKey, sign);
    QVERIFY(!sign.isEmpty());

    const bool verified = Signature::verify(data, rsaKey, sign);
    QCOMPARE(verified, true);
}

void TestSignature::testSigningTest_RSA()
{
    auto privateKey = loadPrivateKey(rsa_1_private);
    QVERIFY(privateKey);

    QString sign;
    Signature::create(data, privateKey, sign);
    QVERIFY(!sign.isEmpty());

    auto publicKey = loadPublicKey(rsa_1_public);
    QVERIFY(publicKey);

    const bool verified = Signature::verify(data, publicKey, sign);
    QCOMPARE(verified, true);
}
