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

#ifndef KEEPASSXC_REMOTEDATABASEMANAGER_H
#define KEEPASSXC_REMOTEDATABASEMANAGER_H

#include <QObject>
#include <QUrl>
#include <QByteArray>
#include <QFuture>
#include <QNetworkAccessManager>
#include <QSslError>
#include <QScopedPointer>

class QNetworkReply;
class QProgressDialog;

class RemoteDatabaseManager : public QObject
{
    Q_OBJECT

public:
    enum Protocol {
        WebDAV,
        SFTP,
        FTP,
        Unknown
    };

    enum TransferStatus {
        Idle,
        Uploading,
        Downloading,
        Error
    };

    struct RemoteFileInfo {
        QString name;
        qint64 size;
        QDateTime lastModified;
        bool isDirectory;
    };

    explicit RemoteDatabaseManager(QObject* parent = nullptr);
    ~RemoteDatabaseManager() override;

    /**
     * Check if the given URL represents a remote database location
     */
    static bool isRemoteUrl(const QString& url);
    
    /**
     * Detect protocol from URL scheme
     */
    static Protocol detectProtocol(const QUrl& url);
    
    /**
     * Detect protocol from URL string
     */
    static Protocol detectProtocol(const QString& url);

    /**
     * Download a remote database file
     * @param url Remote URL (webdav://, sftp://, ftp://)
     * @param parentWidget Parent widget for progress dialog
     * @return Downloaded file data, empty on error
     */
    QByteArray downloadDatabase(const QUrl& url, QWidget* parentWidget = nullptr);

    /**
     * Upload a database file to remote location
     * @param url Remote URL
     * @param data Database file data
     * @param parentWidget Parent widget for progress dialog
     * @return true on success
     */
    bool uploadDatabase(const QUrl& url, const QByteArray& data, QWidget* parentWidget = nullptr);

    /**
     * Check if remote file exists
     * @param url Remote URL
     * @return true if file exists
     */
    bool remoteFileExists(const QUrl& url);

    /**
     * Get remote file information
     * @param url Remote URL
     * @param info Output file info
     * @return true on success
     */
    bool getRemoteFileInfo(const QUrl& url, RemoteFileInfo& info);

    /**
     * List files in remote directory
     * @param url Remote directory URL
     * @param files Output file list
     * @return true on success
     */
    bool listRemoteDirectory(const QUrl& url, QVector<RemoteFileInfo>& files);

    /**
     * Create remote directory
     * @param url Remote directory URL
     * @return true on success
     */
    bool createRemoteDirectory(const QUrl& url);

    /**
     * Delete remote file
     * @param url Remote URL
     * @return true on success
     */
    bool deleteRemoteFile(const QUrl& url);

    /**
     * Get the last error message
     */
    QString lastError() const;

    /**
     * Get current transfer status
     */
    TransferStatus status() const;

    /**
     * Cancel current operation
     */
    void cancel();

signals:
    /**
     * Emitted when transfer progress changes
     * @param bytesTransferred Bytes transferred so far
     * @param totalBytes Total bytes to transfer
     */
    void progressChanged(qint64 bytesTransferred, qint64 totalBytes);

    /**
     * Emitted when operation completes
     * @param success true if operation succeeded
     */
    void finished(bool success);

    /**
     * Emitted when authentication is required
     * @param url The URL requiring authentication
     * @param username Output username (set by slot)
     * @param password Output password (set by slot)
     */
    void authenticationRequired(const QUrl& url, QString& username, QString& password);

    /**
     * Emitted when SSL errors occur
     * @param errors List of SSL errors
     * @param url The URL with SSL errors
     * @return true to ignore errors, false to abort
     */
    bool sslErrorsOccurred(const QList<QSslError>& errors, const QUrl& url);

private slots:
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    // WebDAV operations
    QByteArray downloadWebDAV(const QUrl& url);
    bool uploadWebDAV(const QUrl& url, const QByteArray& data);
    bool propfindWebDAV(const QUrl& url, QVector<RemoteFileInfo>& files);
    bool mkdirWebDAV(const QUrl& url);
    bool deleteWebDAV(const QUrl& url);

    // SFTP operations
    QByteArray downloadSFTP(const QUrl& url);
    bool uploadSFTP(const QUrl& url, const QByteArray& data);
    bool statSFTP(const QUrl& url, RemoteFileInfo& info);
    bool listSFTP(const QUrl& url, QVector<RemoteFileInfo>& files);
    bool mkdirSFTP(const QUrl& url);
    bool deleteSFTP(const QUrl& url);

    // FTP operations
    QByteArray downloadFTP(const QUrl& url);
    bool uploadFTP(const QUrl& url, const QByteArray& data);

    // Utility methods
    QNetworkReply* executeRequest(const QNetworkRequest& request, 
                                   const QByteArray& verb,
                                   const QByteArray& data = QByteArray());
    bool waitForReply(QNetworkReply* reply);
    void setProgressWidget(QWidget* parentWidget);
    void closeProgressWidget();
    bool handleAuthentication(const QUrl& url);
    QUrl resolveUrl(const QUrl& url) const;

    QNetworkAccessManager* m_networkManager;
    QScopedPointer<QProgressDialog> m_progressDialog;
    TransferStatus m_status;
    QString m_lastError;
    bool m_cancelled;
    QString m_username;
    QString m_password;

    // SFTP process management
    class SFTPProcess;
    SFTPProcess* m_sftpProcess;
};

#endif // KEEPASSXC_REMOTEDATABASEMANAGER_H