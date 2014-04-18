/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "PasswordGenerator.h"

#include "crypto/Random.h"

PasswordGenerator::PasswordGenerator()
    : m_length(0)
    , m_classes(0)
    , m_flags(0)
{
}

void PasswordGenerator::setLength(int length)
{
    m_length = length;
}

void PasswordGenerator::setCharClasses(const CharClasses& classes)
{
    m_classes = classes;
}

void PasswordGenerator::setFlags(const GeneratorFlags& flags)
{
    m_flags = flags;
}

QString PasswordGenerator::generatePassword() const
{
    Q_ASSERT(isValid());

    QVector<PasswordGroup> groups = passwordGroups();

    QVector<QChar> passwordChars;
    Q_FOREACH (const PasswordGroup& group, groups) {
        Q_FOREACH (QChar ch, group) {
            passwordChars.append(ch);
        }
    }

    QString password;

    if (m_flags & CharFromEveryGroup) {
        for (int i = 0; i < groups.size(); i++) {
            int pos = randomGen()->randomUInt(groups[i].size());

            password.append(groups[i][pos]);
        }

        for (int i = groups.size(); i < m_length; i++) {
            int pos = randomGen()->randomUInt(passwordChars.size());

            password.append(passwordChars[pos]);
        }

        // shuffle chars
        for (int i = (password.size() - 1); i >= 1; i--) {
            int j = randomGen()->randomUInt(i + 1);

            QChar tmp = password[i];
            password[i] = password[j];
            password[j] = tmp;
        }
    }
    else {
        for (int i = 0; i < m_length; i++) {
            int pos = randomGen()->randomUInt(passwordChars.size());

            password.append(passwordChars[pos]);
        }
    }

    return password;
}

int PasswordGenerator::getbits() const
{
    QVector<PasswordGroup> groups = passwordGroups();

    int bits = 0;
    QVector<QChar> passwordChars;
    Q_FOREACH (const PasswordGroup& group, groups) {
        bits += group.size();
    }

    bits *= m_length;

    return bits;
}


bool PasswordGenerator::isValid() const
{
    if (m_classes == 0) {
        return false;
    }
    else if (m_length == 0) {
        return false;
    }

    if ((m_flags & CharFromEveryGroup) && (m_length < numCharClasses())) {
        return false;
    }

    return true;
}

QVector<PasswordGroup> PasswordGenerator::passwordGroups() const
{
    QVector<PasswordGroup> passwordGroups;

    if (m_classes & LowerLetters) {
        PasswordGroup group;

        for (int i = 97; i < (97 + 26); i++) {
            if ((m_flags & ExcludeLookAlike) && (i == 108)) { // "l"
                continue;
            }

            group.append(i);
        }

        passwordGroups.append(group);
    }
    if (m_classes & UpperLetters) {
        PasswordGroup group;

        for (int i = 65; i < (65 + 26); i++) {
            if ((m_flags & ExcludeLookAlike) && (i == 73 || i == 79)) { // "I" and "O"
                continue;
            }

            group.append(i);
        }

        passwordGroups.append(group);
    }
    if (m_classes & Numbers) {
        PasswordGroup group;

        for (int i = 48; i < (48 + 10); i++) {
            if ((m_flags & ExcludeLookAlike) && (i == 48 || i == 49)) { // "0" and "1"
                continue;
            }

            group.append(i);
        }

        passwordGroups.append(group);
    }
    if (m_classes & SpecialCharacters) {
        PasswordGroup group;

        for (int i = 33; i <= 47; i++) {
            group.append(i);
        }

        for (int i = 58; i <= 64; i++) {
            group.append(i);
        }

        for (int i = 91; i <= 96; i++) {
            group.append(i);
        }

        for (int i = 123; i <= 126; i++) {
            if ((m_flags & ExcludeLookAlike) && (i == 124)) { // "|"
                continue;
            }

            group.append(i);
        }

        passwordGroups.append(group);
    }

    return passwordGroups;
}

int PasswordGenerator::numCharClasses() const
{
    int numClasses = 0;

    if (m_classes & LowerLetters) {
        numClasses++;
    }
    if (m_classes & UpperLetters) {
        numClasses++;
    }
    if (m_classes & Numbers) {
        numClasses++;
    }
    if (m_classes & SpecialCharacters) {
        numClasses++;
    }

    return numClasses;
}
