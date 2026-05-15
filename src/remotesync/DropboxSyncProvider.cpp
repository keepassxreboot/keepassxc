/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "DropboxSyncProvider.h"

#include "HttpRetryHelper.h"
#include "RemoteSyncParams.h"

#include "core/Clock.h"
#include "gui/remote/RemoteSettings.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QUrl>

DropboxSyncProvider::DropboxSyncProvider(QObject* parent)
    : RemoteSyncProvider(parent)
{
    m_abortFlag.storeRelease(0);
}

DropboxSyncProvider::~DropboxSyncProvider() = default;

void DropboxSyncProvider::setNetworkAccessManager(QNetworkAccessManager* nam)
{
    // Caller retains ownership of the injected NAM (see header doc). Do not
    // delete or reparent -- both would violate the contract and risk
    // double-free / use-after-free if the caller owns the mock on its stack
    // or swaps it for another instance. Internal NAMs created via
    // `new QNetworkAccessManager(this)` in ensureNam() are still cleaned up
    // by Qt parent-child on destruction of this object.
    m_nam = nam;
}

void DropboxSyncProvider::ensureNam()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

RemoteHandler::RemoteResult DropboxSyncProvider::download(const RemoteSyncParams* params)
{
    // Safe: the factory and buildParamsFromConfig pair providers and params
    // by type, so a DropboxSyncProvider only ever receives DropboxSyncParams.
    // Same applies to upload / refreshAuth / applyRefreshedTokens below.
    auto* dpxParams = static_cast<const DropboxSyncParams*>(params);

    // Validate remote path
    if (!dpxParams->remotePath.startsWith(QLatin1Char('/'))) {
        return {false, tr("Remote path must start with '/'"), {}, {}, {}};
    }

    // Lazy-init QNAM if not injected (own instance for clean thread affinity)
    ensureNam();

    // Clear stale rev so a failed download doesn't leave an outdated rev for upload
    m_lastRev.clear();

    m_abortFlag.storeRelease(0);

    const int timeoutMs = dpxParams->timeoutMsec;
    QByteArray authHeader = QByteArray("Bearer ") + dpxParams->accessToken.toUtf8();

    // Build Dropbox-API-Arg header JSON (QJsonDocument handles non-ASCII path encoding)
    QJsonObject apiArg;
    apiArg[QStringLiteral("path")] = dpxParams->remotePath;
    const QByteArray apiArgJson = QJsonDocument(apiArg).toJson(QJsonDocument::Compact);

    // Capture QNAM pointer for the lambda (m_nam is stable across the call)
    QNetworkAccessManager* nam = m_nam;

    auto makeRequest = [this, nam, &authHeader, &apiArgJson]() -> QNetworkReply* {
        QNetworkRequest request(QUrl(QStringLiteral("https://content.dropboxapi.com/2/files/download")));
        request.setRawHeader("Authorization", authHeader);
        request.setRawHeader("Dropbox-API-Arg", apiArgJson);
        // Do NOT set Content-Type -- Dropbox /2/files/download rejects
        // requests that include it.

        QNetworkReply* reply = nam->post(request, QByteArray());
        {
            QMutexLocker locker(&m_replyMutex);
            m_activeReply = reply;
        }
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, timeoutMs, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }

    // Zero the auth header now that all requests are complete
    authHeader.fill('\0');

    if (!reply) {
        return {false, tr("Network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    // Pure network errors (no HTTP status received at all).
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        qWarning("[DPX] download: network error: %s", qPrintable(errorMsg));
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}, ErrorKind::Network};
    }

    if (httpStatus == HttpConflict) {
        // Endpoint-specific error -- parse error_summary
        QByteArray body = reply->readAll();
        QJsonDocument errDoc = QJsonDocument::fromJson(body);
        QString errorSummary = errDoc.object()[QStringLiteral("error_summary")].toString();
        reply->deleteLater();

        if (errorSummary.startsWith(QStringLiteral("path/not_found"))) {
            // File doesn't exist on remote -- not an error for first sync
            return {true, {}, {}, {}, {}};
        }

        return {false, tr("Dropbox error: %1").arg(errorSummary), {}, {}, {}, ErrorKind::NotFound};
    }

    if (httpStatus != HttpOk) {
        reply->readAll(); // drain body to free network resources
        reply->deleteLater();
        // Map a few key statuses to ErrorKind; others stay Other.
        ErrorKind kind = ErrorKind::Other;
        if (httpStatus == 401) {
            kind = ErrorKind::AuthExpired;
        } else if (httpStatus == 403) {
            kind = ErrorKind::Permission;
        } else if (httpStatus == 429) {
            kind = ErrorKind::RateLimit;
        } else if (httpStatus >= 500 && httpStatus < 600) {
            kind = ErrorKind::ServerError;
        }
        return {false, tr("Dropbox API error (HTTP %1)").arg(httpStatus), {}, {}, {}, kind};
    }

    // Success (HTTP 200) -- extract rev from Dropbox-API-Result header
    QByteArray resultHeader = reply->rawHeader("Dropbox-API-Result");
    QJsonDocument metaDoc = QJsonDocument::fromJson(resultHeader);
    if (!metaDoc.isNull() && metaDoc.isObject()) {
        m_lastRev = metaDoc.object()[QStringLiteral("rev")].toString();
    }

    // Check Content-Length before reading to avoid OOM on malicious responses
    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (contentLength > MaxDatabaseSize) {
        reply->deleteLater();
        return {false, tr("Downloaded file exceeds size limit (%1 bytes)").arg(contentLength), {}, {}, {}};
    }

    QByteArray fileData = reply->readAll();
    reply->deleteLater();

    // Trust boundary: also check actual size (Content-Length may be absent or wrong).
    if (fileData.size() > MaxDatabaseSize) {
        return {false, tr("Downloaded file exceeds size limit (%1 bytes)").arg(fileData.size()), {}, {}, {}};
    }

    // Write file content to a temporary file
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open()) {
        return {false, tr("Failed to create temporary file"), {}, {}, {}};
    }

    if (tmpFile.write(fileData) != fileData.size()) {
        tmpFile.remove();
        return {false, tr("Failed to write temporary file"), {}, {}, {}};
    }
    tmpFile.close();

    return {true, {}, tmpFile.fileName(), {}, {}};
}

RemoteHandler::RemoteResult DropboxSyncProvider::upload(const QString& filePath, const RemoteSyncParams* params)
{
    auto* dpxParams = static_cast<const DropboxSyncParams*>(params);

    // Validate remote path
    if (!dpxParams->remotePath.startsWith(QLatin1Char('/'))) {
        return {false, tr("Remote path must start with '/'"), {}, {}, {}};
    }

    // Read file content
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {false, tr("Failed to open file for upload: %1").arg(filePath), {}, {}, {}};
    }
    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.size() > MaxDatabaseSize) {
        return {false, tr("File exceeds size limit (%1 bytes)").arg(fileData.size()), {}, {}, {}};
    }

    // Lazy-init QNAM if not injected
    ensureNam();

    // Reset abort flag
    m_abortFlag.storeRelease(0);

    const int timeoutMs = dpxParams->timeoutMsec;
    QByteArray authHeader = QByteArray("Bearer ") + dpxParams->accessToken.toUtf8();

    // Build Dropbox-API-Arg header JSON
    QJsonObject apiArg;
    apiArg[QStringLiteral("path")] = dpxParams->remotePath;

    if (m_lastRev.isEmpty()) {
        // First upload (file didn't exist on remote) -- use "add" mode
        apiArg[QStringLiteral("mode")] = QStringLiteral("add");
    } else {
        // Update existing file -- "update" mode must be an object with .tag
        // and update fields; the string shorthand other modes use is rejected.
        QJsonObject mode;
        mode[QStringLiteral(".tag")] = QStringLiteral("update");
        mode[QStringLiteral("update")] = m_lastRev;
        apiArg[QStringLiteral("mode")] = mode;
    }

    apiArg[QStringLiteral("autorename")] = false; // Do NOT create conflicted copies
    apiArg[QStringLiteral("mute")] = true; // Suppress Dropbox desktop notifications

    const QByteArray apiArgJson = QJsonDocument(apiArg).toJson(QJsonDocument::Compact);

    QNetworkAccessManager* nam = m_nam;

    auto makeRequest = [this, nam, &authHeader, &apiArgJson, &fileData]() -> QNetworkReply* {
        QNetworkRequest request(QUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload")));
        request.setRawHeader("Authorization", authHeader);
        request.setRawHeader("Dropbox-API-Arg", apiArgJson);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));

        QNetworkReply* reply = nam->post(request, fileData);
        {
            QMutexLocker locker(&m_replyMutex);
            m_activeReply = reply;
        }
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, timeoutMs, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }

    // Zero the auth header now that all requests are complete
    authHeader.fill('\0');

    if (!reply) {
        return {false, tr("Network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    // Pure network errors (no HTTP status received at all).
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        qWarning("[DPX] upload: network error: %s", qPrintable(errorMsg));
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}, ErrorKind::Network};
    }

    if (httpStatus == HttpConflict) {
        // Endpoint-specific error
        QByteArray body = reply->readAll();
        QJsonDocument errDoc = QJsonDocument::fromJson(body);
        QString errorSummary = errDoc.object()[QStringLiteral("error_summary")].toString();
        reply->deleteLater();

        if (errorSummary.startsWith(QStringLiteral("path/conflict"))) {
            return {false,
                    tr("Remote file changed since last download. Re-sync to merge changes."),
                    {},
                    {},
                    {},
                    ErrorKind::Conflict};
        }

        return {false, tr("Dropbox error: %1").arg(errorSummary), {}, {}, {}, ErrorKind::Other};
    }

    if (httpStatus != HttpOk) {
        reply->readAll(); // drain body to free network resources
        reply->deleteLater();
        ErrorKind kind = ErrorKind::Other;
        if (httpStatus == 401) {
            kind = ErrorKind::AuthExpired;
        } else if (httpStatus == 403) {
            kind = ErrorKind::Permission;
        } else if (httpStatus == 429) {
            kind = ErrorKind::RateLimit;
        } else if (httpStatus == 507) {
            kind = ErrorKind::Quota;
        } else if (httpStatus >= 500 && httpStatus < 600) {
            kind = ErrorKind::ServerError;
        }
        return {false, tr("Dropbox API error (HTTP %1)").arg(httpStatus), {}, {}, {}, kind};
    }

    // Success -- update m_lastRev from response body so subsequent uploads
    // in the same session use the rev the server now holds.
    QJsonDocument respDoc = QJsonDocument::fromJson(reply->readAll());
    if (!respDoc.isNull() && respDoc.isObject()) {
        QString newRev = respDoc.object()[QStringLiteral("rev")].toString();
        if (!newRev.isEmpty()) {
            m_lastRev = newRev;
        }
    }

    reply->deleteLater();
    return {true, {}, {}, {}, {}};
}

// ---------------------------------------------------------------------------
// refreshAuth -- proactive token refresh via Dropbox /oauth2/token
// ---------------------------------------------------------------------------

RemoteHandler::RemoteResult DropboxSyncProvider::refreshAuth(const RemoteSyncParams* params)
{
    auto* dpxParams = static_cast<const DropboxSyncParams*>(params);

    if (dpxParams->refreshToken.isEmpty()) {
        return {false, tr("No refresh token. Re-authorize in Settings."), {}, {}, {}, ErrorKind::AuthRevoked};
    }

    // Proactive check: if token still valid with buffer, skip refresh
    if (dpxParams->expiresAt.isValid()
        && Clock::currentDateTimeUtc().addSecs(TokenRefreshBufferSecs) < dpxParams->expiresAt) {
        return {true, {}, {}, {}, {}};
    }

    ensureNam();
    m_abortFlag.storeRelease(0);

    const int timeoutMs = dpxParams->timeoutMsec;

    // Build POST body for refresh_token grant
    QByteArray postBody;
    postBody.append("grant_type=refresh_token");
    postBody.append("&refresh_token=");
    postBody.append(QUrl::toPercentEncoding(dpxParams->refreshToken));
    postBody.append("&client_id=");
    postBody.append(QUrl::toPercentEncoding(dpxParams->appKey));

    QNetworkAccessManager* nam = m_nam;

    auto makeRequest = [this, nam, &postBody]() -> QNetworkReply* {
        QNetworkRequest request(QUrl(QStringLiteral("https://api.dropboxapi.com/oauth2/token")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

        QNetworkReply* reply = nam->post(request, postBody);
        {
            QMutexLocker locker(&m_replyMutex);
            m_activeReply = reply;
        }
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, timeoutMs, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }

    // Zero the POST body (contains refresh token)
    postBody.fill('\0');

    if (!reply) {
        return {false, tr("Token refresh failed: network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        QString errorMsg = tr("Token refresh failed: %1").arg(reply->errorString());
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}, ErrorKind::Network};
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument respDoc = QJsonDocument::fromJson(responseData);
    QJsonObject respObj = respDoc.object();

    if (httpStatus != HttpOk) {
        // Do not log the response body -- it can contain sensitive OAuth
        // fields. Status code + length is enough for diagnostics.
        qWarning("[DPX] refreshAuth: error (status %d, body length %d)", httpStatus, responseData.length());
        // Check for invalid_grant (refresh token revoked/expired)
        QString errorTag = respObj[QStringLiteral("error")].toString();
        if (errorTag == QStringLiteral("invalid_grant")) {
            return {false,
                    tr("Refresh token expired or revoked. Re-authorize in Settings."),
                    {},
                    {},
                    {},
                    ErrorKind::AuthRevoked};
        }
        QString errorDesc = respObj[QStringLiteral("error_description")].toString();
        return {false,
                tr("Token refresh failed: %1").arg(errorDesc.isEmpty() ? errorTag : errorDesc),
                {},
                {},
                {},
                ErrorKind::AuthExpired};
    }

    // Parse successful response
    QString newAccessToken = respObj[QStringLiteral("access_token")].toString();
    int expiresIn = respObj[QStringLiteral("expires_in")].toInt();

    if (newAccessToken.isEmpty()) {
        return {false, tr("Token refresh failed: missing access_token in response"), {}, {}, {}};
    }

    // Compute new expiry time
    QDateTime newExpiresAt = Clock::currentDateTimeUtc().addSecs(expiresIn);

    // CRITICAL: Do NOT read refresh_token from response -- Dropbox does not
    // return it on refresh. Keep existing refreshToken unchanged.

    // Build JSON output for caller to persist
    QJsonObject tokenData;
    tokenData[QStringLiteral("accessToken")] = newAccessToken;
    tokenData[QStringLiteral("expiresAt")] = newExpiresAt.toMSecsSinceEpoch();
    QString tokenJson = QString::fromUtf8(QJsonDocument(tokenData).toJson(QJsonDocument::Compact));

    // Zero sensitive data
    responseData.fill('\0');

    return {true, {}, {}, tokenJson, {}};
}

// ---------------------------------------------------------------------------
// revokeToken -- best-effort token revocation
// ---------------------------------------------------------------------------

RemoteHandler::RemoteResult DropboxSyncProvider::revokeToken(const DropboxSyncParams* params)
{
    // Internal wiring: callers always pass a constructed DropboxSyncParams;
    // a null here is a wiring bug rather than a user-facing failure.
    Q_ASSERT(params);

    if (params->accessToken.isEmpty()) {
        return {true, {}, {}, {}, {}};
    }

    ensureNam();
    m_abortFlag.storeRelease(0);

    QByteArray authHeader = QByteArray("Bearer ") + params->accessToken.toUtf8();
    QNetworkAccessManager* nam = m_nam;

    auto makeRequest = [this, nam, &authHeader]() -> QNetworkReply* {
        QNetworkRequest request(QUrl(QStringLiteral("https://api.dropboxapi.com/2/auth/token/revoke")));
        request.setRawHeader("Authorization", authHeader);
        // Revoke endpoint requires no body, but POST must have content-type
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        QNetworkReply* reply = nam->post(request, QByteArray());
        {
            QMutexLocker locker(&m_replyMutex);
            m_activeReply = reply;
        }
        return reply;
    };

    // Short timeout (10s) for revocation -- best-effort
    RetryPolicy policy;
    policy.maxRetries = 1; // Don't retry aggressively for revocation
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 10000, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }

    // Zero the auth header
    authHeader.fill('\0');

    if (reply) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus != HttpOk) {
            qWarning("DropboxSyncProvider: token revocation returned HTTP %d (best-effort, ignoring)", httpStatus);
        }
        reply->deleteLater();
    } else {
        qWarning("DropboxSyncProvider: token revocation network request failed (best-effort, ignoring)");
    }

    // Best-effort: always return success. Caller clears local tokens regardless.
    return {true, {}, {}, {}, {}};
}

void DropboxSyncProvider::abort()
{
    m_abortFlag.storeRelease(1);

    QMutexLocker locker(&m_replyMutex);
    if (m_activeReply) {
        // Marshal abort() to the reply's owning thread for thread safety.
        // Cancelling an in-progress browser-auth flow is the page-side's
        // responsibility via DropboxLoginFlow::cancel.
        QMetaObject::invokeMethod(m_activeReply, "abort", Qt::QueuedConnection);
    }
}

// ---------------------------------------------------------------------------
// RemoteSyncProvider abstraction overrides.
// ---------------------------------------------------------------------------

QString DropboxSyncProvider::displayName() const
{
    // Untranslated identifier; UI applies tr() at call site.
    return QStringLiteral("Dropbox");
}

RemoteSyncParams* DropboxSyncProvider::createParams() const
{
    // Caller takes ownership.
    auto* params = new DropboxSyncParams();
    params->type = QStringLiteral("dropbox");
    return params;
}

RemoteSyncParams* DropboxSyncProvider::buildParamsFromConfig(const QJsonObject& config) const
{
    // Caller takes ownership.
    auto* params = new DropboxSyncParams();
    params->type = QStringLiteral("dropbox");
    params->name = config[QStringLiteral("name")].toString();
    params->appKey = config[QStringLiteral("appKey")].toString();
    params->remotePath = config[QStringLiteral("remotePath")].toString();
    params->accessToken = config[QStringLiteral("accessToken")].toString();
    params->refreshToken = config[QStringLiteral("refreshToken")].toString();
    if (config.contains(QStringLiteral("expiresAt"))) {
        params->expiresAt =
            QDateTime::fromMSecsSinceEpoch(config[QStringLiteral("expiresAt")].toVariant().toLongLong());
    }
    return params;
}

bool DropboxSyncProvider::applyRefreshedTokens(const QString& stdOutput, RemoteSyncParams* params)
{
    // Empty stdOutput means refreshAuth had no token data to apply (e.g. token
    // still valid; proactive refresh skipped). Treat as success no-op.
    if (stdOutput.isEmpty()) {
        return true;
    }

    QJsonDocument doc = QJsonDocument::fromJson(stdOutput.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        // Hard-fail; engine surfaces auth-failure banner and the user
        // re-authorizes.
        qWarning("DropboxSyncProvider: failed to parse refreshed token JSON");
        return false;
    }

    auto* dpxParams = static_cast<DropboxSyncParams*>(params);

    QJsonObject tokenData = doc.object();
    if (tokenData.contains(QStringLiteral("accessToken"))) {
        dpxParams->accessToken = tokenData[QStringLiteral("accessToken")].toString();
    }
    if (tokenData.contains(QStringLiteral("expiresAt"))) {
        dpxParams->expiresAt =
            QDateTime::fromMSecsSinceEpoch(tokenData[QStringLiteral("expiresAt")].toVariant().toLongLong());
    }
    return true;
}

RemoteSyncProvider::ErrorKind DropboxSyncProvider::classifyError(const QString& errorMessage) const
{
    // invalid_grant maps to AuthRevoked (refresh token revoked or expired).
    // The other two map to AuthExpired (short-lived access-token expiry,
    // recoverable via refresh_token grant).
    if (errorMessage.contains(QStringLiteral("invalid_access_token"), Qt::CaseInsensitive)
        || errorMessage.contains(QStringLiteral("expired_access_token"), Qt::CaseInsensitive)) {
        return ErrorKind::AuthExpired;
    }
    if (errorMessage.contains(QStringLiteral("invalid_grant"), Qt::CaseInsensitive)) {
        return ErrorKind::AuthRevoked;
    }
    return ErrorKind::Other;
}

bool DropboxSyncProvider::isAuthorized(const QJsonObject& config) const
{
    // Operational contract: a config is "authorized" only if it has every
    // field required for a successful sync round-trip.
    //   - accessToken: short-lived bearer used by download/upload
    //   - refreshToken: required by refreshAuth to mint a new accessToken
    //     (without it an expired/restarted session can never recover)
    //   - appKey: client_id used by refreshAuth
    //   - remotePath: target path on Dropbox; sync has no usable default
    return !config.value(QStringLiteral("accessToken")).toString().isEmpty()
           && !config.value(QStringLiteral("refreshToken")).toString().isEmpty()
           && !config.value(QStringLiteral("appKey")).toString().isEmpty()
           && !config.value(QStringLiteral("remotePath")).toString().isEmpty();
}

void DropboxSyncProvider::persistRefreshedTokens(const QString& stdOutput, RemoteSettings* settings) const
{
    if (!settings) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(stdOutput.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning("DropboxSyncProvider: failed to parse refreshed token JSON for persist");
        return;
    }

    QJsonObject config = settings->cloudSyncConfig();
    if (config.value(QStringLiteral("type")).toString() != QStringLiteral("dropbox")) {
        qWarning("DropboxSyncProvider: stored cloud config is not Dropbox; skipping token persist");
        return;
    }

    // Update only the fields that refreshAuth returns (accessToken, expiresAt).
    // Do NOT overwrite refreshToken -- Dropbox refresh response has no refresh_token field.
    QJsonObject tokenData = doc.object();
    if (tokenData.contains(QStringLiteral("accessToken"))) {
        config[QStringLiteral("accessToken")] = tokenData[QStringLiteral("accessToken")].toString();
    }
    if (tokenData.contains(QStringLiteral("expiresAt"))) {
        config[QStringLiteral("expiresAt")] = tokenData[QStringLiteral("expiresAt")];
    }

    settings->setCloudSyncConfig(config);
    settings->saveSettings();
}
