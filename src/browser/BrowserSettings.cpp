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
#include "core/PasswordHealth.h"

BrowserSettings* BrowserSettings::m_instance(nullptr);

BrowserSettings* BrowserSettings::instance()
{
    if (!m_instance) {
        m_instance = new BrowserSettings();
    }

    return m_instance;
}

bool BrowserSettings::isEnabled()
{
    return config()->get(Config::Browser_Enabled).toBool();
}

void BrowserSettings::setEnabled(bool enabled)
{
    config()->set(Config::Browser_Enabled, enabled);
}

bool BrowserSettings::showNotification()
{
    return config()->get(Config::Browser_ShowNotification).toBool();
}

void BrowserSettings::setShowNotification(bool showNotification)
{
    config()->set(Config::Browser_ShowNotification, showNotification);
}

bool BrowserSettings::bestMatchOnly()
{
    return config()->get(Config::Browser_BestMatchOnly).toBool();
}

void BrowserSettings::setBestMatchOnly(bool bestMatchOnly)
{
    config()->set(Config::Browser_BestMatchOnly, bestMatchOnly);
}

bool BrowserSettings::unlockDatabase()
{
    return config()->get(Config::Browser_UnlockDatabase).toBool();
}

void BrowserSettings::setUnlockDatabase(bool unlockDatabase)
{
    config()->set(Config::Browser_UnlockDatabase, unlockDatabase);
}

bool BrowserSettings::matchUrlScheme()
{
    return config()->get(Config::Browser_MatchUrlScheme).toBool();
}

void BrowserSettings::setMatchUrlScheme(bool matchUrlScheme)
{
    config()->set(Config::Browser_MatchUrlScheme, matchUrlScheme);
}

bool BrowserSettings::sortByUsername()
{
    return config()->get(Config::Browser_SortByUsername).toBool();
}

void BrowserSettings::setSortByUsername(bool sortByUsername)
{
    config()->set(Config::Browser_SortByUsername, sortByUsername);
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
    return config()->get(Config::Browser_AlwaysAllowAccess).toBool();
}

void BrowserSettings::setAlwaysAllowAccess(bool alwaysAllowAccess)
{
    config()->set(Config::Browser_AlwaysAllowAccess, alwaysAllowAccess);
}

bool BrowserSettings::alwaysAllowUpdate()
{
    return config()->get(Config::Browser_AlwaysAllowUpdate).toBool();
}

void BrowserSettings::setAlwaysAllowUpdate(bool alwaysAllowUpdate)
{
    config()->set(Config::Browser_AlwaysAllowUpdate, alwaysAllowUpdate);
}

bool BrowserSettings::httpAuthPermission()
{
    return config()->get(Config::Browser_HttpAuthPermission).toBool();
}

void BrowserSettings::setHttpAuthPermission(bool httpAuthPermission)
{
    config()->set(Config::Browser_HttpAuthPermission, httpAuthPermission);
}

bool BrowserSettings::searchInAllDatabases()
{
    return config()->get(Config::Browser_SearchInAllDatabases).toBool();
}

void BrowserSettings::setSearchInAllDatabases(bool searchInAllDatabases)
{
    config()->set(Config::Browser_SearchInAllDatabases, searchInAllDatabases);
}

bool BrowserSettings::supportKphFields()
{
    return config()->get(Config::Browser_SupportKphFields).toBool();
}

void BrowserSettings::setSupportKphFields(bool supportKphFields)
{
    config()->set(Config::Browser_SupportKphFields, supportKphFields);
}

bool BrowserSettings::noMigrationPrompt()
{
    return config()->get(Config::Browser_NoMigrationPrompt).toBool();
}

void BrowserSettings::setNoMigrationPrompt(bool prompt)
{
    config()->set(Config::Browser_NoMigrationPrompt, prompt);
}

bool BrowserSettings::supportBrowserProxy()
{
    return config()->get(Config::Browser_SupportBrowserProxy).toBool();
}

void BrowserSettings::setSupportBrowserProxy(bool enabled)
{
    config()->set(Config::Browser_SupportBrowserProxy, enabled);
}

bool BrowserSettings::useCustomProxy()
{
    return config()->get(Config::Browser_UseCustomProxy).toBool();
}

void BrowserSettings::setUseCustomProxy(bool enabled)
{
    config()->set(Config::Browser_UseCustomProxy, enabled);
}

QString BrowserSettings::customProxyLocation()
{
    if (!useCustomProxy()) {
        return QString();
    }
    return config()->get(Config::Browser_CustomProxyLocation).toString();
}

void BrowserSettings::setCustomProxyLocation(const QString& location)
{
    config()->set(Config::Browser_CustomProxyLocation, location);
}

bool BrowserSettings::updateBinaryPath()
{
    return config()->get(Config::Browser_UpdateBinaryPath).toBool();
}

void BrowserSettings::setUpdateBinaryPath(bool enabled)
{
    config()->set(Config::Browser_UpdateBinaryPath, enabled);
}

bool BrowserSettings::allowExpiredCredentials()
{
    return config()->get(Config::Browser_AllowExpiredCredentials).toBool();
}

void BrowserSettings::setAllowExpiredCredentials(bool enabled)
{
    config()->set(Config::Browser_AllowExpiredCredentials, enabled);
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

bool BrowserSettings::braveSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::BRAVE);
}

void BrowserSettings::setBraveSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::BRAVE, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::torBrowserSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::TOR_BROWSER);
}

void BrowserSettings::setTorBrowserSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::TOR_BROWSER, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::edgeSupport()
{
    return m_hostInstaller.checkIfInstalled(HostInstaller::SupportedBrowsers::EDGE);
}

void BrowserSettings::setEdgeSupport(bool enabled)
{
    m_hostInstaller.installBrowser(
        HostInstaller::SupportedBrowsers::EDGE, enabled, supportBrowserProxy(), customProxyLocation());
}

bool BrowserSettings::passwordUseNumbers()
{
    return config()->get(Config::PasswordGenerator_Numbers).toBool();
}

void BrowserSettings::setPasswordUseNumbers(bool useNumbers)
{
    config()->set(Config::PasswordGenerator_Numbers, useNumbers);
}

bool BrowserSettings::passwordUseLowercase()
{
    return config()->get(Config::PasswordGenerator_LowerCase).toBool();
}

void BrowserSettings::setPasswordUseLowercase(bool useLowercase)
{
    config()->set(Config::PasswordGenerator_LowerCase, useLowercase);
}

bool BrowserSettings::passwordUseUppercase()
{
    return config()->get(Config::PasswordGenerator_UpperCase).toBool();
}

void BrowserSettings::setPasswordUseUppercase(bool useUppercase)
{
    config()->set(Config::PasswordGenerator_UpperCase, useUppercase);
}

bool BrowserSettings::passwordUseSpecial()
{
    return config()->get(Config::PasswordGenerator_SpecialChars).toBool();
}

void BrowserSettings::setPasswordUseSpecial(bool useSpecial)
{
    config()->set(Config::PasswordGenerator_SpecialChars, useSpecial);
}

bool BrowserSettings::passwordUseBraces()
{
    return config()->get(Config::PasswordGenerator_Braces).toBool();
}

void BrowserSettings::setPasswordUseBraces(bool useBraces)
{
    config()->set(Config::PasswordGenerator_Braces, useBraces);
}

bool BrowserSettings::passwordUsePunctuation()
{
    return config()->get(Config::PasswordGenerator_Punctuation).toBool();
}

void BrowserSettings::setPasswordUsePunctuation(bool usePunctuation)
{
    config()->set(Config::PasswordGenerator_Punctuation, usePunctuation);
}

bool BrowserSettings::passwordUseQuotes()
{
    return config()->get(Config::PasswordGenerator_Quotes).toBool();
}

void BrowserSettings::setPasswordUseQuotes(bool useQuotes)
{
    config()->set(Config::PasswordGenerator_Quotes, useQuotes);
}

bool BrowserSettings::passwordUseDashes()
{
    return config()->get(Config::PasswordGenerator_Dashes).toBool();
}

void BrowserSettings::setPasswordUseDashes(bool useDashes)
{
    config()->set(Config::PasswordGenerator_Dashes, useDashes);
}

bool BrowserSettings::passwordUseMath()
{
    return config()->get(Config::PasswordGenerator_Math).toBool();
}

void BrowserSettings::setPasswordUseMath(bool useMath)
{
    config()->set(Config::PasswordGenerator_Math, useMath);
}

bool BrowserSettings::passwordUseLogograms()
{
    return config()->get(Config::PasswordGenerator_Logograms).toBool();
}

void BrowserSettings::setPasswordUseLogograms(bool useLogograms)
{
    config()->set(Config::PasswordGenerator_Logograms, useLogograms);
}

bool BrowserSettings::passwordUseEASCII()
{
    return config()->get(Config::PasswordGenerator_EASCII).toBool();
}

void BrowserSettings::setPasswordUseEASCII(bool useEASCII)
{
    config()->set(Config::PasswordGenerator_EASCII, useEASCII);
}

bool BrowserSettings::advancedMode()
{
    return config()->get(Config::PasswordGenerator_AdvancedMode).toBool();
}

void BrowserSettings::setAdvancedMode(bool advancedMode)
{
    config()->set(Config::PasswordGenerator_AdvancedMode, advancedMode);
}

QString BrowserSettings::passwordAdditionalChars()
{
    return config()->get(Config::PasswordGenerator_AdditionalChars).toString();
}

void BrowserSettings::setPasswordAdditionalChars(const QString& chars)
{
    config()->set(Config::PasswordGenerator_AdditionalChars, chars);
}

QString BrowserSettings::passwordExcludedChars()
{
    return config()->get(Config::PasswordGenerator_ExcludedChars).toString();
}

void BrowserSettings::setPasswordExcludedChars(const QString& chars)
{
    config()->set(Config::PasswordGenerator_ExcludedChars, chars);
}

int BrowserSettings::passPhraseWordCount()
{
    return config()->get(Config::PasswordGenerator_WordCount).toInt();
}

void BrowserSettings::setPassPhraseWordCount(int wordCount)
{
    config()->set(Config::PasswordGenerator_WordCount, wordCount);
}

QString BrowserSettings::passPhraseWordSeparator()
{
    return config()->get(Config::PasswordGenerator_WordSeparator).toString();
}

void BrowserSettings::setPassPhraseWordSeparator(const QString& separator)
{
    config()->set(Config::PasswordGenerator_WordSeparator, separator);
}

int BrowserSettings::generatorType()
{
    return config()->get(Config::PasswordGenerator_Type).toInt();
}

void BrowserSettings::setGeneratorType(int type)
{
    config()->set(Config::PasswordGenerator_Type, type);
}

bool BrowserSettings::passwordEveryGroup()
{
    return config()->get(Config::PasswordGenerator_EnsureEvery).toBool();
}

void BrowserSettings::setPasswordEveryGroup(bool everyGroup)
{
    config()->set(Config::PasswordGenerator_EnsureEvery, everyGroup);
}

bool BrowserSettings::passwordExcludeAlike()
{
    return config()->get(Config::PasswordGenerator_ExcludeAlike).toBool();
}

void BrowserSettings::setPasswordExcludeAlike(bool excludeAlike)
{
    config()->set(Config::PasswordGenerator_ExcludeAlike, excludeAlike);
}

int BrowserSettings::passwordLength()
{
    return config()->get(Config::PasswordGenerator_Length).toInt();
}

void BrowserSettings::setPasswordLength(int length)
{
    config()->set(Config::PasswordGenerator_Length, length);
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

QJsonObject BrowserSettings::generatePassword()
{
    QJsonObject password;
    if (generatorType() == 0) {
        m_passwordGenerator.setLength(passwordLength());
        m_passwordGenerator.setCharClasses(passwordCharClasses());
        m_passwordGenerator.setFlags(passwordGeneratorFlags());
        const QString pw = m_passwordGenerator.generatePassword();
        password["entropy"] = PasswordHealth(pw).entropy();
        password["password"] = pw;
    } else {
        m_passPhraseGenerator.setWordCount(passPhraseWordCount());
        m_passPhraseGenerator.setWordSeparator(passPhraseWordSeparator());
        password["entropy"] = m_passPhraseGenerator.estimateEntropy();
        password["password"] = m_passPhraseGenerator.generatePassphrase();
    }
    return password;
}

void BrowserSettings::updateBinaryPaths(const QString& customProxyLocation)
{
    bool isProxy = supportBrowserProxy();
    m_hostInstaller.updateBinaryPaths(isProxy, customProxyLocation);
}

bool BrowserSettings::checkIfProxyExists(QString& path)
{
    return m_hostInstaller.checkIfProxyExists(supportBrowserProxy(), customProxyLocation(), path);
}
