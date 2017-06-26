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
    static int  httpPort();
    static void setHttpPort(int port);

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
    static QString generatePassword();
    static int getbits();

private:
    static PasswordGenerator m_generator;
};

#endif // HTTPSETTINGS_H
