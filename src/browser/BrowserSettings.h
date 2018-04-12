/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BROWSERSETTINGS_H
#define BROWSERSETTINGS_H

#include "HostInstaller.h"
#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"

class BrowserSettings
{
public:
    static bool isEnabled();
    static void setEnabled(bool enabled);

    static bool showNotification(); // TODO!!
    static void setShowNotification(bool showNotification);
    static bool bestMatchOnly();
    static void setBestMatchOnly(bool bestMatchOnly);
    static bool unlockDatabase();
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
    static bool searchInAllDatabases();
    static void setSearchInAllDatabases(bool searchInAllDatabases);
    static bool supportKphFields();
    static void setSupportKphFields(bool supportKphFields);

    static bool supportBrowserProxy();
    static void setSupportBrowserProxy(bool enabled);
    static bool useCustomProxy();
    static void setUseCustomProxy(bool enabled);
    static QString customProxyLocation();
    static void setCustomProxyLocation(QString location);
    static bool updateBinaryPath();
    static void setUpdateBinaryPath(bool enabled);
    static bool chromeSupport();
    static void setChromeSupport(bool enabled);
    static bool chromiumSupport();
    static void setChromiumSupport(bool enabled);
    static bool firefoxSupport();
    static void setFirefoxSupport(bool enabled);
    static bool vivaldiSupport();
    static void setVivaldiSupport(bool enabled);

    static bool passwordUseNumbers();
    static void setPasswordUseNumbers(bool useNumbers);
    static bool passwordUseLowercase();
    static void setPasswordUseLowercase(bool useLowercase);
    static bool passwordUseUppercase();
    static void setPasswordUseUppercase(bool useUppercase);
    static bool passwordUseSpecial();
    static void setPasswordUseSpecial(bool useSpecial);
    static bool passwordUseBraces();
    static void setPasswordUseBraces(bool useBraces);
    static bool passwordUsePunctuation();
    static void setPasswordUsePunctuation(bool usePunctuation);
    static bool passwordUseQuotes();
    static void setPasswordUseQuotes(bool useQuotes);
    static bool passwordUseDashes();
    static void setPasswordUseDashes(bool useDashes);
    static bool passwordUseMath();
    static void setPasswordUseMath(bool useMath);
    static bool passwordUseLogograms();
    static void setPasswordUseLogograms(bool useLogograms);
    static bool passwordUseEASCII();
    static void setPasswordUseEASCII(bool useEASCII);
    static bool advancedMode();
    static void setAdvancedMode(bool advancedMode);
    static QString passwordExcludedChars();
    static void setPasswordExcludedChars(QString chars);
    static int passPhraseWordCount();
    static void setPassPhraseWordCount(int wordCount);
    static QString passPhraseWordSeparator();
    static void setPassPhraseWordSeparator(QString separator);
    static int generatorType();
    static void setGeneratorType(int type);
    static bool passwordEveryGroup();
    static void setPasswordEveryGroup(bool everyGroup);
    static bool passwordExcludeAlike();
    static void setPasswordExcludeAlike(bool excludeAlike);
    static int passwordLength();
    static void setPasswordLength(int length);
    static PasswordGenerator::CharClasses passwordCharClasses();
    static PasswordGenerator::GeneratorFlags passwordGeneratorFlags();
    static QString generatePassword();
    static int getbits();
    static void updateBinaryPaths(QString customProxyLocation = QString());

private:
    static PasswordGenerator m_passwordGenerator;
    static PassphraseGenerator m_passPhraseGenerator;
    static HostInstaller m_hostInstaller;
};

#endif // BROWSERSETTINGS_H
