/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "OpVaultReader.h"
#include "OpData01.h"

#include "core/Group.h"
#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "keys/PasswordKey.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <gcrypt.h>

OpVaultReader::OpVaultReader(QObject* parent)
    : QObject(parent)
    , m_error(false)
{
}

OpVaultReader::~OpVaultReader()
{
}

Database* OpVaultReader::readDatabase(QDir& opdataDir, const QString& password)
{
    if (!opdataDir.exists()) {
        m_error = true;
        m_errorStr = tr("Directory .opvault must exist");
        return nullptr;
    }
    if (!opdataDir.isReadable()) {
        m_error = true;
        m_errorStr = tr("Directory .opvault must be readable");
        return nullptr;
    }

    // https://support.1password.com/opvault-design/#directory-layout
    QDir defaultDir = QDir(opdataDir);
    if (!defaultDir.cd("default")) {
        m_error = true;
        m_errorStr = tr("Directory .opvault/default must exist");
        return nullptr;
    }
    if (!defaultDir.isReadable()) {
        m_error = true;
        m_errorStr = tr("Directory .opvault/default must be readable");
        return nullptr;
    }

    auto vaultName = opdataDir.dirName();

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(password));

    QScopedPointer<Database> db(new Database());
    db->setKdf(KeePass2::uuidToKdf(KeePass2::KDF_ARGON2D));
    db->setCipher(KeePass2::CIPHER_AES256);
    db->setKey(key, true, false);
    db->metadata()->setName(vaultName);

    auto rootGroup = db->rootGroup();
    rootGroup->setTimeInfo({});
    rootGroup->setUpdateTimeinfo(false);
    rootGroup->setName(vaultName.remove(".opvault"));
    rootGroup->setUuid(QUuid::createUuid());

    populateCategoryGroups(rootGroup);

    QFile profileJsFile(defaultDir.absoluteFilePath("profile.js"));
    QJsonObject profileJson = readAndAssertJsonFile(profileJsFile, "var profile=", ";");
    if (profileJson.isEmpty()) {
        return nullptr;
    }
    if (!processProfileJson(profileJson, password, rootGroup)) {
        zeroKeys();
        return nullptr;
    }
    if (profileJson.contains("uuid") and profileJson["uuid"].isString()) {
        rootGroup->setUuid(Tools::hexToUuid(profileJson["uuid"].toString()));
    }

    QFile foldersJsFile(defaultDir.filePath("folders.js"));
    if (foldersJsFile.exists()) {
        QJsonObject foldersJs = readAndAssertJsonFile(foldersJsFile, "loadFolders(", ");");
        if (!processFolderJson(foldersJs, rootGroup)) {
            zeroKeys();
            return nullptr;
        }
    }

    const QString bandChars("0123456789ABCDEF");
    QString bandPattern("band_%1.js");
    for (QChar ch : bandChars) {
        QFile bandFile(defaultDir.filePath(bandPattern.arg(ch)));
        if (!bandFile.exists()) {
            continue;
        }
        // https://support.1password.com/opvault-design/#band-files
        QJsonObject bandJs = readAndAssertJsonFile(bandFile, "ld(", ");");
        const QStringList keys = bandJs.keys();
        for (const QString& entryKey : keys) {
            const QJsonObject bandEnt = bandJs[entryKey].toObject();
            const QString uuid = bandEnt["uuid"].toString();
            if (entryKey != uuid) {
                qWarning() << QString("Mismatched Entry UUID, its JSON key <<%1>> and its UUID <<%2>>")
                                  .arg(entryKey)
                                  .arg(uuid);
            }
            QStringList requiredKeys({"d", "k", "hmac"});
            bool ok = true;
            for (const QString& requiredKey : asConst(requiredKeys)) {
                if (!bandEnt.contains(requiredKey)) {
                    qCritical() << "Skipping malformed Entry UUID " << uuid << " without key " << requiredKey;
                    ok = false;
                    break;
                }
            }
            if (!ok) {
                continue;
            }
            // https://support.1password.com/opvault-design/#items
            auto entry = processBandEntry(bandEnt, defaultDir, rootGroup);
            if (!entry) {
                qWarning() << "Unable to process Band Entry " << uuid;
            }
        }
    }

    // Remove empty categories (groups)
    for (auto group : rootGroup->children()) {
        if (group->isEmpty()) {
            delete group;
        }
    }

    zeroKeys();
    return db.take();
}

bool OpVaultReader::hasError()
{
    return m_error;
}

QString OpVaultReader::errorString()
{
    return m_errorStr;
}

bool OpVaultReader::processProfileJson(QJsonObject& profileJson, const QString& password, Group* rootGroup)
{
    unsigned long iterations = profileJson["iterations"].toInt();
    // QString lastUpdatedBy = profileJson["lastUpdatedBy"].toString();
    QString masterKeyB64 = profileJson["masterKey"].toString();
    QString overviewKeyB64 = profileJson["overviewKey"].toString();
    // QString profileName = profileJs["profileName"].toString();
    QByteArray salt;
    {
        QString saltB64 = profileJson["salt"].toString();
        salt = QByteArray::fromBase64(saltB64.toUtf8());
    }
    auto rootGroupTime = rootGroup->timeInfo();
    auto createdAt = static_cast<uint>(profileJson["createdAt"].toInt());
    rootGroupTime.setCreationTime(QDateTime::fromTime_t(createdAt, Qt::UTC));
    auto updatedAt = static_cast<uint>(profileJson["updatedAt"].toInt());
    rootGroupTime.setLastModificationTime(QDateTime::fromTime_t(updatedAt, Qt::UTC));
    rootGroup->setUuid(Tools::hexToUuid(profileJson["uuid"].toString()));

    const auto derivedKeys = deriveKeysFromPassPhrase(salt, password, iterations);
    if (derivedKeys->error) {
        m_error = true;
        m_errorStr = derivedKeys->errorStr;
        delete derivedKeys;
        return false;
    }

    QByteArray encKey = derivedKeys->encrypt;
    QByteArray hmacKey = derivedKeys->hmac;
    delete derivedKeys;

    auto masterKeys = decodeB64CompositeKeys(masterKeyB64, encKey, hmacKey);
    if (masterKeys->error) {
        m_error = true;
        m_errorStr = masterKeys->errorStr;
        delete masterKeys;
        return false;
    }
    m_masterKey = masterKeys->encrypt;
    m_masterHmacKey = masterKeys->hmac;
    delete masterKeys;
    auto overviewKeys = decodeB64CompositeKeys(overviewKeyB64, encKey, hmacKey);
    if (overviewKeys->error) {
        m_error = true;
        m_errorStr = overviewKeys->errorStr;
        delete overviewKeys;
        return false;
    }
    m_overviewKey = overviewKeys->encrypt;
    m_overviewHmacKey = overviewKeys->hmac;
    delete overviewKeys;

    return true;
}

bool OpVaultReader::processFolderJson(QJsonObject& foldersJson, Group* rootGroup)
{
    const QStringList keys = foldersJson.keys();

    bool result = true;
    for (const QString& key : keys) {
        const QJsonValueRef& folderValue = foldersJson[key];
        if (!folderValue.isObject()) {
            qWarning() << "Found non-Object folder with key \"" << key << "\"";
            continue;
        }
        const QJsonObject folder = folderValue.toObject();
        QJsonObject overviewJs;
        const QString overviewStr = folder.value("overview").toString();
        OpData01 foldOverview01;
        if (!foldOverview01.decodeBase64(overviewStr, m_overviewKey, m_overviewHmacKey)) {
            qCritical() << "Unable to decipher folder UUID \"" << key << "\": " << foldOverview01.errorString();
            result = false;
            continue;
        }
        auto foldOverview = foldOverview01.getClearText();
        QJsonDocument fOverJSON = QJsonDocument::fromJson(foldOverview);
        overviewJs = fOverJSON.object();

        const QString& folderTitle = overviewJs["title"].toString();
        auto myGroup = new Group();
        myGroup->setParent(rootGroup);
        myGroup->setName(folderTitle);
        if (folder.contains("uuid")) {
            myGroup->setUuid(Tools::hexToUuid(folder["uuid"].toString()));
        }

        if (overviewJs.contains("smart") && overviewJs["smart"].toBool()) {
            if (!overviewJs.contains("predicate_b64")) {
                const QString& errMsg =
                    QString(R"(Expected a predicate in smart folder[uuid="%1"; title="%2"]))").arg(key, folderTitle);
                qWarning() << errMsg;
                myGroup->setNotes(errMsg);
            } else {
                QByteArray pB64 = QByteArray::fromBase64(overviewJs["predicate_b64"].toString().toUtf8());
                myGroup->setNotes(pB64.toHex());
            }
        }

        TimeInfo ti;
        bool timeInfoOk = false;
        if (folder.contains("created")) {
            auto createdTime = static_cast<uint>(folder["created"].toInt());
            ti.setCreationTime(QDateTime::fromTime_t(createdTime, Qt::UTC));
            timeInfoOk = true;
        }
        if (folder.contains("updated")) {
            auto updateTime = static_cast<uint>(folder["updated"].toInt());
            ti.setLastModificationTime(QDateTime::fromTime_t(updateTime, Qt::UTC));
            timeInfoOk = true;
        }
        // "tx" is modified by sync, not by user; maybe a custom attribute?
        if (timeInfoOk) {
            myGroup->setTimeInfo(ti);
        }
    }
    return result;
}

/*
 * Asserts that the given file is an existing file, able to be read, contains JSON, and that
 * the payload is a JSON object. Currently it just returns an empty QJsonObject as a means of
 * indicating the error, although it will qCritical() if unable to actually open the file for reading.
 *
 * @param file the path containing the JSON file
 * @param stripLeading any leading characters that might be present in file which should be removed
 * @param stripTrailing the trailing characters that might be present in file which should be removed
 * @return
 */
QJsonObject OpVaultReader::readAndAssertJsonFile(QFile& file, const QString& stripLeading, const QString& stripTrailing)
{
    QByteArray filePayload;
    const QFileInfo& fileInfo = QFileInfo(file);
    auto absFilePath = fileInfo.absoluteFilePath();
    if (!fileInfo.exists()) {
        qCritical() << QString("File \"%1\" must exist").arg(absFilePath);
        return QJsonObject();
    }
    if (!fileInfo.isReadable()) {
        qCritical() << QString("File \"%1\" must be readable").arg(absFilePath);
        return QJsonObject();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << QString("Unable to open \"%1\" readonly+text").arg(absFilePath);
    }
    filePayload = file.readAll();
    file.close();
    if (!stripLeading.isEmpty()) {
        QByteArray prefix = stripLeading.toUtf8();
        if (filePayload.startsWith(prefix)) {
            filePayload = filePayload.remove(0, prefix.size());
        }
    }
    if (!stripTrailing.isEmpty()) {
        QByteArray suffix = stripTrailing.toUtf8();
        if (filePayload.endsWith(suffix)) {
            const int delBytes = suffix.size();
            filePayload = filePayload.remove(filePayload.length() - delBytes, delBytes);
        }
    }

    QJsonParseError* error = Q_NULLPTR;
    QJsonDocument jDoc = QJsonDocument::fromJson(filePayload, error);
    if (!jDoc.isObject()) {
        qCritical() << "Expected " << filePayload << "to be a JSON Object";
        return QJsonObject();
    }
    return jDoc.object();
}

/* Convenience method for calling decodeCompositeKeys when you have a base64 encrypted composite key. */
OpVaultReader::DerivedKeyHMAC*
OpVaultReader::decodeB64CompositeKeys(const QString& b64, const QByteArray& encKey, const QByteArray& hmacKey)
{
    auto result = new DerivedKeyHMAC();

    OpData01 keyKey01;
    if (!keyKey01.decodeBase64(b64, encKey, hmacKey)) {
        result->error = true;
        result->errorStr = tr("Unable to decode masterKey: %1").arg(keyKey01.errorString());
        return result;
    }
    delete result;

    const QByteArray keyKey = keyKey01.getClearText();

    return decodeCompositeKeys(keyKey);
}

/*
 * Given a string of bytes, decompose it into its constituent parts, an encryption key and a HMAC key.
 * The plaintext of the masterKey is 256 bytes of data selected randomly when the keychain was first created.
 *
 * The 256 byte (2048 bit) plaintext content of the masterKey is then hashed with SHA-512.
 * The first 32 bytes (256-bits) of the resulting hash are the master encryption key,
 * and the second 32 bytes are the master hmac key.
 */
OpVaultReader::DerivedKeyHMAC* OpVaultReader::decodeCompositeKeys(const QByteArray& keyKey)
{
    const int encKeySize = 256 / 8;
    const int hmacKeySize = 256 / 8;
    const int digestSize = encKeySize + hmacKeySize;

    auto result = new DerivedKeyHMAC;
    result->error = false;

    result->encrypt = QByteArray(encKeySize, '\0');
    result->hmac = QByteArray(hmacKeySize, '\0');

    const char* buffer_vp = keyKey.data();
    auto buf_len = size_t(keyKey.size());

    const int algo = GCRY_MD_SHA512;
    unsigned char digest[digestSize];
    gcry_md_hash_buffer(algo, digest, buffer_vp, buf_len);

    unsigned char* cp = digest;
    for (int i = 0, len = encKeySize; i < len; ++i) {
        result->encrypt[i] = *(cp++);
    }
    for (int i = 0, len = hmacKeySize; i < len; ++i) {
        result->hmac[i] = *(cp++);
    }

    return result;
}

/*
 * Translates the provided salt and passphrase into a derived set of keys, one for encryption
 * and one for use as a HMAC key. See https://support.1password.com/opvault-design/#key-derivation
 * @param iterations the number of rounds to apply the derivation formula
 * @return a non-null structure containing either the error or the two password-derived keys
 */
OpVaultReader::DerivedKeyHMAC*
OpVaultReader::deriveKeysFromPassPhrase(QByteArray& salt, const QString& password, unsigned long iterations)
{
    const int derivedEncKeySize = 256 / 8;
    const int derivedMACSize = 256 / 8;
    const int keysize = derivedEncKeySize + derivedMACSize;

    auto result = new DerivedKeyHMAC;
    result->error = false;

    QByteArray keybuffer(keysize, '\0');
    auto err = gcry_kdf_derive(password.toUtf8().constData(),
                               password.size(),
                               GCRY_KDF_PBKDF2,
                               GCRY_MD_SHA512,
                               salt.constData(),
                               salt.size(),
                               iterations,
                               keysize,
                               keybuffer.data());
    if (err != 0) {
        result->error = true;
        result->errorStr = tr("Unable to derive master key: %1").arg(gcry_strerror(err));
        return result;
    }
    if (keysize != keybuffer.size()) {
        qWarning() << "Calling PBKDF2(keysize=" << keysize << "yielded" << keybuffer.size() << "bytes";
    }

    QByteArray::const_iterator it = keybuffer.cbegin();

    result->encrypt = QByteArray(derivedEncKeySize, '\0');
    for (int i = 0, len = derivedEncKeySize; i < len && it != keybuffer.cend(); ++i, ++it) {
        result->encrypt[i] = *it;
    }

    result->hmac = QByteArray(derivedMACSize, '\0');
    for (int i = 0; i < derivedMACSize && it != keybuffer.cend(); ++i, ++it) {
        result->hmac[i] = *it;
    }
    return result;
}

/*!
 * \sa https://support.1password.com/opvault-design/#category
 */
void OpVaultReader::populateCategoryGroups(Group* rootGroup)
{
    QMap<QString, QString> categoryMap;
    categoryMap.insert("001", "Login");
    categoryMap.insert("002", "Credit Card");
    categoryMap.insert("003", "Secure Note");
    categoryMap.insert("004", "Identity");
    categoryMap.insert("005", "Password");
    categoryMap.insert("099", "Tombstone");
    categoryMap.insert("100", "Software License");
    categoryMap.insert("101", "Bank Account");
    categoryMap.insert("102", "Database");
    categoryMap.insert("103", "Driver License");
    categoryMap.insert("104", "Outdoor License");
    categoryMap.insert("105", "Membership");
    categoryMap.insert("106", "Passport");
    categoryMap.insert("107", "Rewards");
    categoryMap.insert("108", "SSN");
    categoryMap.insert("109", "Router");
    categoryMap.insert("110", "Server");
    categoryMap.insert("111", "Email");
    for (const QString& catNum : categoryMap.keys()) {
        const QString& category = categoryMap[catNum];
        auto g = new Group();
        g->setName(category);
        g->setProperty("code", catNum);
        g->setUpdateTimeinfo(false);
        // maybe make these stable, so folks can depend on them?
        g->setUuid(QUuid::createUuid());
        g->setParent(rootGroup);
    }
}

void OpVaultReader::zeroKeys()
{
    m_masterKey.fill('\0');
    m_masterHmacKey.fill('\0');
    m_overviewKey.fill('\0');
    m_overviewHmacKey.fill('\0');
}
