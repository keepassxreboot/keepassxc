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

#include <QDir>

FileDialog* FileDialog::m_instance(nullptr);

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
        const auto& workingDir = dir.isEmpty() ? config()->get(Config::LastDir).toString() : dir;
        const auto result = QDir::toNativeSeparators(
            QFileDialog::getOpenFileName(parent, caption, workingDir, filter, selectedFilter, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        saveLastDir(result);
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
        const auto& workingDir = dir.isEmpty() ? config()->get(Config::LastDir).toString() : dir;
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
        if (!results.isEmpty()) {
            saveLastDir(results[0]);
        }
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
        const auto& workingDir = dir.isEmpty() ? config()->get(Config::LastDir).toString() : dir;
        const auto result = QDir::toNativeSeparators(
            QFileDialog::getSaveFileName(parent, caption, workingDir, filter, selectedFilter, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        saveLastDir(result);
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
        const auto& workingDir = dir.isEmpty() ? config()->get(Config::LastDir).toString() : dir;
        const auto result =
            QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, caption, workingDir, options));

#ifdef Q_OS_MACOS
        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
#endif
        saveLastDir(result);
        return result;
    }
}

void FileDialog::setNextFileName(const QString& fileName)
{
    m_nextFileName = fileName;
}

void FileDialog::setNextFileNames(const QStringList& fileNames)
{
    m_nextFileNames = fileNames;
}

void FileDialog::setNextDirName(const QString& dirName)
{
    m_nextDirName = dirName;
}

void FileDialog::setNextForgetDialog()
{
    m_forgetLastDir = true;
}

FileDialog::FileDialog()
{
}

void FileDialog::saveLastDir(const QString& dir)
{
    if (!dir.isEmpty() && !m_forgetLastDir) {
        config()->set(Config::LastDir, QFileInfo(dir).absolutePath());
    }

    m_forgetLastDir = false;
}

FileDialog* FileDialog::instance()
{
    if (!m_instance) {
        m_instance = new FileDialog();
    }

    return m_instance;
}
