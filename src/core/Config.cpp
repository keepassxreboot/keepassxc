/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QTemporaryFile>
#include <QStandardPaths>

Config* Config::m_instance(nullptr);

QVariant Config::get(const QString& key)
{
    return m_settings->value(key, m_defaults.value(key));
}

QVariant Config::get(const QString& key, const QVariant& defaultValue)
{
    return m_settings->value(key, defaultValue);
}

bool Config::hasAccessError()
{
    return m_settings->status() & QSettings::AccessError;
}

QString Config::getFileName()
{
    return m_settings->fileName();
}

void Config::set(const QString& key, const QVariant& value)
{
    m_settings->setValue(key, value);
}

/**
 * Sync configuration with persistent storage.
 *
 * Usually, you don't need to call this method manually, but if you are writing
 * configurations after an emitted \link QCoreApplication::aboutToQuit() signal,
 * use it to guarantee your config values are persisted.
 */
void Config::sync()
{
    m_settings->sync();
}

Config::Config(const QString& fileName, QObject* parent)
    : QObject(parent)
{
    init(fileName);
}

Config::Config(QObject* parent)
    : QObject(parent)
{
    // Check if portable config is present. If not, find it in user's directory
    QString portablePath = QCoreApplication::applicationDirPath() + "/keepassxc.ini";
    if (QFile::exists(portablePath)) {
        init(portablePath);
    } else {
        QString userPath;
        QString homePath = QDir::homePath();

    #if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        // we can't use QStandardPaths on X11 as it uses XDG_DATA_HOME instead of XDG_CONFIG_HOME
        QByteArray env = qgetenv("XDG_CONFIG_HOME");
        if (env.isEmpty()) {
            userPath = homePath;
            userPath += "/.config";
        } else if (env[0] == '/') {
            userPath = QFile::decodeName(env);
        } else {
            userPath = homePath;
            userPath += '/';
            userPath += QFile::decodeName(env);
        }

        userPath += "/keepassxc/";
    #else
        userPath = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        // storageLocation() appends the application name ("/keepassxc") to the end
        userPath += "/";
    #endif

    #ifdef QT_DEBUG
        userPath += "keepassxc_debug.ini";
    #else
        userPath += "keepassxc.ini";
    #endif

        init(userPath);
    }
}

Config::~Config()
{
}

void Config::init(const QString& fileName)
{
    m_settings.reset(new QSettings(fileName, QSettings::IniFormat));
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Config::sync);

    m_defaults.insert("SingleInstance", true);
    m_defaults.insert("RememberLastDatabases", true);
    m_defaults.insert("RememberLastKeyFiles", true);
    m_defaults.insert("OpenPreviousDatabasesOnStartup", true);
    m_defaults.insert("AutoSaveAfterEveryChange", true);
    m_defaults.insert("AutoReloadOnChange", true);
    m_defaults.insert("AutoSaveOnExit", false);
    m_defaults.insert("BackupBeforeSave", false);
    m_defaults.insert("UseAtomicSaves", true);
    m_defaults.insert("SearchLimitGroup", false);
    m_defaults.insert("MinimizeOnCopy", false);
    m_defaults.insert("UseGroupIconOnEntryCreation", false);
    m_defaults.insert("AutoTypeEntryTitleMatch", true);
    m_defaults.insert("AutoTypeEntryURLMatch", true);
    m_defaults.insert("AutoTypeDelay", 25);
    m_defaults.insert("AutoTypeStartDelay", 500);
    m_defaults.insert("UseGroupIconOnEntryCreation", true);
    m_defaults.insert("IgnoreGroupExpansion", true);
    m_defaults.insert("security/clearclipboard", true);
    m_defaults.insert("security/clearclipboardtimeout", 10);
    m_defaults.insert("security/lockdatabaseidle", false);
    m_defaults.insert("security/lockdatabaseidlesec", 240);
    m_defaults.insert("security/lockdatabaseminimize", false);
    m_defaults.insert("security/lockdatabasescreenlock", true);
    m_defaults.insert("security/passwordsrepeat", false);
    m_defaults.insert("security/passwordscleartext", false);
    m_defaults.insert("security/hidepassworddetails", true);
    m_defaults.insert("security/autotypeask", true);
    m_defaults.insert("security/IconDownloadFallbackToGoogle", false);
    m_defaults.insert("GUI/Language", "system");
    m_defaults.insert("GUI/ShowTrayIcon", false);
    m_defaults.insert("GUI/DarkTrayIcon", false);
    m_defaults.insert("GUI/MinimizeToTray", false);
    m_defaults.insert("GUI/MinimizeOnClose", false);
    m_defaults.insert("GUI/HideUsernames", false);
    m_defaults.insert("GUI/HidePasswords", true);
}

Config* Config::instance()
{
    if (!m_instance) {
        m_instance = new Config(qApp);
    }

    return m_instance;
}

void Config::createConfigFromFile(const QString& file)
{
    Q_ASSERT(!m_instance);
    m_instance = new Config(file, qApp);
}

void Config::createTempFileInstance()
{
    Q_ASSERT(!m_instance);
    auto* tmpFile = new QTemporaryFile();
    bool openResult = tmpFile->open();
    Q_ASSERT(openResult);
    Q_UNUSED(openResult);
    m_instance = new Config(tmpFile->fileName(), qApp);
    tmpFile->setParent(m_instance);
}
