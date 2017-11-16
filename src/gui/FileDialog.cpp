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

FileDialog* FileDialog::m_instance(nullptr);

QString FileDialog::getOpenFileName(QWidget* parent, const QString& caption, QString dir,
                                    const QString& filter, QString* selectedFilter,
                                    QFileDialog::Options options)
{
    if (!m_nextFileName.isEmpty()) {
        QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    }
    else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QString result = QFileDialog::getOpenFileName(parent, caption, dir, filter,
                                                      selectedFilter, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }
        if (!result.isEmpty() && m_nextSaveLastDir) {
            config()->set("LastDir", QFileInfo(result).absolutePath());
        }

        m_nextSaveLastDir = true;
        return result;
    }
}

QStringList FileDialog::getOpenFileNames(QWidget *parent, const QString &caption, QString dir,
                                     const QString &filter, QString *selectedFilter,
                                     QFileDialog::Options options)
{
    if (!m_nextFileNames.isEmpty()) {
        QStringList results = m_nextFileNames;
        m_nextFileNames.clear();
        return results;
    }
    else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QStringList results = QFileDialog::getOpenFileNames(parent, caption, dir, filter,
                                                           selectedFilter, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        if (!results.isEmpty() && m_nextSaveLastDir) {
            config()->set("LastDir", QFileInfo(results[0]).absolutePath());
        }

        m_nextSaveLastDir = true;
        return results;
    }
}

QString FileDialog::getSaveFileName(QWidget* parent, const QString& caption, QString dir,
                                    const QString& filter, QString* selectedFilter,
                                    QFileDialog::Options options, const QString& defaultExtension)
{
    if (!m_nextFileName.isEmpty()) {
        QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    }
    else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QString result;
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        Q_UNUSED(defaultExtension);
        // the native dialogs on these platforms already append the file extension
        result = QFileDialog::getSaveFileName(parent, caption, dir, filter,
                                              selectedFilter, options);
#else
        QFileDialog dialog(parent, caption, dir, filter);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        if (selectedFilter) {
            dialog.selectNameFilter(*selectedFilter);
        }
        dialog.setOptions(options);
        dialog.setDefaultSuffix(defaultExtension);

        QStringList results;
        if (dialog.exec()) {
            results = dialog.selectedFiles();
            if (!results.isEmpty()) {
                result = results[0];
            }
        }
#endif

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        if (!result.isEmpty() && m_nextSaveLastDir) {
            config()->set("LastDir", QFileInfo(result).absolutePath());
        }

        m_nextSaveLastDir = true;
        return result;
    }
}

QString FileDialog::getExistingDirectory(QWidget *parent, const QString &caption, QString dir,
                                         QFileDialog::Options options)
{
    if (!m_nextDirName.isEmpty()) {
        QString result = m_nextDirName;
        m_nextDirName.clear();
        return result;
    }
    else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        dir = QFileDialog::getExistingDirectory(parent, caption, dir, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        if (!dir.isEmpty() && m_nextSaveLastDir) {
            config()->set("LastDir", QFileInfo(dir).absolutePath());
        }

        m_nextSaveLastDir = true;
        return dir;
    }
}

void FileDialog::setNextFileName(const QString& fileName)
{
    m_nextFileName = fileName;
}

void FileDialog::setNextFileNames(const QStringList &fileNames)
{
    m_nextFileNames = fileNames;
}

void FileDialog::setNextDirName(const QString &dirName)
{
    m_nextDirName = dirName;
}

void FileDialog::setNextForgetDialog()
{
    m_nextSaveLastDir = false;
}

FileDialog::FileDialog()
{
}

FileDialog* FileDialog::instance()
{
    if (!m_instance) {
        m_instance = new FileDialog();
    }

    return m_instance;
}
