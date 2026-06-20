/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "RemoteDatabaseManager.h"
#include "RemoteDatabase.h"

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

RemoteDatabaseManager::RemoteDatabaseManager(QObject* parent)
    : QObject(parent)
{
}

RemoteDatabaseManager::~RemoteDatabaseManager()
{
    cleanupTempFiles();
    qDeleteAll(m_remoteDatabases);
}

void RemoteDatabaseManager::downloadDatabase(const QString& url,
                                             const QString& password,
                                             const QString& keyfile)
{
    if (url.isEmpty()) {
        return;
    }

    // Store credentials temporarily
    if (!password.isEmpty()) {
        m_passwords[url] = password;
    }
    if (!keyfile.isEmpty()) {
        m_keyfiles[url] = keyfile;
    }

    RemoteDatabase* remoteDb = getOrCreateRemoteDatabase(url);
    QString localPath = generateLocalCachePath(url);
    m_localCachePaths[url] = localPath;

    remoteDb->download(localPath);
}

void RemoteDatabaseManager::uploadDatabase(const QString& url, const QString& localPath)
{
    if (url.isEmpty() || localPath.isEmpty()) {
        return;
    }

    RemoteDatabase* remoteDb = getOrCreateRemoteDatabase(url);
    remoteDb->upload(localPath);
}

void RemoteDatabaseManager::cancelOperation(const QString& url)
{
    if (m_remoteDatabases.contains(url)) {
        m_remoteDatabases[url]->cancel();
    }
}

bool RemoteDatabaseManager::isDownloading(const QString& url) const
{
    return m_remoteDatabases.contains(url) && m_remoteDatabases[url]->state() == RemoteDatabase::Downloading;
}

bool RemoteDatabaseManager::isUploading(const QString& url) const
{
    return m_remoteDatabases.contains(url) && m_remoteDatabases[url]->state() == RemoteDatabase::Uploading;
}

void RemoteDatabaseManager::handleDownloadComplete(const QString& url, const QString& localPath)
{
    emit databaseDownloaded(url, localPath);
}

void RemoteDatabaseManager::handleUploadComplete(const QString& url)
{
    emit databaseUploaded(url);
}

void RemoteDatabaseManager::handleError(const QString& url, const QString& error)
{
    emit errorOccurred(url, error);
}

void RemoteDatabaseManager::handleDownloadProgress(const QString& url, qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(url, bytesReceived, bytesTotal);
}

void RemoteDatabaseManager::handleUploadProgress(const QString& url, qint64 bytesSent, qint64 bytesTotal)
{
    emit uploadProgress(url, bytesSent, bytesTotal);
}

RemoteDatabase* RemoteDatabaseManager::getOrCreateRemoteDatabase(const QString& url)
{
    if (!m_remoteDatabases.contains(url)) {
        RemoteDatabase* remoteDb = new RemoteDatabase(url, this);
        
        connect(remoteDb, &RemoteDatabase::downloadComplete,
                this, &RemoteDatabaseManager::handleDownloadComplete);
        connect(remoteDb, &RemoteDatabase::uploadComplete,
                this, &RemoteDatabaseManager::handleUploadComplete);
        connect(remoteDb, &RemoteDatabase::errorOccurred,
                this, &RemoteDatabaseManager::handleError);
        connect(remoteDb, &RemoteDatabase::downloadProgress,
                this, &RemoteDatabaseManager::handleDownloadProgress);
        connect(remoteDb, &RemoteDatabase::uploadProgress,
                this, &RemoteDatabaseManager::handleUploadProgress);
        
        m_remoteDatabases[url] = remoteDb;
        
        // Set credentials if available
        if (m_passwords.contains(url)) {
            remoteDb->setPassword(m_passwords[url]);
        }
        if (m_keyfiles.contains(url)) {
            remoteDb->setKeyfile(m_keyfiles[url]);
        }
    }
    
    return m_remoteDatabases[url];
}

QString RemoteDatabaseManager::generateLocalCachePath(const QString& url)
{
    QUrl qurl(url);
    QString fileName = QFileInfo(qurl.path()).fileName();
    if (fileName.isEmpty()) {
        fileName = "remote_database.kdbx";
    }
    
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString uniqueId = QUuid::createUuid().toString(QUuid::Id128);
    
    return QDir(cacheDir).filePath(uniqueId + "_" + fileName);
}

void RemoteDatabaseManager::cleanupTempFiles()
{
    for (auto it = m_localCachePaths.begin(); it != m_localCachePaths.end(); ++it) {
        QFile::remove(it.value());
    }
    m_localCachePaths.clear();
}

FILE: src/remote/RemoteDatabase.h
ACTION: create
CONTENT: