/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "PasswordGenerator.h"

#include "crypto/Random.h"
#include <zxcvbn.h>
#include <iostream>

PasswordGenerator::PasswordGenerator()
    : m_length(0)
    , m_classes(0)
    , m_flags(0)
{
}

double PasswordGenerator::calculateEntropy(QString password)
{
    return ZxcvbnMatch(password.toLatin1(), 0, 0);
}

void PasswordGenerator::setLength(int length)
{
    if (length <= 0) {
        m_length = DefaultLength;
        return;
    }
    m_length = length;
}

void PasswordGenerator::setCharClasses(const CharClasses& classes)
{
    if (classes == 0) {
        m_classes =  DefaultCharset;
        return;
    }
    m_classes = classes;
}

void PasswordGenerator::setFlags(const GeneratorFlags& flags)
{
    m_flags = flags;
}

void PasswordGenerator::setExcludeChars(const QString& excludeChars)
{
    m_excludeChars = excludeChars;
}

QString PasswordGenerator::generatePassword() const
{
    Q_ASSERT(isValid());

    QVector<PasswordGroup> groups = passwordGroups();
    QVector<QChar> chars = passwordChars();
    QString password;

    if (m_flags & CharFromEveryGroup) {
        for (int i = 0; i < groups.size(); i++) {
          QChar ch;
          do
          {
            int pos = randomGen()->randomUInt(groups[i].size());
            ch = groups[i][pos];
          }
          while(m_excludeChars.contains(ch));
          password.append(ch);
        }

        for (int i = groups.size(); i < m_length; i++) {
            int pos = randomGen()->randomUInt(chars.size());

            password.append(chars[pos]);
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
            int pos = randomGen()->randomUInt(chars.size());

            password.append(chars[pos]);
        }
    }

    return password;
}

int PasswordGenerator::getbits() const
{
    const QVector<PasswordGroup> groups = passwordGroups();

    int bits = 0;
    QVector<QChar> passwordChars;
    for (const PasswordGroup& group: groups) {
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

    if(passwordChars().isEmpty())
      return false;

    return true;
}

QVector<QChar> PasswordGenerator::passwordChars() const
{
  const QVector<PasswordGroup> groups = passwordGroups();
  QVector<QChar> passwordChars;
  for (const PasswordGroup& group : groups) {
    for (QChar ch : group) {
      if(!m_excludeChars.contains(ch))
      {
        passwordChars.append(ch);
      }
    }
  }
  return passwordChars;
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
    if (m_classes & EASCII) {
        PasswordGroup group;

        // [U+0080, U+009F] are C1 control characters,
        // U+00A0 is non-breaking space
        for (int i = 161; i <= 172; i++) {
            group.append(i);
        }
        // U+00AD is soft hyphen (format character)
        for (int i = 174; i <= 255; i++) {
            if ((m_flags & ExcludeLookAlike) && (i == 249)) { // "﹒"
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
    if (m_classes & EASCII) {
        numClasses++;
    }

    return numClasses;
}
