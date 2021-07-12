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

#ifndef KEEPASSX_PASSWORDGENERATOR_H
#define KEEPASSX_PASSWORDGENERATOR_H

#include <QVector>

typedef QVector<QChar> PasswordGroup;

class PasswordGenerator
{
public:
    enum CharClass
    {
        LowerLetters = (1 << 0),
        UpperLetters = (1 << 1),
        Numbers = (1 << 2),
        Braces = (1 << 3),
        Punctuation = (1 << 4),
        Quotes = (1 << 5),
        Dashes = (1 << 6),
        Math = (1 << 7),
        Logograms = (1 << 8),
        SpecialCharacters = Braces | Punctuation | Quotes | Dashes | Math | Logograms,
        EASCII = (1 << 9),
        DefaultCharset = LowerLetters | UpperLetters | Numbers
    };
    Q_DECLARE_FLAGS(CharClasses, CharClass)

    enum GeneratorFlag
    {
        ExcludeLookAlike = (1 << 0),
        CharFromEveryGroup = (1 << 1),
        AdvancedMode = (1 << 2),
        DefaultFlags = ExcludeLookAlike | CharFromEveryGroup
    };
    Q_DECLARE_FLAGS(GeneratorFlags, GeneratorFlag)

public:
    PasswordGenerator();

    void setLength(int length);
    void setCharClasses(const CharClasses& classes);
    void setFlags(const GeneratorFlags& flags);
    void setAdditionalChars(const QString& chars);
    void setExcludedChars(const QString& chars);

    bool isValid() const;

    QString generatePassword() const;

    static const int DefaultLength = 32;
    static const char* DefaultAdditionalChars;
    static const char* DefaultExcludedChars;

private:
    QVector<PasswordGroup> passwordGroups() const;
    int numCharClasses() const;

    int m_length;
    CharClasses m_classes;
    GeneratorFlags m_flags;
    QString m_additional;
    QString m_excluded;

    Q_DISABLE_COPY(PasswordGenerator)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PasswordGenerator::CharClasses)

Q_DECLARE_OPERATORS_FOR_FLAGS(PasswordGenerator::GeneratorFlags)

#endif // KEEPASSX_PASSWORDGENERATOR_H
