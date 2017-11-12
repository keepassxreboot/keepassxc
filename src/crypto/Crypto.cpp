/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Crypto.h"

#include <QMutex>

#include <gcrypt.h>

#include "config-keepassx.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"

bool Crypto::m_initalized(false);
QString Crypto::m_errorStr;
QString Crypto::m_backendVersion;

Crypto::Crypto()
{
}

bool Crypto::init()
{
    if (m_initalized) {
        qWarning("Crypto::init: already initalized");
        return true;
    }

    m_backendVersion = QString::fromLocal8Bit(gcry_check_version(0));
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    if (!checkAlgorithms()) {
        return false;
    }

    // has to be set before testing Crypto classes
    m_initalized = true;

    if (!selfTest()) {
        m_initalized = false;
        return false;
    }

    return true;
}

bool Crypto::initalized()
{
    return m_initalized;
}

QString Crypto::errorString()
{
    return m_errorStr;
}

QString Crypto::backendVersion()
{
    return QString("libgcrypt ").append(m_backendVersion);
}

bool Crypto::backendSelfTest()
{
    return (gcry_control(GCRYCTL_SELFTEST) == 0);
}

bool Crypto::checkAlgorithms()
{
    if (gcry_cipher_algo_info(GCRY_CIPHER_AES256, GCRYCTL_TEST_ALGO, nullptr, nullptr) != 0) {
        m_errorStr = "GCRY_CIPHER_AES256 not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }
    if (gcry_cipher_algo_info(GCRY_CIPHER_TWOFISH, GCRYCTL_TEST_ALGO, nullptr, nullptr) != 0) {
        m_errorStr = "GCRY_CIPHER_TWOFISH not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }
    if (gcry_cipher_algo_info(GCRY_CIPHER_SALSA20, GCRYCTL_TEST_ALGO, nullptr, nullptr) != 0) {
        m_errorStr = "GCRY_CIPHER_SALSA20 not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }
    if (gcry_cipher_algo_info(GCRY_CIPHER_CHACHA20, GCRYCTL_TEST_ALGO, nullptr, nullptr) != 0) {
        m_errorStr = "GCRY_CIPHER_CHACHA20 not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }
    if (gcry_md_test_algo(GCRY_MD_SHA256) != 0) {
        m_errorStr = "GCRY_MD_SHA256 not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }
    if (gcry_md_test_algo(GCRY_MD_SHA512) != 0) {
        m_errorStr = "GCRY_MD_SHA512 not found.";
        qWarning("Crypto::checkAlgorithms: %s", qPrintable(m_errorStr));
        return false;
    }

    return true;
}

bool Crypto::selfTest()
{
    return testSha256() && testSha512() && testAes256Cbc() && testAes256Ecb() && testTwofish() && testSalsa20() && testChaCha20();
}

void Crypto::raiseError(const QString& str)
{
    m_errorStr = str;
    qWarning("Crypto::selfTest: %s", qPrintable(m_errorStr));
}

bool Crypto::testSha256()
{
    QByteArray sha256Test = CryptoHash::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                                             CryptoHash::Sha256);

    if (sha256Test != QByteArray::fromHex("248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1")) {
        raiseError("SHA-256 mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testSha512()
{
    QByteArray sha512Test = CryptoHash::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                                             CryptoHash::Sha512);

    if (sha512Test != QByteArray::fromHex("204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445")) {
        raiseError("SHA-512 mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testAes256Cbc()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    plainText.append(QByteArray::fromHex("ae2d8a571e03ac9c9eb76fac45af8e51"));
    QByteArray cipherText = QByteArray::fromHex("f58c4c04d6e5f1ba779eabfb5f7bfbd6");
    cipherText.append(QByteArray::fromHex("9cfc4e967edb808d679f777bc6702c7d"));
    bool ok;

    SymmetricCipher aes256Encrypt(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt);
    if (!aes256Encrypt.init(key, iv)) {
        raiseError(aes256Encrypt.errorString());
        return false;
    }
    QByteArray encryptedText = aes256Encrypt.process(plainText, &ok);
    if (!ok) {
        raiseError(aes256Encrypt.errorString());
        return false;
    }
    if (encryptedText != cipherText) {
        raiseError("AES-256 CBC encryption mismatch.");
        return false;
    }

    SymmetricCipher aes256Decrypt(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt);
    if (!aes256Decrypt.init(key, iv)) {
        raiseError(aes256Decrypt.errorString());
        return false;
    }
    QByteArray decryptedText = aes256Decrypt.process(cipherText, &ok);
    if (!ok) {
        raiseError(aes256Decrypt.errorString());
        return false;
    }
    if (decryptedText != plainText) {
        raiseError("AES-256 CBC decryption mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testAes256Ecb()
{
    QByteArray key = QByteArray::fromHex("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F");
    QByteArray iv = QByteArray::fromHex("00000000000000000000000000000000");
    QByteArray plainText = QByteArray::fromHex("00112233445566778899AABBCCDDEEFF");
    plainText.append(QByteArray::fromHex("00112233445566778899AABBCCDDEEFF"));
    QByteArray cipherText = QByteArray::fromHex("8EA2B7CA516745BFEAFC49904B496089");
    cipherText.append(QByteArray::fromHex("8EA2B7CA516745BFEAFC49904B496089"));
    bool ok;

    SymmetricCipher aes256Encrypt(SymmetricCipher::Aes256, SymmetricCipher::Ecb, SymmetricCipher::Encrypt);
    if (!aes256Encrypt.init(key, iv)) {
        raiseError(aes256Encrypt.errorString());
        return false;
    }
    QByteArray encryptedText = aes256Encrypt.process(plainText, &ok);
    if (!ok) {
        raiseError(aes256Encrypt.errorString());
        return false;
    }
    if (encryptedText != cipherText) {
        raiseError("AES-256 ECB encryption mismatch.");
        return false;
    }

    SymmetricCipher aes256Decrypt(SymmetricCipher::Aes256, SymmetricCipher::Ecb, SymmetricCipher::Decrypt);
    if (!aes256Decrypt.init(key, iv)) {
        raiseError(aes256Decrypt.errorString());
        return false;
    }
    QByteArray decryptedText = aes256Decrypt.process(cipherText, &ok);
    if (!ok) {
        raiseError(aes256Decrypt.errorString());
        return false;
    }
    if (decryptedText != plainText) {
        raiseError("AES-256 ECB decryption mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testTwofish()
{
    QByteArray key = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    QByteArray iv = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
    QByteArray plainText = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    plainText.append(QByteArray::fromHex("ae2d8a571e03ac9c9eb76fac45af8e51"));
    QByteArray cipherText = QByteArray::fromHex("e0227c3cc80f3cb1b2ed847cc6f57d3c");
    cipherText.append(QByteArray::fromHex("657b1e7960b30fb7c8d62e72ae37c3a0"));
    bool ok;

    SymmetricCipher twofishEncrypt(SymmetricCipher::Twofish, SymmetricCipher::Cbc, SymmetricCipher::Encrypt);
    if (!twofishEncrypt.init(key, iv)) {
        raiseError(twofishEncrypt.errorString());
        return false;
    }
    QByteArray encryptedText = twofishEncrypt.process(plainText, &ok);
    if (!ok) {
        raiseError(twofishEncrypt.errorString());
        return false;
    }
    if (encryptedText != cipherText) {
        raiseError("Twofish encryption mismatch.");
        return false;
    }


    SymmetricCipher twofishDecrypt(SymmetricCipher::Twofish, SymmetricCipher::Cbc, SymmetricCipher::Decrypt);
    if (!twofishDecrypt.init(key, iv)) {
        raiseError(twofishEncrypt.errorString());
        return false;
    }
    QByteArray decryptedText = twofishDecrypt.process(cipherText, &ok);
    if (!ok) {
        raiseError(twofishDecrypt.errorString());
        return false;
    }
    if (decryptedText != plainText) {
        raiseError("Twofish encryption mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testSalsa20()
{
    QByteArray salsa20Key = QByteArray::fromHex("F3F4F5F6F7F8F9FAFBFCFDFEFF000102030405060708090A0B0C0D0E0F101112");
    QByteArray salsa20iv = QByteArray::fromHex("0000000000000000");
    QByteArray salsa20Plain = QByteArray::fromHex("00000000000000000000000000000000");
    QByteArray salsa20Cipher = QByteArray::fromHex("B4C0AFA503BE7FC29A62058166D56F8F");
    bool ok;

    SymmetricCipher salsa20Stream(SymmetricCipher::Salsa20, SymmetricCipher::Stream,
                                  SymmetricCipher::Encrypt);
    if (!salsa20Stream.init(salsa20Key, salsa20iv)) {
        raiseError(salsa20Stream.errorString());
        return false;
    }

    QByteArray salsaProcessed = salsa20Stream.process(salsa20Plain, &ok);
    if (!ok) {
        raiseError(salsa20Stream.errorString());
        return false;
    }
    if (salsaProcessed != salsa20Cipher) {
        raiseError("Salsa20 stream cipher mismatch.");
        return false;
    }

    return true;
}

bool Crypto::testChaCha20() {
    QByteArray chacha20Key = QByteArray::fromHex("0000000000000000000000000000000000000000000000000000000000000000");
    QByteArray chacha20iv = QByteArray::fromHex("0000000000000000");
    QByteArray chacha20Plain = QByteArray::fromHex("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    QByteArray chacha20Cipher = QByteArray::fromHex("76b8e0ada0f13d90405d6ae55386bd28bdd219b8a08ded1aa836efcc8b770dc7da41597c5157488d7724e03fb8d84a376a43b8f41518a11cc387b669b2ee6586");
    bool ok;

    SymmetricCipher chacha20Stream(SymmetricCipher::ChaCha20, SymmetricCipher::Stream,
                                  SymmetricCipher::Encrypt);
    if (!chacha20Stream.init(chacha20Key, chacha20iv)) {
        raiseError(chacha20Stream.errorString());
        return false;
    }

    QByteArray chacha20Processed = chacha20Stream.process(chacha20Plain, &ok);
    if (!ok) {
        raiseError(chacha20Stream.errorString());
        return false;
    }
    if (chacha20Processed != chacha20Cipher) {
        raiseError("ChaCha20 stream cipher mismatch.");
        return false;
    }

    return true;
}