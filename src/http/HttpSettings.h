/**
 ***************************************************************************
 * @file HttpSettings.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef HTTPSETTINGS_H
#define HTTPSETTINGS_H

#include "core/PasswordGenerator.h"

class HttpSettings
{
public:
    static bool isEnabled();
    static void setEnabled(bool enabled);

    static bool showNotification();  //TODO!!
    static void setShowNotification(bool showNotification);
    static bool bestMatchOnly();     //TODO!!
    static void setBestMatchOnly(bool bestMatchOnly);
    static bool unlockDatabase();    //TODO!!
    static void setUnlockDatabase(bool unlockDatabase);
    static bool matchUrlScheme();
    static void setMatchUrlScheme(bool matchUrlScheme);
    static bool sortByUsername();
    static void setSortByUsername(bool sortByUsername = true);
    static bool sortByTitle();
    static void setSortByTitle(bool sortByUsertitle = true);
    static bool alwaysAllowAccess();
    static void setAlwaysAllowAccess(bool alwaysAllowAccess);
    static bool alwaysAllowUpdate();
    static void setAlwaysAllowUpdate(bool alwaysAllowUpdate);
    static bool searchInAllDatabases();//TODO!!
    static void setSearchInAllDatabases(bool searchInAllDatabases);
    static bool supportKphFields();
    static void setSupportKphFields(bool supportKphFields);

    static bool passwordUseNumbers();
    static void setPasswordUseNumbers(bool useNumbers);
    static bool passwordUseLowercase();
    static void setPasswordUseLowercase(bool useLowercase);
    static bool passwordUseUppercase();
    static void setPasswordUseUppercase(bool useUppercase);
    static bool passwordUseSpecial();
    static void setPasswordUseSpecial(bool useSpecial);
    static bool passwordEveryGroup();
    static void setPasswordEveryGroup(bool everyGroup);
    static bool passwordExcludeAlike();
    static void setPasswordExcludeAlike(bool excludeAlike);
    static int  passwordLength();
    static void setPasswordLength(int length);
    static PasswordGenerator::CharClasses passwordCharClasses();
    static PasswordGenerator::GeneratorFlags passwordGeneratorFlags();
};

#endif // HTTPSETTINGS_H
