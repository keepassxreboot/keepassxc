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

#include "FileWatcher.h"

#include "core/Clock.h"
#include <QFileInfo>

#ifdef Q_OS_LINUX
#include <sys/vfs.h>
#endif

namespace
{
    const int FileChangeDelay = 500;
    const int TimerResolution = 100;
}

DelayingFileWatcher::DelayingFileWatcher(QObject* parent)
    : QObject(parent)
    , m_ignoreFileChange(false)
{
    connect(&m_fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged()));
    connect(&m_fileUnblockTimer, SIGNAL(timeout()), this, SLOT(observeFileChanges()));
    connect(&m_fileChangeDelayTimer, SIGNAL(timeout()), SIGNAL(fileChanged()));

    m_fileChangeDelayTimer.setSingleShot(true);
    m_fileUnblockTimer.setSingleShot(true);
}

void DelayingFileWatcher::restart()
{
    m_fileWatcher.addPath(m_filePath);
}

void DelayingFileWatcher::stop()
{
    m_fileWatcher.removePath(m_filePath);
}

void DelayingFileWatcher::start(const QString& filePath)
{
    if (!m_filePath.isEmpty()) {
        m_fileWatcher.removePath(m_filePath);
    }

#if defined(Q_OS_LINUX)
    struct statfs statfsBuf;
    bool forcePolling = false;
    const auto NFS_SUPER_MAGIC = 0x6969;

    if (!statfs(filePath.toLocal8Bit().constData(), &statfsBuf)) {
        forcePolling = (statfsBuf.f_type == NFS_SUPER_MAGIC);
    } else {
        // if we can't get the fs type let's fall back to polling
        forcePolling = true;
    }
    auto objectName = forcePolling ? QLatin1String("_qt_autotest_force_engine_poller") : QLatin1String("");
    m_fileWatcher.setObjectName(objectName);
#endif

    m_fileWatcher.addPath(filePath);

    if (!filePath.isEmpty()) {
        m_filePath = filePath;
    }
}

void DelayingFileWatcher::ignoreFileChanges()
{
    m_ignoreFileChange = true;
    m_fileChangeDelayTimer.stop();
}

void DelayingFileWatcher::observeFileChanges(bool delayed)
{
    int timeout = 0;
    if (delayed) {
        timeout = FileChangeDelay;
    } else {
        m_ignoreFileChange = false;
        start(m_filePath);
    }
    if (timeout > 0 && !m_fileUnblockTimer.isActive()) {
        m_fileUnblockTimer.start(timeout);
    }
}

void DelayingFileWatcher::onWatchedFileChanged()
{
    if (m_ignoreFileChange) {
        // the client forcefully silenced us
        return;
    }
    if (m_fileChangeDelayTimer.isActive()) {
        // we are waiting to fire the delayed fileChanged event, so nothing
        // to do here
        return;
    }

    m_fileChangeDelayTimer.start(FileChangeDelay);
}

BulkFileWatcher::BulkFileWatcher(QObject* parent)
    : QObject(parent)
{
    connect(&m_fileWatcher, SIGNAL(fileChanged(QString)), SLOT(handleFileChanged(QString)));
    connect(&m_fileWatcher, SIGNAL(directoryChanged(QString)), SLOT(handleDirectoryChanged(QString)));
    connect(&m_fileWatchUnblockTimer, SIGNAL(timeout()), this, SLOT(observeFileChanges()));
    m_fileWatchUnblockTimer.setSingleShot(true);
}

void BulkFileWatcher::clear()
{
    for (const QString& path : m_fileWatcher.files() + m_fileWatcher.directories()) {
        const QFileInfo info(path);
        m_fileWatcher.removePath(info.absoluteFilePath());
        m_fileWatcher.removePath(info.absolutePath());
    }
    m_filePaths.clear();
    m_watchedFilesInDirectory.clear();
    m_ignoreFilesChangess.clear();
}

void BulkFileWatcher::removePath(const QString& path)
{
    const QFileInfo info(path);
    m_fileWatcher.removePath(info.absoluteFilePath());
    m_fileWatcher.removePath(info.absolutePath());
    m_filePaths.remove(info.absoluteFilePath());
    m_filePaths.remove(info.absolutePath());
    m_watchedFilesInDirectory[info.absolutePath()].remove(info.absoluteFilePath());
}

void BulkFileWatcher::addPath(const QString& path)
{
    const QFileInfo info(path);
    m_fileWatcher.addPath(info.absoluteFilePath());
    m_fileWatcher.addPath(info.absolutePath());
    m_filePaths.insert(info.absoluteFilePath());
    m_filePaths.insert(info.absolutePath());
    m_watchedFilesInDirectory[info.absolutePath()][info.absoluteFilePath()] = info.exists();
}

void BulkFileWatcher::restart(const QString& path)
{
    const QFileInfo info(path);
    Q_ASSERT(m_filePaths.contains(info.absoluteFilePath()));
    Q_ASSERT(m_filePaths.contains(info.absolutePath()));
    m_fileWatcher.addPath(info.absoluteFilePath());
    m_fileWatcher.addPath(info.absolutePath());
}

void BulkFileWatcher::handleFileChanged(const QString& path)
{
    addPath(path);

    const QFileInfo info(path);
    if (m_ignoreFilesChangess[info.canonicalFilePath()] > Clock::currentDateTimeUtc()) {
        // changes are blocked
        return;
    }

    emit fileChanged(path);
}

void BulkFileWatcher::handleDirectoryChanged(const QString& path)
{
    qDebug("Directory changed %s", qPrintable(path));
    const QFileInfo directory(path);
    const QMap<QString, bool>& watchedFiles = m_watchedFilesInDirectory[directory.absolutePath()];
    for (const QString& file : watchedFiles.keys()) {
        const QFileInfo info(file);
        const bool existed = watchedFiles[info.absoluteFilePath()];
        if (!info.exists() && existed) {
            qDebug("Remove watch file %s", qPrintable(info.absoluteFilePath()));
            m_fileWatcher.removePath(info.absolutePath());
            emit fileRemoved(info.absoluteFilePath());
        }
        if (!existed && info.exists()) {
            qDebug("Add watch file %s", qPrintable(info.absoluteFilePath()));
            m_fileWatcher.addPath(info.absolutePath());
            emit fileCreated(info.absoluteFilePath());
        }
        if (existed && info.exists()) {
            qDebug("Refresh watch file %s", qPrintable(info.absoluteFilePath()));
            m_fileWatcher.removePath(info.absolutePath());
            m_fileWatcher.addPath(info.absolutePath());
            emit fileChanged(info.absoluteFilePath());
        }
        m_watchedFilesInDirectory[info.absolutePath()][info.absoluteFilePath()] = info.exists();
    }
}

void BulkFileWatcher::ignoreFileChanges(const QString& path)
{
    const QFileInfo info(path);
    m_ignoreFilesChangess[info.canonicalFilePath()] = Clock::currentDateTimeUtc().addMSecs(FileChangeDelay);
}

void BulkFileWatcher::observeFileChanges(bool delayed)
{
    int timeout = 0;
    if (delayed) {
        timeout = TimerResolution;
    } else {
        const QDateTime current = Clock::currentDateTimeUtc();
        for (const QString& key : m_ignoreFilesChangess.keys()) {
            if (m_ignoreFilesChangess[key] < current) {
                // We assume that there was no concurrent change of the database
                // during our block - so no need to reimport
                qDebug("Remove block from %s", qPrintable(key));
                m_ignoreFilesChangess.remove(key);
                continue;
            }
            qDebug("Keep block from %s", qPrintable(key));
            timeout = static_cast<int>(current.msecsTo(m_ignoreFilesChangess[key]));
        }
    }
    if (timeout > 0 && !m_fileWatchUnblockTimer.isActive()) {
        m_fileWatchUnblockTimer.start(timeout);
    }
}
