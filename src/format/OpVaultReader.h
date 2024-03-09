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

#ifndef OPVAULT_READER_H_
#define OPVAULT_READER_H_

#include <QDir>

class Database;
class Group;
class Entry;

/*!
 * Imports a directory in the 1Password \c opvault format into a \c Database.
 * \sa https://support.1password.com/opvault-overview/
 * \sa https://support.1password.com/opvault-design/
 * \sa https://cache.agilebits.com/security-kb/freddy-2013-12-04.tar.gz is the sample data used to test this class,
 * and its password is \c freddy
 */
class OpVaultReader : public QObject
{
    Q_OBJECT

public:
    explicit OpVaultReader(QObject* parent = nullptr);
    ~OpVaultReader() override;

    QSharedPointer<Database> convert(QDir& opdataDir, const QString& password);

    bool hasError();
    QString errorString();

private:
    struct DerivedKeyHMAC
    {
        QByteArray encrypt;
        QByteArray hmac;
        QString error;
    };

    QJsonObject readAndAssertJsonFile(QFile& file, const QString& stripLeading, const QString& stripTrailing);

    DerivedKeyHMAC* deriveKeysFromPassPhrase(QByteArray& salt, const QString& password, unsigned long iterations);
    DerivedKeyHMAC* decodeB64CompositeKeys(const QString& b64, const QByteArray& encKey, const QByteArray& hmacKey);
    DerivedKeyHMAC* decodeCompositeKeys(const QByteArray& keyKey);

    /*!
     * \sa https://support.1password.com/opvault-design/#profile-js
     * @param profileJson the contents of \c profile.js
     * @return \c true if the profile data was decrypted successfully, \c false otherwise
     */
    bool processProfileJson(QJsonObject& profileJson, const QString& password, Group* rootGroup);

    /*!
     * \sa https://support.1password.com/opvault-design/#folders-js
     * @param foldersJson the map from a folder UUID to its data (name and any smart query)
     * @return \c true if the folder data was decrypted successfully, \c false otherwise
     */
    bool processFolderJson(QJsonObject& foldersJson, Group* rootGroup);

    /*!
     * Decrypts the provided band object into its interior structure,
     * as well as the encryption key and HMAC key declared therein,
     * which are used to decrypt the attachments, also.
     * @returns \c nullptr if unable to do the decryption, otherwise the interior object and its keys
     */
    bool decryptBandEntry(const QJsonObject& bandEntry, QJsonObject& data, QByteArray& key, QByteArray& hmacKey);
    Entry* processBandEntry(const QJsonObject& bandEntry, const QDir& attachmentDir, Group* rootGroup);

    bool readAttachment(const QString& filePath,
                        const QByteArray& itemKey,
                        const QByteArray& itemHmacKey,
                        QJsonObject& metadata,
                        QByteArray& payload);
    void fillAttachment(Entry* entry,
                        const QFileInfo& attachmentFileInfo,
                        const QByteArray& entryKey,
                        const QByteArray& entryHmacKey);
    void fillAttachments(Entry* entry,
                         const QDir& attachmentDir,
                         const QByteArray& entryKey,
                         const QByteArray& entryHmacKey);

    bool fillAttributes(Entry* entry, const QJsonObject& bandEntry);

    void fillFromSection(Entry* entry, const QJsonObject& section);
    void fillFromSectionField(Entry* entry, const QString& sectionName, const QJsonObject& field);
    QString resolveAttributeName(const QString& section, const QString& name, const QString& text);

    void populateCategoryGroups(Group* rootGroup);
    /*! Used to blank the memory after the keys have been used. */
    void zeroKeys();

    QString m_error;
    QByteArray m_masterKey;
    QByteArray m_masterHmacKey;
    /*! Used to decrypt overview text, such as folder names. */
    QByteArray m_overviewKey;
    QByteArray m_overviewHmacKey;

    friend class TestImports;
};

#endif /* OPVAULT_READER_H_ */
