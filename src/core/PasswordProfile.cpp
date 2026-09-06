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

#include "PasswordProfile.h"
#include "core/Resources.h"
#include <QDir>
#include <QFileInfo>

PasswordProfile::PasswordProfile()
{
}

PasswordProfile::PasswordProfile(const QString& name)
    : m_name(name)
{
}

QUuid PasswordProfile::id() const
{
    return m_id;
}

void PasswordProfile::setId(const QUuid& id)
{
    m_id = id;
}

QString PasswordProfile::name() const
{
    return m_name;
}

void PasswordProfile::setName(const QString& name)
{
    m_name = name;
}

PasswordProfile::ProfileType PasswordProfile::type() const
{
    return m_type;
}

void PasswordProfile::setType(ProfileType type)
{
    m_type = type;
}

void PasswordProfile::setPasswordSettings(int length,
                                          const PasswordGenerator::CharClasses& classes,
                                          const PasswordGenerator::GeneratorFlags& flags,
                                          const QString& customCharacterSet,
                                          const QString& excludedCharacterSet)
{
    m_type = Password;
    m_passwordLength = length;
    m_charClasses = classes;
    m_generatorFlags = flags;
    m_customCharacterSet = customCharacterSet;
    m_excludedCharacterSet = excludedCharacterSet;
}

void PasswordProfile::applyPasswordSettings(PasswordGenerator* generator) const
{
    if (m_type != Password || !generator) {
        return;
    }

    generator->setLength(m_passwordLength);
    generator->setCharClasses(m_charClasses);
    generator->setFlags(m_generatorFlags);
    generator->setCustomCharacterSet(m_customCharacterSet);
    generator->setExcludedCharacterSet(m_excludedCharacterSet);
}

void PasswordProfile::setPassphraseSettings(int wordCount,
                                            PassphraseGenerator::PassphraseWordCase wordCase,
                                            const QString& wordSeparator,
                                            const QString& wordList)
{
    m_type = Passphrase;
    m_passphraseWordCount = wordCount;
    m_wordCase = wordCase;
    m_wordSeparator = wordSeparator;
    m_wordList = wordList.isEmpty() ? QString(PassphraseGenerator::DefaultWordList) : wordList;
}

void PasswordProfile::applyPassphraseSettings(PassphraseGenerator* generator) const
{
    if (m_type != Passphrase || !generator) {
        return;
    }

    generator->setWordCount(m_passphraseWordCount);
    generator->setWordCase(m_wordCase);
    generator->setWordSeparator(m_wordSeparator);
    QString path;
    const QFileInfo wordList(m_wordList);
    if (wordList.fileName() == m_wordList) {
        path = resources()->wordlistPath(m_wordList);
    } else if (QDir::cleanPath(m_wordList) == QDir::cleanPath(resources()->userWordlistPath(wordList.fileName()))) {
        path = m_wordList;
    }
    generator->setWordList(path);
}

QVariantMap PasswordProfile::toVariantMap() const
{
    QVariantMap map;

    map["id"] = m_id.toString(QUuid::WithoutBraces);
    map["name"] = m_name;
    map["type"] = static_cast<int>(m_type);

    if (m_type == Password) {
        map["passwordLength"] = m_passwordLength;
        map["charClasses"] = static_cast<int>(m_charClasses);
        map["generatorFlags"] = static_cast<int>(m_generatorFlags);
        map["customCharacterSet"] = m_customCharacterSet;
        map["excludedCharacterSet"] = m_excludedCharacterSet;
    } else if (m_type == Passphrase) {
        map["passphraseWordCount"] = m_passphraseWordCount;
        map["wordCase"] = static_cast<int>(m_wordCase);
        map["wordSeparator"] = m_wordSeparator;
        map["wordList"] = m_wordList;
    }

    return map;
}

PasswordProfile PasswordProfile::fromVariantMap(const QVariantMap& map)
{
    PasswordProfile profile;

    profile.m_id = QUuid(map.value("id").toString());
    profile.m_name = map.value("name").toString();
    // Reject incomplete or mistyped settings rather than silently weakening a policy.
    const QStringList integerKeys = map.value("type").toInt() == Password
                                        ? QStringList{"type", "passwordLength", "charClasses", "generatorFlags"}
                                        : QStringList{"type", "passphraseWordCount", "wordCase"};
    for (const auto& key : integerKeys) {
        const auto value = map.value(key);
        bool ok = false;
        const auto number = value.toDouble(&ok);
        if (!ok || value.metaType().id() == QMetaType::QString || value.metaType().id() == QMetaType::Bool
            || number != value.toInt()) {
            return PasswordProfile();
        }
    }
    const QStringList stringKeys = map.value("type").toInt() == Password
                                       ? QStringList{"id", "name", "customCharacterSet", "excludedCharacterSet"}
                                       : QStringList{"id", "name", "wordSeparator", "wordList"};
    for (const auto& key : stringKeys) {
        if (map.value(key).metaType().id() != QMetaType::QString) {
            return PasswordProfile();
        }
    }
    profile.m_type = static_cast<ProfileType>(map.value("type", Password).toInt());

    if (profile.m_type == Password) {
        profile.m_passwordLength = map.value("passwordLength", PasswordGenerator::DefaultLength).toInt();
        profile.m_charClasses = static_cast<PasswordGenerator::CharClasses>(
            map.value("charClasses", static_cast<int>(PasswordGenerator::DefaultCharset)).toInt());
        profile.m_generatorFlags = static_cast<PasswordGenerator::GeneratorFlags>(
            map.value("generatorFlags", static_cast<int>(PasswordGenerator::DefaultFlags)).toInt());
        profile.m_customCharacterSet = map.value("customCharacterSet").toString();
        profile.m_excludedCharacterSet = map.value("excludedCharacterSet").toString();
    } else if (profile.m_type == Passphrase) {
        profile.m_passphraseWordCount = map.value("passphraseWordCount", PassphraseGenerator::DefaultWordCount).toInt();
        profile.m_wordCase = static_cast<PassphraseGenerator::PassphraseWordCase>(
            map.value("wordCase", static_cast<int>(PassphraseGenerator::LOWERCASE)).toInt());
        profile.m_wordSeparator = map.value("wordSeparator", PassphraseGenerator::DefaultSeparator).toString();
        profile.m_wordList = map.value("wordList", PassphraseGenerator::DefaultWordList).toString();
    }

    return profile;
}

bool PasswordProfile::isValid() const
{
    if (m_name.trimmed().isEmpty() || m_id.isNull()) {
        return false;
    }

    if (m_type == Password) {
        if (m_passwordLength < 1 || m_passwordLength > 999 || (int(m_charClasses) & ~1023)
            || (int(m_generatorFlags) & ~7)) {
            return false;
        }
        PasswordGenerator generator;
        applyPasswordSettings(&generator);
        return generator.isValid();
    } else if (m_type == Passphrase) {
        return m_passphraseWordCount > 0 && m_passphraseWordCount <= 100 && m_wordCase >= PassphraseGenerator::LOWERCASE
               && m_wordCase <= PassphraseGenerator::MIXEDCASE && !m_wordList.isEmpty();
    }

    return false;
}
