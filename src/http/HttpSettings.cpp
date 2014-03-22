/**
 ***************************************************************************
 * @file HttpSettings.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "HttpSettings.h"
#include "core/Config.h"

bool HttpSettings::isEnabled()
{
    return config()->get("Http/Enabled", true).toBool();
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
    config()->set("Http/AlwaysAllowAccess", alwaysAllowUpdate);
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

bool HttpSettings::passwordUseNumbers()
{
    return config()->get("Http/PasswordUseNumbers", true).toBool();
}

void HttpSettings::setPasswordUseNumbers(bool useNumbers)
{
    config()->set("Http/PasswordUseNumbers", useNumbers);
}

bool HttpSettings::passwordUseLowercase()
{
    return config()->get("Http/PasswordUseLowercase", true).toBool();
}

void HttpSettings::setPasswordUseLowercase(bool useLowercase)
{
    config()->set("Http/PasswordUseLowercase", useLowercase);
}

bool HttpSettings::passwordUseUppercase()
{
    return config()->get("Http/PasswordUseUppercase", true).toBool();
}

void HttpSettings::setPasswordUseUppercase(bool useUppercase)
{
    config()->set("Http/PasswordUseUppercase", useUppercase);
}

bool HttpSettings::passwordUseSpecial()
{
    return config()->get("Http/PasswordUseSpecial", false).toBool();
}

void HttpSettings::setPasswordUseSpecial(bool useSpecial)
{
    config()->set("Http/PasswordUseSpecial", useSpecial);
}

bool HttpSettings::passwordEveryGroup()
{
    return config()->get("Http/PasswordEveryGroup", true).toBool();
}

void HttpSettings::setPasswordEveryGroup(bool everyGroup)
{
    config()->get("Http/PasswordEveryGroup", everyGroup);
}

bool HttpSettings::passwordExcludeAlike()
{
    return config()->get("Http/PasswordExcludeAlike", true).toBool();
}

void HttpSettings::setPasswordExcludeAlike(bool excludeAlike)
{
    config()->set("Http/PasswordExcludeAlike", excludeAlike);
}

int HttpSettings::passwordLength()
{
    return config()->get("Http/PasswordLength", 20).toInt();
}

void HttpSettings::setPasswordLength(int length)
{
    config()->set("Http/PasswordLength", length);
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
