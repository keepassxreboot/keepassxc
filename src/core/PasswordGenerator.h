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

#ifndef KEEPASSX_PASSWORDGENERATOR_H
#define KEEPASSX_PASSWORDGENERATOR_H

#include <QtCore/QFlags>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "core/Global.h"

typedef QVector<QChar> PasswordGroup;

class PasswordGenerator
{
public:
    enum CharClass
    {
        LowerLetters      = 0x1,
        UpperLetters      = 0x2,
        Numbers           = 0x4,
        SpecialCharacters = 0x8
    };
    Q_DECLARE_FLAGS(CharClasses, CharClass)

    enum GeneratorFlag
    {
        ExcludeLookAlike   = 0x1,
        CharFromEveryGroup = 0x2
    };
    Q_DECLARE_FLAGS(GeneratorFlags, GeneratorFlag)

    QString generatePassword(int length, const PasswordGenerator::CharClasses& classes,
                             const PasswordGenerator::GeneratorFlags& flags);
    bool isValidCombination(int length, const PasswordGenerator::CharClasses& classes,
                            const PasswordGenerator::GeneratorFlags& flags);

    static PasswordGenerator* instance();

private:
    PasswordGenerator();

    QVector<PasswordGroup> passwordGroups(const PasswordGenerator::CharClasses& classes,
                                          const PasswordGenerator::GeneratorFlags& flags);
    int numCharClasses(const PasswordGenerator::CharClasses& classes);

    static PasswordGenerator* m_instance;

    Q_DISABLE_COPY(PasswordGenerator)
};

inline PasswordGenerator* passwordGenerator() {
    return PasswordGenerator::instance();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(PasswordGenerator::CharClasses)

Q_DECLARE_OPERATORS_FOR_FLAGS(PasswordGenerator::GeneratorFlags)

#endif // KEEPASSX_PASSWORDGENERATOR_H
