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

QString FileDialog::getOpenFileName(QWidget* parent,
                                    const QString& caption,
                                    QString dir,
                                    const QString& filter,
                                    QString* selectedFilter,
                                    QFileDialog::Options options)
{
    if (!m_nextFileName.isEmpty()) {
        QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    } else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QString result = QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        saveLastDir(result);
        return result;
    }
}

QStringList FileDialog::getOpenFileNames(QWidget* parent,
                                         const QString& caption,
                                         QString dir,
                                         const QString& filter,
                                         QString* selectedFilter,
                                         QFileDialog::Options options)
{
    if (!m_nextFileNames.isEmpty()) {
        QStringList results = m_nextFileNames;
        m_nextFileNames.clear();
        return results;
    } else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QStringList results = QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        if (!results.isEmpty()) {
            saveLastDir(results[0]);
        }
        return results;
    }
}

QString FileDialog::getFileName(QWidget* parent,
                                const QString& caption,
                                QString dir,
                                const QString& filter,
                                QString* selectedFilter,
                                QFileDialog::Options options,
                                const QString& defaultExtension,
                                const QString& defaultName)
{
    if (!m_nextFileName.isEmpty()) {
        QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    } else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QString result;
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        Q_UNUSED(defaultName);
        Q_UNUSED(defaultExtension);
        // the native dialogs on these platforms already append the file extension
        result = QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
#else
        QFileDialog dialog(parent, caption, dir, filter);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        if (selectedFilter) {
            dialog.selectNameFilter(*selectedFilter);
        }
        if (!defaultName.isEmpty()) {
            dialog.selectFile(defaultName);
        }
        dialog.setOptions(options);
        if (!defaultExtension.isEmpty()) {
            dialog.setDefaultSuffix(defaultExtension);
        }
        dialog.setLabelText(QFileDialog::Accept, QFileDialog::tr("Select"));
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

        saveLastDir(result);
        return result;
    }
}

QString FileDialog::getSaveFileName(QWidget* parent,
                                    const QString& caption,
                                    QString dir,
                                    const QString& filter,
                                    QString* selectedFilter,
                                    QFileDialog::Options options,
                                    const QString& defaultExtension,
                                    const QString& defaultName)
{
    if (!m_nextFileName.isEmpty()) {
        QString result = m_nextFileName;
        m_nextFileName.clear();
        return result;
    } else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        QString result;
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
        Q_UNUSED(defaultName);
        Q_UNUSED(defaultExtension);
        // the native dialogs on these platforms already append the file extension
        result = QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
#else
        QFileDialog dialog(parent, caption, dir, filter);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        if (selectedFilter) {
            dialog.selectNameFilter(*selectedFilter);
        }
        if (!defaultName.isEmpty()) {
            dialog.selectFile(defaultName);
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

        saveLastDir(result);
        return result;
    }
}

QString
FileDialog::getExistingDirectory(QWidget* parent, const QString& caption, QString dir, QFileDialog::Options options)
{
    if (!m_nextDirName.isEmpty()) {
        QString result = m_nextDirName;
        m_nextDirName.clear();
        return result;
    } else {
        if (dir.isEmpty()) {
            dir = config()->get("LastDir").toString();
        }

        dir = QFileDialog::getExistingDirectory(parent, caption, dir, options);

        // on Mac OS X the focus is lost after closing the native dialog
        if (parent) {
            parent->activateWindow();
        }

        saveLastDir(dir);
        return dir;
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
        config()->set("LastDir", QFileInfo(dir).absolutePath());
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
