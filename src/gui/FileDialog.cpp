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

#include "FileDialog.h"

#include "core/Config.h"

#include <QProcessEnvironment>

FileDialog* FileDialog::m_instance(nullptr);

FileDialog::FileDialog() = default;

QString FileDialog::getOpenFileName(QWidget* parent,
                                    const QString& caption,
                                    const QString& dir,
                                    const QString& filter,
                                    QString* selectedFilter,
                                    const QFileDialog::Options options)
{
    if (!m_nextFileName.isEmpty()) {
        const QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    } else {
        const auto& workingDir = dir.isEmpty() ? getLastDir("default") : dir;
        const auto result = QDir::toNativeSeparators(
            QFileDialog::getOpenFileName(parent, caption, workingDir, filter, selectedFilter, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        return result;
    }
}

QStringList FileDialog::getOpenFileNames(QWidget* parent,
                                         const QString& caption,
                                         const QString& dir,
                                         const QString& filter,
                                         QString* selectedFilter,
                                         const QFileDialog::Options options)
{
    if (!m_nextFileNames.isEmpty()) {
        const QStringList results = m_nextFileNames;
        m_nextFileNames.clear();
        return results;
    } else {
        const auto& workingDir = dir.isEmpty() ? getLastDir("default") : dir;
        auto results = QFileDialog::getOpenFileNames(parent, caption, workingDir, filter, selectedFilter, options);

        for (auto& path : results) {
            path = QDir::toNativeSeparators(path);
        }

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        return results;
    }
}

QString FileDialog::getSaveFileName(QWidget* parent,
                                    const QString& caption,
                                    const QString& dir,
                                    const QString& filter,
                                    QString* selectedFilter,
                                    const QFileDialog::Options options)
{
    if (!m_nextFileName.isEmpty()) {
        const QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    } else {
        const auto& workingDir = dir.isEmpty() ? getLastDir("default") : dir;
        const auto result = QDir::toNativeSeparators(
            QFileDialog::getSaveFileName(parent, caption, workingDir, filter, selectedFilter, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        return result;
    }
}

QString FileDialog::getExistingDirectory(QWidget* parent,
                                         const QString& caption,
                                         const QString& dir,
                                         const QFileDialog::Options options)
{
    if (!m_nextDirName.isEmpty()) {
        const QString result = m_nextDirName;
        m_nextDirName.clear();
        return result;
    } else {
        const auto& workingDir = dir.isEmpty() ? getLastDir("default") : dir;
        const auto result =
            QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, caption, workingDir, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        return result;
    }
}

void FileDialog::setNextFileName(const QString& fileName)
{
    m_nextFileName = fileName;
}

void FileDialog::setNextDirectory(const QString& path)
{
    m_nextDirName = path;
}

void FileDialog::saveLastDir(const QString& role, const QString& path, bool sensitive)
{
    auto lastDirs = config()->get(Config::LastDir).toHash();
    if (sensitive && !config()->get(Config::RememberLastDatabases).toBool()) {
        // Ensure this role is forgotten
        lastDirs.remove(role);
    } else {
        auto pathInfo = QFileInfo(path);
        if (!pathInfo.exists()) {
            lastDirs.remove(role);
        } else {
            lastDirs.insert(role, pathInfo.absolutePath());
        }
    }
    config()->set(Config::LastDir, lastDirs);
}

QString FileDialog::getLastDir(const QString& role, const QString& defaultDir)
{
    auto lastDirs = config()->get(Config::LastDir).toHash();
    auto fallbackDir = defaultDir;

    if (fallbackDir.isEmpty()) {
        // Fallback to the environment variable, if it exists, otherwise use the home directory
        const auto& env = QProcessEnvironment::systemEnvironment();
        fallbackDir = env.value("KPXC_INITIAL_DIR", QDir::homePath());
    }

    return lastDirs.value(role, fallbackDir).toString();
}

FileDialog* FileDialog::instance()
{
    if (!m_instance) {
        m_instance = new FileDialog();
    }

    return m_instance;
}
