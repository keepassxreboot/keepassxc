/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_GOOGLEDRIVESERVICE_H
#define KEEPASSXC_GOOGLEDRIVESERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QFile>
#include <QTcpServer>

class GoogleDriveService : public QObject
{
    Q_OBJECT

public:
    struct FileInfo
    {
        QString id;
        QString name;
        QDateTime modifiedTime;
        qint64 size;
        QString mimeType;
        bool isFolder() const
        {
            return mimeType == "application/vnd.google-apps.folder";
        }
    };

    explicit GoogleDriveService(QObject* parent = nullptr);
    ~GoogleDriveService() override;

    bool isAuthenticated() const;

    void authenticate();
    void disconnect();

    void listFiles(const QString& parentId = {});
    void downloadFile(const QString& fileId, const QString& localPath);
    void uploadFile(const QString& localPath, const QString& parentId = {},
                   const QString& name = {});
    void updateFile(const QString& fileId, const QString& localPath,
                    const QString& name = {});
    void deleteFile(const QString& fileId);
    void createFolder(const QString& parentId, const QString& name);

signals:
    void authStatusChanged(bool authenticated);
    void authFailed(const QString& error);
    void fileListReady(const QList<FileInfo>& files);
    void fileListFailed(const QString& error);
    void fileDownloaded(const QString& fileId, const QString& localPath);
    void fileDownloadFailed(const QString& fileId, const QString& error);
    void fileUploaded(const QString& fileId, const QString& name);
    void fileUploadFailed(const QString& error);
    void fileDeleted(const QString& fileId);
    void fileDeleteFailed(const QString& fileId, const QString& error);
    void folderCreated(const QString& folderId, const QString& name);
    void folderCreateFailed(const QString& error);

private slots:
    void onTokenExchangeFinished();
    void onRefreshFinished();
    void onListFinished();
    void onDownloadFinished();
    void onUploadFinished();
    void onCreateFolderFinished();

private:
    void exchangeAuthCode(const QString& code);
    bool ensureToken();
    void refreshAccessToken();
    void handleAuthRedirect();
    QNetworkRequest makeRequest(const QString& url, bool auth = true) const;
    void storeTokens(const QString& accessToken, const QString& refreshToken, int expiresIn);

    QString m_accessToken;
    QString m_refreshToken;
    QDateTime m_tokenExpiry;
    QNetworkAccessManager* m_net;
    QTcpServer* m_callbackServer;
    QString m_authState;

    enum class PendingOp { None, List, Download, Upload, Delete, CreateFolder };
    PendingOp m_pendingOp = PendingOp::None;
    QString m_pendingFileId;
    QString m_pendingLocalPath;
    QString m_pendingParentId;
    QString m_pendingUploadName;
    QString m_pendingListParentId;
    QString m_pendingCreateFolderName;

    QNetworkReply* m_tokenReply = nullptr;
    QNetworkReply* m_listReply = nullptr;
    QNetworkReply* m_downloadReply = nullptr;
    QNetworkReply* m_uploadReply = nullptr;
    QNetworkReply* m_createFolderReply = nullptr;
    QFile* m_downloadFile = nullptr;
};

#endif // KEEPASSXC_GOOGLEDRIVESERVICE_H
