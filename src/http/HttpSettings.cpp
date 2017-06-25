/*
*  Copyright (C) 2013 Francois Ferrand
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

#include "HttpSettings.h"
#include "core/Config.h"

PasswordGenerator HttpSettings::m_generator;

bool HttpSettings::isEnabled()
{
    return config()->get("Http/Enabled", false).toBool();
}

void HttpSettings::setEnabled(bool enabled)
{
    config()->set("Http/Enabled", enabled);
}

bool HttpSettings::showNotification()
{
    return config()->get("Http/ShowNotification", true).toBool();
}

void HttpSettings::setShowNotification(bool showNotification)
{
    config()->set("Http/ShowNotification", showNotification);
}

bool HttpSettings::bestMatchOnly()
{
    return config()->get("Http/BestMatchOnly", false).toBool();
}

void HttpSettings::setBestMatchOnly(bool bestMatchOnly)
{
    config()->set("Http/BestMatchOnly", bestMatchOnly);
}

bool HttpSettings::unlockDatabase()
{
    return config()->get("Http/UnlockDatabase", true).toBool();
}

void HttpSettings::setUnlockDatabase(bool unlockDatabase)
{
    config()->set("Http/UnlockDatabase", unlockDatabase);
}

bool HttpSettings::matchUrlScheme()
{
    return config()->get("Http/MatchUrlScheme", true).toBool();
}

void HttpSettings::setMatchUrlScheme(bool matchUrlScheme)
{
    config()->set("Http/MatchUrlScheme", matchUrlScheme);
}

bool HttpSettings::sortByUsername()
{
    return config()->get("Http/SortByUsername", false).toBool();
}

void HttpSettings::setSortByUsername(bool sortByUsername)
{
    config()->set("Http/SortByUsername", sortByUsername);
}

bool HttpSettings::sortByTitle()
{
    return !sortByUsername();
}

void HttpSettings::setSortByTitle(bool sortByUsertitle)
{
    setSortByUsername(!sortByUsertitle);
}

bool HttpSettings::alwaysAllowAccess()
{
    return config()->get("Http/AlwaysAllowAccess", false).toBool();
}

void HttpSettings::setAlwaysAllowAccess(bool alwaysAllowAccess)
{
    config()->set("Http/AlwaysAllowAccess", alwaysAllowAccess);
}

bool HttpSettings::alwaysAllowUpdate()
{
    return config()->get("Http/AlwaysAllowUpdate", false).toBool();
}

void HttpSettings::setAlwaysAllowUpdate(bool alwaysAllowUpdate)
{
    config()->set("Http/AlwaysAllowUpdate", alwaysAllowUpdate);
}

bool HttpSettings::searchInAllDatabases()
{
    return config()->get("Http/SearchInAllDatabases", false).toBool();
}

void HttpSettings::setSearchInAllDatabases(bool searchInAllDatabases)
{
    config()->set("Http/SearchInAllDatabases", searchInAllDatabases);
}

bool HttpSettings::supportKphFields()
{
    return config()->get("Http/SupportKphFields", true).toBool();
}

void HttpSettings::setSupportKphFields(bool supportKphFields)
{
    config()->set("Http/SupportKphFields", supportKphFields);
}

int  HttpSettings::httpPort()
{
    static const int PORT = 19455;

    return config()->get("Http/Port", PORT).toInt();
}

void HttpSettings::setHttpPort(int port)
{
    config()->set("Http/Port", port);
}

bool HttpSettings::passwordUseNumbers()
{
    return config()->get("Http/generator/Numbers", true).toBool();
}

void HttpSettings::setPasswordUseNumbers(bool useNumbers)
{
    config()->set("Http/generator/Numbers", useNumbers);
}

bool HttpSettings::passwordUseLowercase()
{
    return config()->get("Http/generator/LowerCase", true).toBool();
}

void HttpSettings::setPasswordUseLowercase(bool useLowercase)
{
    config()->set("Http/generator/LowerCase", useLowercase);
}

bool HttpSettings::passwordUseUppercase()
{
    return config()->get("Http/generator/UpperCase", true).toBool();
}

void HttpSettings::setPasswordUseUppercase(bool useUppercase)
{
    config()->set("Http/generator/UpperCase", useUppercase);
}

bool HttpSettings::passwordUseSpecial()
{
    return config()->get("Http/generator/SpecialChars", false).toBool();
}

void HttpSettings::setPasswordUseSpecial(bool useSpecial)
{
    config()->set("Http/generator/SpecialChars", useSpecial);
}

bool HttpSettings::passwordEveryGroup()
{
    return config()->get("Http/generator/EnsureEvery", true).toBool();
}

void HttpSettings::setPasswordEveryGroup(bool everyGroup)
{
    config()->get("Http/generator/EnsureEvery", everyGroup);
}

bool HttpSettings::passwordExcludeAlike()
{
    return config()->get("Http/generator/ExcludeAlike", true).toBool();
}

void HttpSettings::setPasswordExcludeAlike(bool excludeAlike)
{
    config()->set("Http/generator/ExcludeAlike", excludeAlike);
}

int HttpSettings::passwordLength()
{
    return config()->get("Http/generator/Length", 20).toInt();
}

void HttpSettings::setPasswordLength(int length)
{
    config()->set("Http/generator/Length", length);
    m_generator.setLength(length);
}

PasswordGenerator::CharClasses HttpSettings::passwordCharClasses()
{
    PasswordGenerator::CharClasses classes;
    if (passwordUseLowercase())
        classes |= PasswordGenerator::LowerLetters;
    if (passwordUseUppercase())
        classes |= PasswordGenerator::UpperLetters;
    if (passwordUseNumbers())
        classes |= PasswordGenerator::Numbers;
    if (passwordUseSpecial())
        classes |= PasswordGenerator::SpecialCharacters;
    return classes;
}

PasswordGenerator::GeneratorFlags HttpSettings::passwordGeneratorFlags()
{
    PasswordGenerator::GeneratorFlags flags;
    if (passwordExcludeAlike())
        flags |= PasswordGenerator::ExcludeLookAlike;
    if (passwordEveryGroup())
        flags |= PasswordGenerator::CharFromEveryGroup;
    return flags;
}

QString HttpSettings::generatePassword()
{
    m_generator.setLength(passwordLength());
    m_generator.setCharClasses(passwordCharClasses());
    m_generator.setFlags(passwordGeneratorFlags());

    return m_generator.generatePassword();
}

int HttpSettings::getbits()
{
    return m_generator.getbits();
}
