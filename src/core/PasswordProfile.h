/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_PASSWORDPROFILE_H
#define KEEPASSX_PASSWORDPROFILE_H

#include <QString>
#include <QUuid>
#include <QVariantMap>

#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"

/**
 * @brief Password profile for storing password/passphrase generator settings
 *
 * This class encapsulates settings for both password and passphrase generators
 * and provides serialization/deserialization to/from QVariantMap for storage
 * in database custom data.
 */
class PasswordProfile
{
public:
    enum ProfileType
    {
        Password,
        Passphrase
    };

    PasswordProfile();
    explicit PasswordProfile(const QString& name);

    // Profile metadata
    QUuid id() const;
    void setId(const QUuid& id);
    QString name() const;
    void setName(const QString& name);

    ProfileType type() const;
    void setType(ProfileType type);

    // Password generator settings
    void setPasswordSettings(int length,
                             const PasswordGenerator::CharClasses& classes,
                             const PasswordGenerator::GeneratorFlags& flags,
                             const QString& customCharacterSet = QString(),
                             const QString& excludedCharacterSet = QString());

    void applyPasswordSettings(PasswordGenerator* generator) const;

    // Passphrase generator settings
    void setPassphraseSettings(int wordCount,
                               PassphraseGenerator::PassphraseWordCase wordCase,
                               const QString& wordSeparator,
                               const QString& wordList = QString());

    void applyPassphraseSettings(PassphraseGenerator* generator) const;

    // Serialization
    QVariantMap toVariantMap() const;
    static PasswordProfile fromVariantMap(const QVariantMap& map);

    // Validation
    bool isValid() const;

private:
    QUuid m_id = QUuid::createUuid();
    QString m_name;
    ProfileType m_type = Password;

    // Password generator settings
    int m_passwordLength = PasswordGenerator::DefaultLength;
    PasswordGenerator::CharClasses m_charClasses = PasswordGenerator::DefaultCharset;
    PasswordGenerator::GeneratorFlags m_generatorFlags = PasswordGenerator::DefaultFlags;
    QString m_customCharacterSet;
    QString m_excludedCharacterSet;

    // Passphrase generator settings
    int m_passphraseWordCount = PassphraseGenerator::DefaultWordCount;
    PassphraseGenerator::PassphraseWordCase m_wordCase = PassphraseGenerator::LOWERCASE;
    QString m_wordSeparator = PassphraseGenerator::DefaultSeparator;
    QString m_wordList = PassphraseGenerator::DefaultWordList;
};

#endif // KEEPASSX_PASSWORDPROFILE_H
