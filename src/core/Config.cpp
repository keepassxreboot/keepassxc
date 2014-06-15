/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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
#include <QDesktopServices>
#include <QDir>
#include <QSettings>
#include <QTemporaryFile>

Config* Config::m_instance(Q_NULLPTR);

QVariant Config::get(const QString& key)
{
    return m_settings->value(key, m_defaults.value(key));
}

QVariant Config::get(const QString& key, const QVariant& defaultValue)
{
    return m_settings->value(key, defaultValue);
}

void Config::set(const QString& key, const QVariant& value)
{
    m_settings->setValue(key, value);
}

Config::Config(const QString& fileName, QObject* parent)
    : QObject(parent)
{
    init(fileName);
}

Config::Config(QObject* parent)
    : QObject(parent)
{
    QString userPath;
    QString homePath = QDir::homePath();

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // we can't use QDesktopServices on X11 as it uses XDG_DATA_HOME instead of XDG_CONFIG_HOME
    QByteArray env = qgetenv("XDG_CONFIG_HOME");
    if (env.isEmpty()) {
        userPath = homePath;
        userPath += "/.config";
    }
    else if (env[0] == '/') {
        userPath = QFile::decodeName(env);
    }
    else {
        userPath = homePath;
        userPath += '/';
        userPath += QFile::decodeName(env);
    }

    userPath += "/keepassx/";
#else
    userPath = QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    // storageLocation() appends the application name ("/keepassx") to the end
    userPath += "/";
#endif

    userPath += "keepassx2.ini";

    init(userPath);
}

Config::~Config()
{
}

void Config::init(const QString& fileName)
{
    m_settings.reset(new QSettings(fileName, QSettings::IniFormat));

    m_defaults.insert("RememberLastDatabases", true);
    m_defaults.insert("OpenPreviousDatabasesOnStartup", true);
    m_defaults.insert("ModifiedOnExpandedStateChanges", true);
    m_defaults.insert("AutoSaveAfterEveryChange", false);
    m_defaults.insert("AutoSaveOnExit", false);
    m_defaults.insert("ShowToolbar", true);
    m_defaults.insert("MinimizeOnCopy", false);
    m_defaults.insert("UseGroupIconOnEntryCreation", false);
    m_defaults.insert("AutoTypeEntryTitleMatch", true);
    m_defaults.insert("security/clearclipboard", true);
    m_defaults.insert("security/clearclipboardtimeout", 10);
    m_defaults.insert("security/lockdatabaseidle", false);
    m_defaults.insert("security/lockdatabaseidlesec", 10);
    m_defaults.insert("security/passwordscleartext", false);
    m_defaults.insert("security/autotypeask", true);
    m_defaults.insert("GUI/Language", "system");
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
    QTemporaryFile* tmpFile = new QTemporaryFile();
    bool openResult = tmpFile->open();
    Q_ASSERT(openResult);
    Q_UNUSED(openResult);
    m_instance = new Config(tmpFile->fileName(), qApp);
    tmpFile->setParent(m_instance);
}
