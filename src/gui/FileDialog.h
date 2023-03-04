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

#ifndef KEEPASSX_FILEDIALOG_H
#define KEEPASSX_FILEDIALOG_H

#include <QFileDialog>

class FileDialog
{
public:
    QString getOpenFileName(QWidget* parent = nullptr,
                            const QString& caption = QString(),
                            const QString& dir = QString(),
                            const QString& filter = QString(),
                            QString* selectedFilter = nullptr,
                            const QFileDialog::Options options = {});

    QStringList getOpenFileNames(QWidget* parent = nullptr,
                                 const QString& caption = QString(),
                                 const QString& dir = QString(),
                                 const QString& filter = QString(),
                                 QString* selectedFilter = nullptr,
                                 const QFileDialog::Options options = {});

    QString getSaveFileName(QWidget* parent = nullptr,
                            const QString& caption = QString(),
                            const QString& dir = QString(),
                            const QString& filter = QString(),
                            QString* selectedFilter = nullptr,
                            const QFileDialog::Options options = {});

    QString getExistingDirectory(QWidget* parent = nullptr,
                                 const QString& caption = QString(),
                                 const QString& dir = QString(),
                                 const QFileDialog::Options options = QFileDialog::ShowDirsOnly);

    /**
     * Bypasses the selection process and returns the provided filename/directory immediately (for testing)
     */
    void setNextFileName(const QString& fileName);
    void setNextDirectory(const QString& path);

    static void saveLastDir(const QString& role, const QString& path, bool sensitive = false);
    static QString getLastDir(const QString& role, const QString& defaultDir = QString());

    static FileDialog* instance();

private:
    FileDialog();
    QString m_nextFileName;
    QStringList m_nextFileNames;
    QString m_nextDirName;

    static FileDialog* m_instance;

    Q_DISABLE_COPY(FileDialog)
};

inline FileDialog* fileDialog()
{
    return FileDialog::instance();
}

#endif // KEEPASSX_FILEDIALOG_H
