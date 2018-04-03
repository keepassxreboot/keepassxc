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

#include "TemporaryFile.h"

#include <QFileInfo>

#ifdef Q_OS_WIN
const QString TemporaryFile::SUFFIX = ".win";

TemporaryFile::~TemporaryFile()
{
    if (m_tempFile.autoRemove()) {
        m_file.remove();
    }
}
#endif

bool TemporaryFile::open()
{
#ifdef Q_OS_WIN
    // Still call QTemporaryFile::open() so that it figures out the temporary
    // file name to use. Assuming that by appending the SUFFIX to whatever
    // QTemporaryFile chooses is also an available file.
    bool tempFileOpened = m_tempFile.open();
    if (tempFileOpened) {
        m_file.setFileName(filePath());
        return m_file.open(QIODevice::WriteOnly);
    }
    return false;
#else
    return m_tempFile.open();
#endif
}

void TemporaryFile::close()
{
    m_tempFile.close();
#ifdef Q_OS_WIN
    m_file.close();
#endif
}

qint64 TemporaryFile::write(const char* data, qint64 maxSize)
{
#ifdef Q_OS_WIN
    return m_file.write(data, maxSize);
#else
    return m_tempFile.write(data, maxSize);
#endif
}

qint64 TemporaryFile::write(const QByteArray& byteArray)
{
#ifdef Q_OS_WIN
    return m_file.write(byteArray);
#else
    return m_tempFile.write(byteArray);
#endif
}

QString TemporaryFile::fileName() const
{
#ifdef Q_OS_WIN
    return QFileInfo(m_tempFile).fileName() + TemporaryFile::SUFFIX;
#else
    return QFileInfo(m_tempFile).fileName();
#endif
}

QString TemporaryFile::filePath() const
{
#ifdef Q_OS_WIN
    return m_tempFile.fileName() + TemporaryFile::SUFFIX;
#else
    return m_tempFile.fileName();
#endif
}
