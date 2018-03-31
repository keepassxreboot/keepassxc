/*
 *  Copyright (C) 2016 Danny Su <contact@dannysu.com>
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

#ifndef KEEPASSX_TEMPORARYFILE_H
#define KEEPASSX_TEMPORARYFILE_H

#include <QFile>
#include <QObject>
#include <QTemporaryFile>

/**
 * QTemporaryFile::close() doesn't actually close the file according to
 * http://doc.qt.io/qt-5/qtemporaryfile.html: "For as long as the
 * QTemporaryFile object itself is not destroyed, the unique temporary file
 * will exist and be kept open internally by QTemporaryFile."
 *
 * This behavior causes issues when running tests on Windows. If the file is
 * not closed, the testSave test will fail due to Access Denied. The
 * auto-reload test also fails from Windows not triggering file change
 * notification because the file isn't actually closed by QTemporaryFile.
 *
 * This class isolates the Windows specific logic that uses QFile to really
 * close the test file when requested to.
 */
class TemporaryFile : public QObject
{
    Q_OBJECT

public:
#ifdef Q_OS_WIN
    ~TemporaryFile();
#endif

    bool open();
    void close();
    qint64 write(const char* data, qint64 maxSize);
    qint64 write(const QByteArray& byteArray);

    QString fileName() const;
    QString filePath() const;

private:
    QTemporaryFile m_tempFile;
#ifdef Q_OS_WIN
    QFile m_file;
    static const QString SUFFIX;
#endif
};

#endif // KEEPASSX_TEMPORARYFILE_H
