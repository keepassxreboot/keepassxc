/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_FILEWATCHER_H
#define KEEPASSXC_FILEWATCHER_H

#include <QFileSystemWatcher>
#include <QSet>
#include <QTimer>
#include <QVariant>

class DelayingFileWatcher : public QObject
{
    Q_OBJECT

public:
    explicit DelayingFileWatcher(QObject* parent = nullptr);

    void blockAutoReload(bool block);
    void start(const QString& path);

    void restart();
    void stop();
    void ignoreFileChanges();

signals:
    void fileChanged();

public slots:
    void observeFileChanges(bool delayed = false);

private slots:
    void onWatchedFileChanged();

private:
    QString m_filePath;
    QFileSystemWatcher m_fileWatcher;
    QTimer m_fileChangeDelayTimer;
    QTimer m_fileUnblockTimer;
    bool m_ignoreFileChange;
};

class BulkFileWatcher : public QObject
{
    Q_OBJECT

    enum Signal
    {
        Created,
        Updated,
        Removed
    };

public:
    explicit BulkFileWatcher(QObject* parent = nullptr);

    void clear();

    void removePath(const QString& path);
    void addPath(const QString& path);

    void ignoreFileChanges(const QString& path);

signals:
    void fileCreated(QString);
    void fileChanged(QString);
    void fileRemoved(QString);

public slots:
    void observeFileChanges(bool delayed = false);

private slots:
    void handleFileChanged(const QString& path);
    void handleDirectoryChanged(const QString& path);
    void emitSignals();

private:
    void scheduleSignal(Signal event, const QString& path);

private:
    QMap<QString, bool> m_watchedPaths;
    QMap<QString, QDateTime> m_ignoreFilesChangess;
    QFileSystemWatcher m_fileWatcher;
    QMap<QString, QMap<QString, bool>> m_watchedFilesInDirectory;
    // needed for Import/Export-References to prevent update after self-write
    QTimer m_fileWatchUnblockTimer;
    // needed to tolerate multiple signals for same event
    QTimer m_pendingSignalsTimer;
    QMap<QString, QList<Signal>> m_pendingSignals;
};

#endif // KEEPASSXC_FILEWATCHER_H
