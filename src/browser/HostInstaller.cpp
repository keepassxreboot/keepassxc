/*
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

#include "HostInstaller.h"
#include "config-keepassx.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QStandardPaths>

HostInstaller::HostInstaller()
    : HOST_NAME("org.keepassxc.keepassxc_browser")
    , ALLOWED_EXTENSIONS(QStringList() << "keepassxc-browser@keepassxc.org")
    , ALLOWED_ORIGINS(QStringList() << "chrome-extension://iopaggbpplllidnfmcghoonnokmjoicf/"
                                    << "chrome-extension://oboonakemofpalcgghocfoadofidjkkk/")
#if defined(Q_OS_MACOS)
    , TARGET_DIR_CHROME("/Library/Application Support/Google/Chrome/NativeMessagingHosts")
    , TARGET_DIR_CHROMIUM("/Library/Application Support/Chromium/NativeMessagingHosts")
    , TARGET_DIR_FIREFOX("/Library/Application Support/Mozilla/NativeMessagingHosts")
    , TARGET_DIR_VIVALDI("/Library/Application Support/Vivaldi/NativeMessagingHosts")
    , TARGET_DIR_TOR_BROWSER("/Library/Application Support/TorBrowser-Data/Browser/Mozilla/NativeMessagingHosts")
    , TARGET_DIR_BRAVE("/Library/Application Support/BraveSoftware/Brave-Browser/NativeMessagingHosts")
#elif defined(Q_OS_WIN)
    // clang-format off
    , TARGET_DIR_CHROME("HKEY_CURRENT_USER\\Software\\Google\\Chrome\\NativeMessagingHosts\\org.keepassxc.keepassxc_browser")
    , TARGET_DIR_CHROMIUM("HKEY_CURRENT_USER\\Software\\Chromium\\NativeMessagingHosts\\org.keepassxc.keepassxc_browser")
    // clang-format on
    , TARGET_DIR_FIREFOX("HKEY_CURRENT_USER\\Software\\Mozilla\\NativeMessagingHosts\\org.keepassxc.keepassxc_browser")
    , TARGET_DIR_VIVALDI(TARGET_DIR_CHROME)
    , TARGET_DIR_TOR_BROWSER(TARGET_DIR_FIREFOX)
    , TARGET_DIR_BRAVE(TARGET_DIR_CHROME)
#else
    , TARGET_DIR_CHROME("/.config/google-chrome/NativeMessagingHosts")
    , TARGET_DIR_CHROMIUM("/.config/chromium/NativeMessagingHosts")
    , TARGET_DIR_FIREFOX("/.mozilla/native-messaging-hosts")
    , TARGET_DIR_VIVALDI("/.config/vivaldi/NativeMessagingHosts")
    , TARGET_DIR_TOR_BROWSER("/.tor-browser/app/Browser/TorBrowser/Data/Browser/.mozilla/native-messaging-hosts")
    , TARGET_DIR_BRAVE("/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts")
#endif
{
}

/**
 * Checks if the selected browser has native messaging host properly installed
 *
 * @param browser Selected browser
 * @return bool Script is installed correctly
 */
bool HostInstaller::checkIfInstalled(SupportedBrowsers browser)
{
    QString fileName = getPath(browser);
#ifdef Q_OS_WIN
    QSettings settings(getTargetPath(browser), QSettings::NativeFormat);
    return registryEntryFound(settings);
#else
    return QFile::exists(fileName);
#endif
}

/**
 * Checks if keepassxc-proxy location is found
 *
 * @param proxy Is keepassxc-proxy enabled
 * @param location Custom proxy location
 * @param path The path is set here and returned to the caller
 * @return bool
 */
bool HostInstaller::checkIfProxyExists(const bool& proxy, const QString& location, QString& path) const
{
    QString fileName = getProxyPath(proxy, location);
    path = fileName;
    return QFile::exists(fileName);
}

/**
 * Installs native messaging JSON script for the selected browser
 *
 * @param browser Selected browser
 * @param enabled Is browser integration enabled
 * @param proxy Is keepassxc-proxy enabled
 * @param location Custom proxy location
 */
void HostInstaller::installBrowser(SupportedBrowsers browser,
                                   const bool& enabled,
                                   const bool& proxy,
                                   const QString& location)
{
    if (enabled) {
#ifdef Q_OS_WIN
        // Create a registry key
        QSettings settings(getTargetPath(browser), QSettings::NativeFormat);
        settings.setValue("Default", getPath(browser));
#endif
        // Always create the script file
        QJsonObject script = constructFile(browser, proxy, location);
        if (!saveFile(browser, script)) {
            QMessageBox::critical(nullptr,
                                  tr("KeePassXC: Cannot save file!"),
                                  tr("Cannot save the native messaging script file."),
                                  QMessageBox::Ok);
        }
    } else {
        // Remove the script file
        QString fileName = getPath(browser);
        QFile::remove(fileName);
#ifdef Q_OS_WIN
        // Remove the registry entry
        QSettings settings(getTargetPath(browser), QSettings::NativeFormat);
        settings.remove("Default");
#endif
    }
}

/**
 * Updates the paths to native messaging host for each browser that has been enabled
 *
 * @param proxy Is keepassxc-proxy enabled
 * @param location Custom proxy location
 */
void HostInstaller::updateBinaryPaths(const bool& proxy, const QString& location)
{
    // Where 6 is the number of entries in the SupportedBrowsers enum declared in HostInstaller.h
    for (int i = 0; i < 6; ++i) {
        if (checkIfInstalled(static_cast<SupportedBrowsers>(i))) {
            installBrowser(static_cast<SupportedBrowsers>(i), true, proxy, location);
        }
    }
}

/**
 * Returns the target path for each browser. Windows uses a registry path instead of a file path
 *
 * @param browser Selected browser
 * @return QString Current target path for the selected browser
 */
QString HostInstaller::getTargetPath(SupportedBrowsers browser) const
{
    switch (browser) {
    case SupportedBrowsers::CHROME:
        return TARGET_DIR_CHROME;
    case SupportedBrowsers::CHROMIUM:
        return TARGET_DIR_CHROMIUM;
    case SupportedBrowsers::FIREFOX:
        return TARGET_DIR_FIREFOX;
    case SupportedBrowsers::VIVALDI:
        return TARGET_DIR_VIVALDI;
    case SupportedBrowsers::TOR_BROWSER:
        return TARGET_DIR_TOR_BROWSER;
    case SupportedBrowsers::BRAVE:
        return TARGET_DIR_BRAVE;
    default:
        return QString();
    }
}

/**
 * Returns the browser name
 * Needed for Windows to separate Chromium- or Firefox-based scripts
 *
 * @param browser Selected browser
 * @return QString Name of the selected browser
 */
QString HostInstaller::getBrowserName(SupportedBrowsers browser) const
{
    switch (browser) {
    case SupportedBrowsers::CHROME:
        return "chrome";
    case SupportedBrowsers::CHROMIUM:
        return "chromium";
    case SupportedBrowsers::FIREFOX:
        return "firefox";
    case SupportedBrowsers::VIVALDI:
        return "vivaldi";
    case SupportedBrowsers::TOR_BROWSER:
        return "tor-browser";
    case SupportedBrowsers::BRAVE:
        return "brave";
    default:
        return QString();
    }
}

/**
 * Returns the path of native messaging JSON script for the selected browser
 *
 * @param browser Selected browser
 * @return QString JSON script path for the selected browser
 */
QString HostInstaller::getPath(SupportedBrowsers browser) const
{
#ifdef Q_OS_WIN
    // If portable settings file exists save the JSON scripts to application folder
    QString userPath;
    QString portablePath = QCoreApplication::applicationDirPath() + "/keepassxc.ini";
    if (QFile::exists(portablePath)) {
        userPath = QCoreApplication::applicationDirPath();
    } else {
        userPath = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    }

    QString winPath = QString("%1/%2_%3.json").arg(userPath, HOST_NAME, getBrowserName(browser));
    winPath.replace("/", "\\");
    return winPath;
#else
    QString path = getTargetPath(browser);
    return QString("%1%2/%3.json").arg(QDir::homePath(), path, HOST_NAME);
#endif
}

/**
 * Gets the installation directory for JSON script file (application install path)
 *
 * @param browser Selected browser
 * @return QString Install path
 */
QString HostInstaller::getInstallDir(SupportedBrowsers browser) const
{
    QString path = getTargetPath(browser);
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath();
#else
    return QString("%1%2").arg(QDir::homePath(), path);
#endif
}

/**
 * Gets the path to keepassxc-proxy binary
 *
 * @param proxy Is keepassxc-proxy used with KeePassXC
 * @param location Custom proxy path
 * @return path Path to keepassxc-proxy
 */
QString HostInstaller::getProxyPath(const bool& proxy, const QString& location) const
{
    QString path;
#ifdef KEEPASSXC_DIST_APPIMAGE
    if (proxy && !location.isEmpty()) {
        path = location;
    } else {
        path = QProcessEnvironment::systemEnvironment().value("APPIMAGE");
    }
#else
    if (proxy) {
        if (!location.isEmpty()) {
            path = location;
        } else {
            path = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
            path.append("/keepassxc-proxy");
#ifdef Q_OS_WIN
            path.append(".exe");
#endif
        }
    } else {
        path = QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath();
    }
#ifdef Q_OS_WIN
    path.replace("/", "\\");
#endif

#endif // #ifdef KEEPASSXC_DIST_APPIMAGE
    return path;
}

/**
 * Constructs the JSON script file used with native messaging
 *
 * @param browser Browser (Chromium- and Firefox-based browsers need a different parameters for the script)
 * @param proxy Is keepassxc-proxy used with KeePassXC
 * @param location Custom proxy location
 * @return script The JSON script file
 */
QJsonObject HostInstaller::constructFile(SupportedBrowsers browser, const bool& proxy, const QString& location)
{
    QString path = getProxyPath(proxy, location);

    QJsonObject script;
    script["name"] = HOST_NAME;
    script["description"] = QString("KeePassXC integration with native messaging support");
    script["path"] = path;
    script["type"] = QString("stdio");

    QJsonArray arr;
    if (browser == SupportedBrowsers::FIREFOX || browser == SupportedBrowsers::TOR_BROWSER) {
        for (const QString& extension : ALLOWED_EXTENSIONS) {
            arr.append(extension);
        }
        script["allowed_extensions"] = arr;
    } else {
        for (const QString& origin : ALLOWED_ORIGINS) {
            arr.append(origin);
        }
        script["allowed_origins"] = arr;
    }

    return script;
}

/**
 * Checks if a registry setting is found with default value
 *
 * @param settings Registry path
 * @return bool Is the registry value found
 */
bool HostInstaller::registryEntryFound(const QSettings& settings)
{
    return !settings.value("Default").isNull();
}

/**
 * Saves a JSON script file
 *
 * @param browser Selected browser
 * @param script JSON native messaging script object
 * @return bool Write succeeds
 */
bool HostInstaller::saveFile(SupportedBrowsers browser, const QJsonObject& script)
{
    QString path = getPath(browser);
    QString installDir = getInstallDir(browser);
    QDir dir(installDir);
    if (!dir.exists()) {
        QDir().mkpath(installDir);
    }

    QFile scriptFile(path);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(script);
    return scriptFile.write(doc.toJson()) >= 0;
}
