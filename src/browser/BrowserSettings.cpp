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

#include "BrowserSettings.h"
#include "core/Config.h"

PasswordGenerator BrowserSettings::m_passwordGenerator;
PassphraseGenerator BrowserSettings::m_passPhraseGenerator;
HostInstaller BrowserSettings::m_hostInstaller;

bool BrowserSettings::isEnabled()
{
    return config()->get("Browser/Enabled", false).toBool();
}

void BrowserSettings::setEnabled(bool enabled)
{
    config()->set("Browser/Enabled", enabled);
}

bool BrowserSettings::showNotification()
{
    return config()->get("Browser/ShowNotification", true).toBool();
}

void BrowserSettings::setShowNotification(bool showNotification)
{
    config()->set("Browser/ShowNotification", showNotification);
}

bool BrowserSettings::bestMatchOnly()
{
    return config()->get("Browser/BestMatchOnly", false).toBool();
}

void BrowserSettings::setBestMatchOnly(bool bestMatchOnly)
{
    config()->set("Browser/BestMatchOnly", bestMatchOnly);
}

bool BrowserSettings::unlockDatabase()
{
    return config()->get("Browser/UnlockDatabase", true).toBool();
}

void BrowserSettings::setUnlockDatabase(bool unlockDatabase)
{
    config()->set("Browser/UnlockDatabase", unlockDatabase);
}

bool BrowserSettings::matchUrlScheme()
{
    return config()->get("Browser/MatchUrlScheme", true).toBool();
}

void BrowserSettings::setMatchUrlScheme(bool matchUrlScheme)
{
    config()->set("Browser/MatchUrlScheme", matchUrlScheme);
}

bool BrowserSettings::sortByUsername()
{
    return config()->get("Browser/SortByUsername", false).toBool();
}

void BrowserSettings::setSortByUsername(bool sortByUsername)
{
    config()->set("Browser/SortByUsername", sortByUsername);
}

bool BrowserSettings::sortByTitle()
{
    return !sortByUsername();
}

void BrowserSettings::setSortByTitle(bool sortByUsertitle)
{
    setSortByUsername(!sortByUsertitle);
}

bool BrowserSettings::alwaysAllowAccess()
{
    return config()->get("Browser/AlwaysAllowAccess", false).toBool();
}

void BrowserSettings::setAlwaysAllowAccess(bool alwaysAllowAccess)
{
    config()->set("Browser/AlwaysAllowAccess", alwaysAllowAccess);
}

bool BrowserSettings::alwaysAllowUpdate()
{
    return config()->get("Browser/AlwaysAllowUpdate", false).toBool();
}

void BrowserSettings::setAlwaysAllowUpdate(bool alwaysAllowUpdate)
{
    config()->set("Browser/AlwaysAllowUpdate", alwaysAllowUpdate);
}

bool BrowserSettings::searchInAllDatabases()
{
    return config()->get("Browser/SearchInAllDatabases", false).toBool();
}

void BrowserSettings::setSearchInAllDatabases(bool searchInAllDatabases)
{
    config()->set("Browser/SearchInAllDatabases", searchInAllDatabases);
}

bool BrowserSettings::supportKphFields()
{
    return config()->get("Browser/SupportKphFields", true).toBool();
}

void BrowserSettings::setSupportKphFields(bool supportKphFields)
{
    config()->set("Browser/SupportKphFields", supportKphFields);
}

bool BrowserSettings::supportBrowserProxy()
{
    return config()->get("Browser/SupportBrowserProxy", true).toBool();
}

void BrowserSettings::setSupportBrowserProxy(bool enabled)
{
    config()->set("Browser/SupportBrowserProxy", enabled);
}

bool BrowserSettings::useCustomProxy()
{
    return config()->get("Browser/UseCustomProxy", false).toBool();
}

void BrowserSettings::setUseCustomProxy(bool enabled)
{
    config()->set("Browser/UseCustomProxy", enabled);
}

QString BrowserSettings::customProxyLocation()
{
    if (!useCustomProxy()) {
        return QString();
    }
    return config()->get("Browser/CustomProxyLocation", "").toString();
}

void BrowserSettings::setCustomProxyLocation(QString location)
{
    config()->set("Browser/CustomProxyLocation", location);
}

bool BrowserSettings::updateBinaryPath()
{
    return config()->get("Browser/UpdateBinaryPath", false).toBool();
}

void BrowserSettings::setUpdateBinaryPath(bool enabled)
{
    config()->set("Browser/UpdateBinaryPath", enabled);
}

bool BrowserSettings::chromeSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::CHROME);
}

void BrowserSettings::setChromeSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::CHROME, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::chromiumSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::CHROMIUM);
}

void BrowserSettings::setChromiumSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::CHROMIUM, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::firefoxSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::FIREFOX);
}

void BrowserSettings::setFirefoxSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::FIREFOX, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::vivaldiSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::VIVALDI);
}

void BrowserSettings::setVivaldiSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::VIVALDI, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::passwordUseNumbers()
{
    return config()->get("generator/Numbers", PasswordGenerator::DefaultNumbers).toBool();
}

void BrowserSettings::setPasswordUseNumbers(bool useNumbers)
{
    config()->set("generator/Numbers", useNumbers);
}

bool BrowserSettings::passwordUseLowercase()
{
    return config()->get("generator/LowerCase", PasswordGenerator::DefaultLower).toBool();
}

void BrowserSettings::setPasswordUseLowercase(bool useLowercase)
{
    config()->set("generator/LowerCase", useLowercase);
}

bool BrowserSettings::passwordUseUppercase()
{
    return config()->get("generator/UpperCase", PasswordGenerator::DefaultUpper).toBool();
}

void BrowserSettings::setPasswordUseUppercase(bool useUppercase)
{
    config()->set("generator/UpperCase", useUppercase);
}

bool BrowserSettings::passwordUseSpecial()
{
    return config()->get("generator/SpecialChars", PasswordGenerator::DefaultSpecial).toBool();
}

void BrowserSettings::setPasswordUseSpecial(bool useSpecial)
{
    config()->set("generator/SpecialChars", useSpecial);
}

bool BrowserSettings::passwordUseBraces()
{
    return config()->get("generator/Braces", PasswordGenerator::DefaultBraces).toBool();
}

void BrowserSettings::setPasswordUseBraces(bool useBraces)
{
    config()->set("generator/Braces", useBraces);
}

bool BrowserSettings::passwordUsePunctuation()
{
    return config()->get("generator/Punctuation", PasswordGenerator::DefaultQuotes).toBool();
}

void BrowserSettings::setPasswordUsePunctuation(bool usePunctuation)
{
    config()->set("generator/Punctuation", usePunctuation);
}

bool BrowserSettings::passwordUseQuotes()
{
    return config()->get("generator/Quotes", PasswordGenerator::DefaultQuotes).toBool();
}

void BrowserSettings::setPasswordUseQuotes(bool useQuotes)
{
    config()->set("generator/Quotes", useQuotes);
}

bool BrowserSettings::passwordUseDashes()
{
    return config()->get("generator/Dashes", PasswordGenerator::DefaultDashes).toBool();
}

void BrowserSettings::setPasswordUseDashes(bool useDashes)
{
    config()->set("generator/Dashes", useDashes);
}

bool BrowserSettings::passwordUseMath()
{
    return config()->get("generator/Math", PasswordGenerator::DefaultMath).toBool();
}

void BrowserSettings::setPasswordUseMath(bool useMath)
{
    config()->set("generator/Math", useMath);
}

bool BrowserSettings::passwordUseLogograms()
{
    return config()->get("generator/Logograms", PasswordGenerator::DefaultLogograms).toBool();
}

void BrowserSettings::setPasswordUseLogograms(bool useLogograms)
{
    config()->set("generator/Logograms", useLogograms);
}

bool BrowserSettings::passwordUseEASCII()
{
    return config()->get("generator/EASCII", PasswordGenerator::DefaultEASCII).toBool();
}

void BrowserSettings::setPasswordUseEASCII(bool useEASCII)
{
    config()->set("generator/EASCII", useEASCII);
}

bool BrowserSettings::advancedMode()
{
    return config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool();
}

void BrowserSettings::setAdvancedMode(bool advancedMode)
{
    config()->set("generator/AdvancedMode", advancedMode);
}

QString BrowserSettings::passwordExcludedChars()
{
    return config()->get("generator/ExcludedChars", PasswordGenerator::DefaultExcludedChars).toString();
}

void BrowserSettings::setPasswordExcludedChars(QString chars)
{
    config()->set("generator/ExcludedChars", chars);
}

int BrowserSettings::passPhraseWordCount()
{
    return config()->get("generator/WordCount", PassphraseGenerator::DefaultWordCount).toInt();
}

void BrowserSettings::setPassPhraseWordCount(int wordCount)
{
    config()->set("generator/WordCount", wordCount);
}

QString BrowserSettings::passPhraseWordSeparator()
{
    return config()->get("generator/WordSeparator", PassphraseGenerator::DefaultSeparator).toString();
}

void BrowserSettings::setPassPhraseWordSeparator(QString separator)
{
    config()->set("generator/WordSeparator", separator);
}

int BrowserSettings::generatorType()
{
    return config()->get("generator/Type", 0).toInt();
}

void BrowserSettings::setGeneratorType(int type)
{
    config()->set("generator/Type", type);
}

bool BrowserSettings::passwordEveryGroup()
{
    return config()->get("generator/EnsureEvery", PasswordGenerator::DefaultFromEveryGroup).toBool();
}

void BrowserSettings::setPasswordEveryGroup(bool everyGroup)
{
    config()->get("generator/EnsureEvery", everyGroup);
}

bool BrowserSettings::passwordExcludeAlike()
{
    return config()->get("generator/ExcludeAlike", PasswordGenerator::DefaultLookAlike).toBool();
}

void BrowserSettings::setPasswordExcludeAlike(bool excludeAlike)
{
    config()->set("generator/ExcludeAlike", excludeAlike);
}

int BrowserSettings::passwordLength()
{
    return config()->get("generator/Length", PasswordGenerator::DefaultLength).toInt();
}

void BrowserSettings::setPasswordLength(int length)
{
    config()->set("generator/Length", length);
    m_passwordGenerator.setLength(length);
}

PasswordGenerator::CharClasses BrowserSettings::passwordCharClasses()
{
    PasswordGenerator::CharClasses classes;
    if (passwordUseLowercase()) {
        classes |= PasswordGenerator::LowerLetters;
    }
    if (passwordUseUppercase()) {
        classes |= PasswordGenerator::UpperLetters;
    }
    if (passwordUseNumbers()) {
        classes |= PasswordGenerator::Numbers;
    }
    if (passwordUseSpecial()) {
        classes |= PasswordGenerator::SpecialCharacters;
    }
    if (passwordUseBraces()) {
        classes |= PasswordGenerator::Braces;
    }
    if (passwordUsePunctuation()) {
        classes |= PasswordGenerator::Punctuation;
    }
    if (passwordUseQuotes()) {
        classes |= PasswordGenerator::Quotes;
    }
    if (passwordUseDashes()) {
        classes |= PasswordGenerator::Dashes;
    }
    if (passwordUseMath()) {
        classes |= PasswordGenerator::Math;
    }
    if (passwordUseLogograms()) {
        classes |= PasswordGenerator::Logograms;
    }
    if (passwordUseEASCII()) {
        classes |= PasswordGenerator::EASCII;
    }
    return classes;
}

PasswordGenerator::GeneratorFlags BrowserSettings::passwordGeneratorFlags()
{
    PasswordGenerator::GeneratorFlags flags;
    if (passwordExcludeAlike()) {
        flags |= PasswordGenerator::ExcludeLookAlike;
    }
    if (passwordEveryGroup()) {
        flags |= PasswordGenerator::CharFromEveryGroup;
    }
    return flags;
}

QString BrowserSettings::generatePassword()
{
    if (generatorType() == 0) {
        m_passwordGenerator.setLength(passwordLength());
        m_passwordGenerator.setCharClasses(passwordCharClasses());
        m_passwordGenerator.setFlags(passwordGeneratorFlags());
        return m_passwordGenerator.generatePassword();
    } else {
        m_passPhraseGenerator.setDefaultWordList();
        m_passPhraseGenerator.setWordCount(passPhraseWordCount());
        m_passPhraseGenerator.setWordSeparator(passPhraseWordSeparator());
        return m_passPhraseGenerator.generatePassphrase();
    }
}

int BrowserSettings::getbits()
{
    return m_passwordGenerator.getbits();
}

void BrowserSettings::updateBinaryPaths(QString customProxyLocation)
{
    bool isProxy = supportBrowserProxy();
    m_hostInstaller.updateBinaryPaths(isProxy, customProxyLocation);
}
