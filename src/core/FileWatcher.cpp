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
} // namespace

DelayingFileWatcher::DelayingFileWatcher(QObject* parent)
    : QObject(parent)
    , m_ignoreFileChange(false)
{
    connect(&m_fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged()));
    connect(&m_fileUnblockTimer, SIGNAL(timeout()), this, SLOT(observeFileChanges()));
    connect(&m_fileChangeDelayTimer, SIGNAL(timeout()), this, SIGNAL(fileChanged()));
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
    connect(&m_watchedFilesIgnoreTimer, SIGNAL(timeout()), this, SLOT(observeFileChanges()));
    connect(&m_pendingSignalsTimer, SIGNAL(timeout()), this, SLOT(emitSignals()));
    m_watchedFilesIgnoreTimer.setSingleShot(true);
    m_pendingSignalsTimer.setSingleShot(true);
}

void BulkFileWatcher::clear()
{
    for (const QString& path : m_fileWatcher.files() + m_fileWatcher.directories()) {
        const QFileInfo info(path);
        m_fileWatcher.removePath(info.absoluteFilePath());
        m_fileWatcher.removePath(info.absolutePath());
    }
    m_watchedPaths.clear();
    m_watchedFilesInDirectory.clear();
    m_watchedFilesIgnored.clear();
}

void BulkFileWatcher::removePath(const QString& path)
{
    const QFileInfo info(path);
    const QString filePath = info.absoluteFilePath();
    const QString directoryPath = info.absolutePath();
    m_watchedFilesInDirectory[directoryPath].remove(filePath);
    m_fileWatcher.removePath(filePath);
    m_watchedPaths.remove(filePath);
    if (m_watchedFilesInDirectory[directoryPath].isEmpty()) {
        m_fileWatcher.removePath(directoryPath);
        m_watchedPaths.remove(directoryPath);
        m_watchedFilesInDirectory.remove(directoryPath);
    }
}

void BulkFileWatcher::addPath(const QString& path)
{
    const QFileInfo info(path);
    const QString filePath = info.absoluteFilePath();
    const QString directoryPath = info.absolutePath();
    if (!m_watchedPaths.value(filePath)) {
        const bool fileSuccess = m_fileWatcher.addPath(filePath);
        m_watchedPaths[filePath] = fileSuccess;
    }
    if (!m_watchedPaths.value(directoryPath)) {
        const bool directorySuccess = m_fileWatcher.addPath(directoryPath);
        m_watchedPaths[directoryPath] = directorySuccess;
    }
    m_watchedFilesInDirectory[directoryPath][filePath] = info.exists() ? info.lastModified().toMSecsSinceEpoch() : 0;
}

void BulkFileWatcher::handleFileChanged(const QString& path)
{
    const QFileInfo info(path);
    const QString filePath = info.absoluteFilePath();
    const QString directoryPath = info.absolutePath();
    const QMap<QString, qint64>& watchedFiles = m_watchedFilesInDirectory[directoryPath];
    const qint64 lastModificationTime = info.lastModified().toMSecsSinceEpoch();
    const bool created = watchedFiles[filePath] == 0 && info.exists();
    const bool deleted = watchedFiles[filePath] != 0 && !info.exists();
    const bool changed = !created && !deleted && lastModificationTime != watchedFiles[filePath];

    addPath(path);

    if (m_watchedFilesIgnored[info.canonicalFilePath()] > Clock::currentDateTimeUtc()) {
        // changes are blocked
        return;
    }
    if (created) {
        qDebug("File created %s", qPrintable(path));
        scheduleSignal(Created, filePath);
    }
    if (changed) {
        qDebug("File changed %s", qPrintable(path));
        scheduleSignal(Updated, filePath);
    }
    if (deleted) {
        qDebug("File removed %s", qPrintable(path));
        scheduleSignal(Removed, filePath);
    }
}

void BulkFileWatcher::handleDirectoryChanged(const QString& path)
{
    qDebug("Directory changed %s", qPrintable(path));
    const QFileInfo directoryInfo(path);
    const QString directoryPath = directoryInfo.absoluteFilePath();
    QMap<QString, qint64>& watchedFiles = m_watchedFilesInDirectory[directoryPath];
    for (const QString& filename : watchedFiles.keys()) {
        const QFileInfo fileInfo(filename);
        const QString filePath = fileInfo.absoluteFilePath();
        const qint64 previousModificationTime = watchedFiles[filePath];
        const qint64 lastModificationTime = fileInfo.lastModified().toMSecsSinceEpoch();
        if (!fileInfo.exists() && previousModificationTime != 0) {
            qDebug("Remove watch file %s", qPrintable(fileInfo.absoluteFilePath()));
            m_fileWatcher.removePath(filePath);
            m_watchedPaths.remove(filePath);
            watchedFiles.remove(filePath);
            scheduleSignal(Removed, filePath);
        }
        if (previousModificationTime == 0 && fileInfo.exists()) {
            qDebug("Add watch file %s", qPrintable(fileInfo.absoluteFilePath()));
            if (!m_watchedPaths.value(filePath)) {
                const bool success = m_fileWatcher.addPath(filePath);
                m_watchedPaths[filePath] = success;
                watchedFiles[filePath] = lastModificationTime;
            }
            scheduleSignal(Created, filePath);
        }
        if (fileInfo.exists() && previousModificationTime != lastModificationTime) {
            // this case is handled using
            qDebug("Refresh watch file %s", qPrintable(fileInfo.absoluteFilePath()));
            m_fileWatcher.removePath(fileInfo.absolutePath());
            m_fileWatcher.addPath(fileInfo.absolutePath());
            scheduleSignal(Updated, filePath);
        }
        m_watchedFilesInDirectory[directoryPath][filePath] = fileInfo.exists() ? lastModificationTime : 0;
    }
}

void BulkFileWatcher::emitSignals()
{
    QMap<QString, QList<Signal>> queued;
    m_pendingSignals.swap(queued);
    for (const auto& path : queued.keys()) {
        const auto& signal = queued[path];
        if (signal.last() == Removed) {
            qDebug("Emit %s removed", qPrintable(path));
            emit fileRemoved(path);
            continue;
        }
        if (signal.first() == Created) {
            qDebug("Emit %s created", qPrintable(path));
            emit fileCreated(path);
            continue;
        }
        qDebug("Emit %s changed", qPrintable(path));
        emit fileChanged(path);
    }
}

void BulkFileWatcher::scheduleSignal(Signal signal, const QString& path)
{
    // we need to collect signals since the file watcher API may send multiple signals for a "single" change
    // therefore we wait until the event loop finished before starting to import any changes
    const QString filePath = QFileInfo(path).absoluteFilePath();
    m_pendingSignals[filePath] << signal;

    if (!m_pendingSignalsTimer.isActive()) {
        m_pendingSignalsTimer.start();
    }
}

void BulkFileWatcher::ignoreFileChanges(const QString& path)
{
    const QFileInfo info(path);
    m_watchedFilesIgnored[info.canonicalFilePath()] = Clock::currentDateTimeUtc().addMSecs(FileChangeDelay);
}

void BulkFileWatcher::observeFileChanges(bool delayed)
{
    int timeout = 0;
    if (delayed) {
        timeout = TimerResolution;
    } else {
        const QDateTime current = Clock::currentDateTimeUtc();
        for (const QString& key : m_watchedFilesIgnored.keys()) {
            if (m_watchedFilesIgnored[key] < current) {
                // We assume that there was no concurrent change of the database
                // during our block - so no need to reimport
                qDebug("Remove block from %s", qPrintable(key));
                m_watchedFilesIgnored.remove(key);
                continue;
            }
            qDebug("Keep block from %s", qPrintable(key));
            timeout = qMin(timeout, static_cast<int>(current.msecsTo(m_watchedFilesIgnored[key])));
        }
    }
    if (timeout > 0 && !m_watchedFilesIgnoreTimer.isActive()) {
        m_watchedFilesIgnoreTimer.start(timeout);
    }
}
