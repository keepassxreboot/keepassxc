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

#include "GoogleDriveService.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QUrlQuery>

#include "core/Config.h"
#include "config-keepassx.h"

#ifdef GOOGLEDRIVE_CLIENT_ID
static constexpr char BuiltInClientId[] = GOOGLEDRIVE_CLIENT_ID;
static constexpr char BuiltInClientSecret[] = GOOGLEDRIVE_CLIENT_SECRET;
#endif

static QString driveClientId()
{
    QString id = config()->get(Config::GoogleDrive_ClientId).toString();
#ifdef GOOGLEDRIVE_CLIENT_ID
    if (id.isEmpty()) {
        id = QString::fromLatin1(BuiltInClientId);
    }
#endif
    return id;
}

static QString driveClientSecret()
{
    QString secret = config()->get(Config::GoogleDrive_ClientSecret).toString();
#ifdef GOOGLEDRIVE_CLIENT_ID
    if (secret.isEmpty()) {
        secret = QString::fromLatin1(BuiltInClientSecret);
    }
#endif
    return secret;
}

static constexpr int RedirectPort = 18080;
static constexpr char RedirectUri[] = "http://localhost:18080/";
static constexpr char AuthUrl[] = "https://accounts.google.com/o/oauth2/v2/auth";
static constexpr char TokenUrl[] = "https://oauth2.googleapis.com/token";
static constexpr char DriveApiUrl[] = "https://www.googleapis.com/drive/v3/files";
static constexpr char DriveUploadUrl[] = "https://www.googleapis.com/upload/drive/v3/files";
static constexpr char Scope[] = "https://www.googleapis.com/auth/drive";

GoogleDriveService::GoogleDriveService(QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
    , m_callbackServer(new QTcpServer(this))
{
    m_accessToken = config()->get(Config::GoogleDrive_AccessToken).toString();
    m_refreshToken = config()->get(Config::GoogleDrive_RefreshToken).toString();
    m_tokenExpiry = config()->get(Config::GoogleDrive_AccessTokenExpiry).toDateTime();

    connect(m_callbackServer, &QTcpServer::newConnection, this, [this] {
        handleAuthRedirect();
    });
}

GoogleDriveService::~GoogleDriveService()
{
    if (m_callbackServer->isListening()) {
        m_callbackServer->close();
    }
}

bool GoogleDriveService::isAuthenticated() const
{
    return !m_refreshToken.isEmpty();
}

void GoogleDriveService::authenticate()
{
    QString clientId = driveClientId();
    if (clientId.isEmpty()) {
        emit authFailed(tr("No Google Drive Client ID configured. "
                           "Please add your Client ID in the Google Drive settings."));
        return;
    }

    if (m_callbackServer->isListening()) {
        m_callbackServer->close();
    }

    if (!m_callbackServer->listen(QHostAddress::LocalHost, RedirectPort)) {
        emit authFailed(tr("Cannot start local server for OAuth callback on port %1. "
                           "The port may be in use.").arg(RedirectPort));
        return;
    }

    m_authState = QString::number(QRandomGenerator::global()->generate());

    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("redirect_uri", RedirectUri);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", Scope);
    query.addQueryItem("state", m_authState);
    query.addQueryItem("access_type", "offline");
    query.addQueryItem("prompt", "consent");

    QUrl url(AuthUrl);
    url.setQuery(query);

    QDesktopServices::openUrl(url);
}

void GoogleDriveService::disconnect()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpiry = {};
    config()->remove(Config::GoogleDrive_AccessToken);
    config()->remove(Config::GoogleDrive_RefreshToken);
    config()->remove(Config::GoogleDrive_AccessTokenExpiry);
    config()->sync();
    emit authStatusChanged(false);
}

void GoogleDriveService::handleAuthRedirect()
{
    auto socket = m_callbackServer->nextPendingConnection();
    socket->waitForReadyRead(5000);
    QByteArray data = socket->readAll();

    int codePos = data.indexOf("code=");

    if (codePos > 0) {
        int end = data.indexOf('&', codePos);
        if (end < 0) {
            end = data.indexOf(' ', codePos);
        }
        QString code = QString::fromUtf8(data.mid(codePos + 5, end - codePos - 5));
        code = QUrl::fromPercentEncoding(code.toUtf8());

        QString response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                           "<html><body><h1>" + tr("Authenticated!") + "</h1><p>"
                           + tr("You can close this window now.") + "</p></body></html>";
        socket->write(response.toUtf8());
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->close();

        exchangeAuthCode(code);
    } else {
        QString response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
                           "<html><body><h1>" + tr("Authentication failed") + "</h1></body></html>";
        socket->write(response.toUtf8());
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->close();
    }
    socket->deleteLater();
}

void GoogleDriveService::exchangeAuthCode(const QString& code)
{
    QString clientId = driveClientId();
    QString clientSecret = driveClientSecret();

    QUrlQuery params;
    params.addQueryItem("code", code);
    params.addQueryItem("client_id", clientId);
    params.addQueryItem("client_secret", clientSecret);
    params.addQueryItem("redirect_uri", RedirectUri);
    params.addQueryItem("grant_type", "authorization_code");

    QNetworkRequest request{QUrl(TokenUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    m_tokenReply = m_net->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_tokenReply, &QNetworkReply::finished, this, &GoogleDriveService::onTokenExchangeFinished);
}

void GoogleDriveService::onTokenExchangeFinished()
{
    auto reply = m_tokenReply;
    m_tokenReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit authFailed(QString::fromUtf8(reply->readAll()));
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    QString accessToken = obj.value("access_token").toString();
    QString refreshToken = obj.value("refresh_token").toString();
    int expiresIn = obj.value("expires_in").toInt();

    storeTokens(accessToken, refreshToken, expiresIn);
    emit authStatusChanged(true);

    reply->deleteLater();
}

void GoogleDriveService::refreshAccessToken()
{
    QString clientId = driveClientId();
    QString clientSecret = driveClientSecret();

    QUrlQuery params;
    params.addQueryItem("refresh_token", m_refreshToken);
    params.addQueryItem("client_id", clientId);
    params.addQueryItem("client_secret", clientSecret);
    params.addQueryItem("grant_type", "refresh_token");

    QNetworkRequest request{QUrl(TokenUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    m_tokenReply = m_net->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_tokenReply, &QNetworkReply::finished, this, &GoogleDriveService::onRefreshFinished);
}

void GoogleDriveService::onRefreshFinished()
{
    auto reply = m_tokenReply;
    m_tokenReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        PendingOp failedOp = m_pendingOp;
        m_pendingOp = PendingOp::None;
        reply->deleteLater();

        if (failedOp == PendingOp::List) {
            emit fileListFailed(tr("Token refresh failed. Please re-authenticate."));
        } else if (failedOp == PendingOp::Download) {
            emit fileDownloadFailed(m_pendingFileId, tr("Token refresh failed. Please re-authenticate."));
        } else if (failedOp == PendingOp::Upload) {
            emit fileUploadFailed(tr("Token refresh failed. Please re-authenticate."));
        } else if (failedOp == PendingOp::CreateFolder) {
            emit folderCreateFailed(tr("Token refresh failed. Please re-authenticate."));
        }
        emit authFailed(tr("Token refresh failed. Please re-authenticate."));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    QString accessToken = obj.value("access_token").toString();
    int expiresIn = obj.value("expires_in").toInt();
    storeTokens(accessToken, m_refreshToken, expiresIn);

    PendingOp retryOp = m_pendingOp;
    m_pendingOp = PendingOp::None;

    switch (retryOp) {
    case PendingOp::List:
        listFiles(m_pendingListParentId);
        break;
    case PendingOp::Download:
        downloadFile(m_pendingFileId, m_pendingLocalPath);
        break;
    case PendingOp::Upload:
        uploadFile(m_pendingLocalPath, m_pendingParentId, m_pendingUploadName);
        break;
    case PendingOp::Delete:
        deleteFile(m_pendingFileId);
        break;
    case PendingOp::CreateFolder:
        createFolder(m_pendingParentId, m_pendingCreateFolderName);
        break;
    default:
        break;
    }

    reply->deleteLater();
}

void GoogleDriveService::storeTokens(const QString& accessToken, const QString& refreshToken, int expiresIn)
{
    m_accessToken = accessToken;
    if (!refreshToken.isEmpty()) {
        m_refreshToken = refreshToken;
        config()->set(Config::GoogleDrive_RefreshToken, refreshToken);
    }
    m_tokenExpiry = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
    config()->set(Config::GoogleDrive_AccessToken, accessToken);
    config()->set(Config::GoogleDrive_AccessTokenExpiry, m_tokenExpiry);
    config()->sync();
}

bool GoogleDriveService::ensureToken()
{
    if (m_accessToken.isEmpty()) {
        return false;
    }
    if (!m_tokenExpiry.isValid() || m_tokenExpiry > QDateTime::currentDateTimeUtc()) {
        return true;
    }
    return false;
}

QNetworkRequest GoogleDriveService::makeRequest(const QString& url, bool auth) const
{
    QNetworkRequest request{QUrl(url)};
    if (auth) {
        request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());
    }
    return request;
}

void GoogleDriveService::listFiles(const QString& parentId)
{
    m_pendingListParentId = parentId;

    if (!ensureToken()) {
        m_pendingOp = PendingOp::List;
        refreshAccessToken();
        return;
    }

    QUrlQuery params;
    params.addQueryItem("pageSize", "100");
    params.addQueryItem("fields", "files(id,name,modifiedTime,size,mimeType)");
    params.addQueryItem("orderBy", "name");

    if (parentId.isEmpty()) {
        params.addQueryItem("q", "'root' in parents");
    } else {
        params.addQueryItem("q", "'" + parentId + "' in parents");
    }

    QUrl url(DriveApiUrl);
    url.setQuery(params);

    m_listReply = m_net->get(makeRequest(url.toString()));
    connect(m_listReply, &QNetworkReply::finished, this, &GoogleDriveService::onListFinished);
}

void GoogleDriveService::onListFinished()
{
    auto reply = m_listReply;
    m_listReply = nullptr;

    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit fileListFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray rawResponse = reply->readAll();

    QJsonDocument doc = QJsonDocument::fromJson(rawResponse);
    QJsonArray items = doc.object().value("files").toArray();

    QList<FileInfo> files;
    for (const QJsonValue& val : items) {
        QJsonObject obj = val.toObject();
        FileInfo info;
        info.id = obj.value("id").toString();
        info.name = obj.value("name").toString();
        info.modifiedTime = QDateTime::fromString(obj.value("modifiedTime").toString(), Qt::ISODate);
        info.size = obj.value("size").toString().toLongLong();
        info.mimeType = obj.value("mimeType").toString();
        files.append(info);
    }

    emit fileListReady(files);
    reply->deleteLater();
}

void GoogleDriveService::downloadFile(const QString& fileId, const QString& localPath)
{
    if (!ensureToken()) {
        m_pendingOp = PendingOp::Download;
        m_pendingFileId = fileId;
        m_pendingLocalPath = localPath;
        refreshAccessToken();
        return;
    }

    m_pendingFileId = fileId;
    m_downloadFile = new QFile(localPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        emit fileDownloadFailed(fileId, tr("Cannot open local file for writing: %1").arg(localPath));
        return;
    }

    QUrl url(QString(DriveApiUrl) + "/" + fileId + "?alt=media");
    m_downloadReply = m_net->get(makeRequest(url.toString()));
    connect(m_downloadReply, &QNetworkReply::finished, this, &GoogleDriveService::onDownloadFinished);
    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this] {
        if (m_downloadFile) {
            m_downloadFile->write(m_downloadReply->readAll());
        }
    });
}

void GoogleDriveService::onDownloadFinished()
{
    auto reply = m_downloadReply;
    m_downloadReply = nullptr;

    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit fileDownloadFailed(m_pendingFileId, reply->errorString());
    } else {
        if (m_downloadFile) {
            m_downloadFile->write(reply->readAll());
            m_downloadFile->close();
        }
        QString path = m_downloadFile ? m_downloadFile->fileName() : QString();
        emit fileDownloaded(m_pendingFileId, path);
    }

    if (m_downloadFile) {
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }
    reply->deleteLater();
}

void GoogleDriveService::uploadFile(const QString& localPath, const QString& parentId, const QString& name)
{
    if (!ensureToken()) {
        m_pendingOp = PendingOp::Upload;
        m_pendingLocalPath = localPath;
        m_pendingParentId = parentId;
        m_pendingUploadName = name;
        refreshAccessToken();
        return;
    }

    QFile* file = new QFile(localPath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit fileUploadFailed(tr("Cannot open local file: %1").arg(localPath));
        delete file;
        return;
    }

    QString fileName = name.isEmpty() ? QFileInfo(localPath).fileName() : name;

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QJsonObject metadata;
    metadata["name"] = fileName;
    metadata["mimeType"] = "application/x-keepass2";
    if (!parentId.isEmpty()) {
        QJsonArray parents;
        parents.append(parentId);
        metadata["parents"] = parents;
    }

    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          "form-data; name=\"metadata\"");
    metadataPart.setBody(QJsonDocument(metadata).toJson());

    QHttpPart mediaPart;
    mediaPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    mediaPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"media\"; filename=\"%1\"").arg(fileName));
    mediaPart.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(metadataPart);
    multiPart->append(mediaPart);

    QUrlQuery params;
    params.addQueryItem("uploadType", "multipart");
    QUrl url(DriveUploadUrl);
    url.setQuery(params);

    m_uploadReply = m_net->post(makeRequest(url.toString()), multiPart);
    multiPart->setParent(m_uploadReply);

    connect(m_uploadReply, &QNetworkReply::finished, this, &GoogleDriveService::onUploadFinished);
}

void GoogleDriveService::updateFile(const QString& fileId, const QString& localPath, const QString& name)
{
    if (!ensureToken()) {
        m_pendingOp = PendingOp::Upload;
        m_pendingFileId = fileId;
        m_pendingLocalPath = localPath;
        m_pendingUploadName = name;
        refreshAccessToken();
        return;
    }

    QFile* file = new QFile(localPath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit fileUploadFailed(tr("Cannot open local file: %1").arg(localPath));
        delete file;
        return;
    }

    QString fileName = name.isEmpty() ? QFileInfo(localPath).fileName() : name;

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QJsonObject metadata;
    metadata["name"] = fileName;
    metadata["mimeType"] = "application/x-keepass2";

    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"metadata\"");
    metadataPart.setBody(QJsonDocument(metadata).toJson());

    QHttpPart mediaPart;
    mediaPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    mediaPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"media\"; filename=\"%1\"").arg(fileName));
    mediaPart.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(metadataPart);
    multiPart->append(mediaPart);

    QUrlQuery params;
    params.addQueryItem("uploadType", "multipart");
    QUrl url(QString(DriveUploadUrl) + "/" + fileId);
    url.setQuery(params);

    QNetworkRequest req = makeRequest(url.toString());
    m_uploadReply = m_net->sendCustomRequest(req, "PATCH", multiPart);
    multiPart->setParent(m_uploadReply);

    connect(m_uploadReply, &QNetworkReply::finished, this, &GoogleDriveService::onUploadFinished);
}

void GoogleDriveService::onUploadFinished()
{
    auto reply = m_uploadReply;
    m_uploadReply = nullptr;

    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errBody = QString::fromUtf8(reply->readAll());
        emit fileUploadFailed(errBody.isEmpty() ? reply->errorString() : errBody);
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        emit fileUploaded(obj.value("id").toString(), obj.value("name").toString());
    }

    reply->deleteLater();
}

void GoogleDriveService::createFolder(const QString& parentId, const QString& name)
{
    m_pendingParentId = parentId;
    m_pendingCreateFolderName = name;

    if (!ensureToken()) {
        m_pendingOp = PendingOp::CreateFolder;
        refreshAccessToken();
        return;
    }

    QJsonObject metadata;
    metadata["name"] = name;
    metadata["mimeType"] = "application/vnd.google-apps.folder";
    if (!parentId.isEmpty()) {
        QJsonArray parents;
        parents.append(parentId);
        metadata["parents"] = parents;
    }

    QJsonDocument doc(metadata);
    QNetworkRequest folderReq = makeRequest(DriveApiUrl);
    folderReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_createFolderReply = m_net->post(folderReq, doc.toJson());
    connect(m_createFolderReply, &QNetworkReply::finished, this, &GoogleDriveService::onCreateFolderFinished);
}

void GoogleDriveService::onCreateFolderFinished()
{
    auto reply = m_createFolderReply;
    m_createFolderReply = nullptr;

    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errBody = QString::fromUtf8(reply->readAll());
        emit folderCreateFailed(errBody.isEmpty() ? reply->errorString() : errBody);
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        emit folderCreated(obj.value("id").toString(), obj.value("name").toString());
    }

    reply->deleteLater();
}

void GoogleDriveService::deleteFile(const QString& fileId)
{
    m_pendingFileId = fileId;

    if (!ensureToken()) {
        m_pendingOp = PendingOp::Delete;
        refreshAccessToken();
        return;
    }

    QNetworkReply* reply = m_net->deleteResource(makeRequest(QString(DriveApiUrl) + "/" + fileId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileId] {
        if (reply->error() != QNetworkReply::NoError) {
            emit fileDeleteFailed(fileId, reply->errorString());
        } else {
            emit fileDeleted(fileId);
        }
        reply->deleteLater();
    });
}
