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

#include "NextcloudSyncProvider.h"

#include "HttpRetryHelper.h"
#include "RemoteSyncParams.h"
#include "config-keepassx.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QThread>
#include <QUrl>

NextcloudSyncProvider::NextcloudSyncProvider(QObject* parent)
    : RemoteSyncProvider(parent)
{
    setObjectName(QStringLiteral("NextcloudSyncProvider"));
    m_abortFlag.storeRelease(0);
}

NextcloudSyncProvider::~NextcloudSyncProvider() = default;

void NextcloudSyncProvider::setNetworkAccessManager(QNetworkAccessManager* nam)
{
    // Caller-owned; see NextcloudSyncProvider.h.
    m_nam = nam;
}

void NextcloudSyncProvider::ensureNam()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

QString NextcloudSyncProvider::displayName() const
{
    return QStringLiteral("Nextcloud");
}

RemoteSyncParams* NextcloudSyncProvider::createParams() const
{
    auto* p = new NextcloudSyncParams;
    p->type = QStringLiteral("nextcloud");
    return p;
}

RemoteSyncParams* NextcloudSyncProvider::buildParamsFromConfig(const QJsonObject& config) const
{
    auto* base = createParams();
    auto* p = static_cast<NextcloudSyncParams*>(base);
    p->serverBaseUrl = config.value(QStringLiteral("serverBaseUrl")).toString();
    p->remotePath = config.value(QStringLiteral("remotePath")).toString();
    p->loginName = config.value(QStringLiteral("loginName")).toString();
    p->appPassword = config.value(QStringLiteral("appPassword")).toString();
    p->timeoutMsec = config.value(QStringLiteral("timeoutMsec")).toInt(30000);
    return p;
}

// ---------------------------------------------------------------------------
// download() WebDAV GET with Basic auth + User-Agent + ETag capture.
// No qDebug/qWarning prints of m_lastETag, appPassword, loginName, or the
// Authorization header value (no-secrets-in-logs).
// Do NOT call QNetworkReply::ignoreSslErrors here or anywhere in this
// provider. SSL handshake failures surface to the user as an actionable
// banner; the provider assumes a publicly-trusted CA.
// ---------------------------------------------------------------------------
RemoteHandler::RemoteResult NextcloudSyncProvider::download(const RemoteSyncParams* params)
{
    // Public-method body is a single-line wrapper around
    // retryOnAuthOnce(downloadImpl). The abort-flag reset is hoisted here
    // (BEFORE retryOnAuthOnce) so that an abort() called between the
    // first-attempt's post-execute check and the helper's backoff-wake cannot
    // be swallowed by a downloadImpl re-entry resetting the flag.
    m_abortFlag.storeRelease(0);
    return retryOnAuthOnce([this, params]() { return downloadImpl(params); });
}

RemoteHandler::RemoteResult NextcloudSyncProvider::downloadImpl(const RemoteSyncParams* params)
{
    // Safe: the factory and buildParamsFromConfig pair providers and params
    // by type, so a NextcloudSyncProvider only ever receives
    // NextcloudSyncParams. Same applies to uploadImpl below.
    auto* ncParams = static_cast<const NextcloudSyncParams*>(params);

    // Validate required fields. remotePath must start with '/' (NFC-normalized at
    // save time; we do not re-normalize here).
    if (ncParams->serverBaseUrl.isEmpty()) {
        return {false, tr("Nextcloud server URL is required"), {}, {}, {}};
    }
    if (ncParams->loginName.isEmpty()) {
        return {false, tr("Nextcloud login name is required"), {}, {}, {}};
    }
    if (ncParams->remotePath.isEmpty() || !ncParams->remotePath.startsWith(QLatin1Char('/'))) {
        return {false, tr("Remote path must start with '/'"), {}, {}, {}};
    }

    ensureNam();

    // Clear stale ETag so a failed download doesn't leak into the next upload.
    // m_abortFlag is reset in download()'s public wrapper, not here.
    m_lastETag.clear();

    const QString canonicalBase = canonicalizeServerBaseUrl(ncParams->serverBaseUrl);
    const QUrl resourceUrl = buildResourceUrl(canonicalBase, ncParams->loginName, ncParams->remotePath);

    // Basic-auth: base64(loginName ":" appPassword). Both buffers zeroed before
    // return for stack hygiene (QByteArray fill('\0')).
    QByteArray basicCreds = (ncParams->loginName + QLatin1Char(':') + ncParams->appPassword).toUtf8();
    QByteArray authHeader = QByteArray("Basic ") + basicCreds.toBase64();

    auto makeRequest = [this, &resourceUrl, &authHeader]() -> QNetworkReply* {
        QNetworkRequest req(resourceUrl);
        req.setRawHeader("Authorization", authHeader);
        req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);

        QNetworkReply* reply = m_nam->get(req);
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, ncParams->timeoutMsec, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }
    authHeader.fill('\0');
    basicCreds.fill('\0');

    if (!reply) {
        return {false, tr("Network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Network-level errors (no HTTP status received). SSL handshake gets the
    // locked banner; everything else falls through to a generic
    // "Network error: <reason>" string.
    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
            reply->deleteLater();
            return {false,
                    tr("Nextcloud server's SSL certificate could not be verified. "
                       "Check that your server's certificate is valid and the chain "
                       "is correctly configured."),
                    {},
                    {},
                    {}};
        }
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}};
    }

    // 404 distinction: look up the file in trashbin to differentiate a
    // first-sync (silent success) from an in-trash situation (actionable
    // banner). The PROPFIND helper returns false on any failure, which collapses
    // both "no trashbin" and "not in trash" to first-sync semantics.
    if (httpStatus == HttpNotFound) {
        const QString filename = QFileInfo(ncParams->remotePath).fileName();
        reply->deleteLater();
        const bool inTrash = checkIfInTrash(ncParams, filename);
        // Abort during the trashbin PROPFIND must surface as cancelled, not
        // silent first-sync success. checkIfInTrash returns false on any
        // failure (including abort) so without this check an aborted trash
        // lookup would route through the first-sync branch.
        if (m_abortFlag.loadAcquire() != 0) {
            return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
        }
        if (inTrash) {
            return {false,
                    tr("Database is in your Nextcloud trash. Restore it from "
                       "Nextcloud Files, then try syncing again."),
                    {},
                    {},
                    {}};
        }
        // First-sync silent success -- same shape as Dropbox's
        // path/not_found 409 handling.
        return {true, {}, {}, {}, {}};
    }

    if (httpStatus != HttpOk) {
        // Drain body for hygiene. Per-status locked banners go through the
        // centralized mapWebdavStatusToMessage helper. 401/403/423/507 get
        // their locked banners; 5xx gets a generic server-error banner;
        // everything else gets a fallback "Nextcloud returned HTTP %1." banner.
        reply->readAll();
        QString errorMsg = mapWebdavStatusToMessage(httpStatus);
        ErrorKind kind = mapWebdavStatusToKind(httpStatus);
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}, kind};
    }

    // 200 success -- ETag Lifecycle State Machine.
    // Preserve opaque-tag quotes byte-for-byte.
    // OC-Etag fallback for older Nextcloud / external-storage backends
    // (nextcloud/server#14103).
    // Weak detection sets sticky m_serverEmitsWeakETags AND emits
    // weakEtagDetected() exactly once per provider lifetime. The flag is
    // sticky -- once set, no subsequent strong ETag clears it (protects
    // against mid-session server "fixes" that can't be trusted to remain
    // stable).
    QByteArray etagRaw = reply->rawHeader("ETag");
    if (etagRaw.isEmpty()) {
        etagRaw = reply->rawHeader("OC-Etag");
    }
    QString etag = QString::fromLatin1(etagRaw);

    if (etag.startsWith(QLatin1String("W/"))) {
        // Weak ETag -- skip storage; mark session; emit warning ONCE.
        // m_lastETag was reset at function entry; leave it cleared so the
        // next upload uses silent-overwrite semantics (NEITHER conditional
        // header) per RFC 7232 §2.3.
        m_lastETag.clear();
        if (!m_serverEmitsWeakETags) {
            m_serverEmitsWeakETags = true;
            emit weakEtagDetected();
        }
    } else if (!etag.isEmpty() && etag != QLatin1String("\"\"")) {
        // Strong ETag -- store verbatim (quotes preserved byte-for-byte).
        // Empty-quoted ETag ("") collapses to "no useful ETag" so we leave
        // m_lastETag empty (-> first-upload semantics).
        m_lastETag = etag;
    }
    // else: m_lastETag stays empty (server emitted neither header, or only an
    // empty-quoted one) -- forms first-upload semantics for the next upload.

    // Body length sanity check before reading (avoids OOM on malicious responses).
    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (contentLength > MaxDatabaseSize) {
        reply->deleteLater();
        return {false, tr("Downloaded file exceeds size limit (%1 bytes)").arg(contentLength), {}, {}, {}};
    }

    QByteArray fileData = reply->readAll();
    reply->deleteLater();

    // Trust boundary: actual size check (Content-Length may be absent).
    if (fileData.size() > MaxDatabaseSize) {
        return {false, tr("Downloaded file exceeds size limit (%1 bytes)").arg(fileData.size()), {}, {}, {}};
    }

    // Write to a QTemporaryFile (caller owns the path; setAutoRemove(false) so
    // SyncEngine can read it after this function returns). Mirrors Dropbox.
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

// ---------------------------------------------------------------------------
// Minimal PROPFIND request body shared by testConnection() and checkIfInTrash().
// Some reverse-proxy configurations reject PROPFIND with an empty body, so we
// always send this canonical XML body. The body requests only <d:getetag/>
// (the cheapest property to compute on the server).
// ---------------------------------------------------------------------------
static const QByteArray propfindBody = "<?xml version=\"1.0\"?>\n"
                                       "<d:propfind xmlns:d=\"DAV:\">\n"
                                       "  <d:prop><d:getetag/></d:prop>\n"
                                       "</d:propfind>\n";

// ---------------------------------------------------------------------------
// testConnection() -- PROPFIND Depth: 0 against the configured remote path.
// Treats 200 OR 207 (Multi-Status) as success; non-success routes through
// mapWebdavStatusToMessage for per-status locked banners.
// ---------------------------------------------------------------------------
RemoteHandler::RemoteResult NextcloudSyncProvider::testConnection(const NextcloudSyncParams* params)
{
    // Public-method body is a single-line wrapper around
    // retryOnAuthOnce(testConnectionImpl). Abort flag reset is performed here
    // (see download() above). The credential-rejection banner is emitted from
    // testConnectionImpl, not from this wrapper.
    m_abortFlag.storeRelease(0);
    return retryOnAuthOnce([this, params]() { return testConnectionImpl(params); });
}

RemoteHandler::RemoteResult NextcloudSyncProvider::testConnectionImpl(const NextcloudSyncParams* params)
{
    // Callers always pass a constructed NextcloudSyncParams; a null here
    // is a wiring bug rather than a user-facing failure.
    Q_ASSERT(params);

    if (params->serverBaseUrl.isEmpty()) {
        return {false, tr("Nextcloud server URL is required"), {}, {}, {}};
    }
    if (params->loginName.isEmpty()) {
        return {false, tr("Nextcloud login name is required"), {}, {}, {}};
    }
    if (params->remotePath.isEmpty() || !params->remotePath.startsWith(QLatin1Char('/'))) {
        return {false, tr("Remote path must start with '/'"), {}, {}, {}};
    }

    ensureNam();

    const QString canonicalBase = canonicalizeServerBaseUrl(params->serverBaseUrl);
    const QUrl resourceUrl = buildResourceUrl(canonicalBase, params->loginName, params->remotePath);

    QByteArray basicCreds = (params->loginName + QLatin1Char(':') + params->appPassword).toUtf8();
    QByteArray authHeader = QByteArray("Basic ") + basicCreds.toBase64();

    auto makeRequest = [this, &resourceUrl, &authHeader]() -> QNetworkReply* {
        QNetworkRequest req(resourceUrl);
        req.setRawHeader("Authorization", authHeader);
        req.setRawHeader("Depth", "0");
        req.setRawHeader("Content-Type", "application/xml");
        req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);

        QNetworkReply* reply = m_nam->sendCustomRequest(req, "PROPFIND", propfindBody);
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, params->timeoutMsec, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }
    authHeader.fill('\0');
    basicCreds.fill('\0');

    if (!reply) {
        return {false, tr("Network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
            reply->deleteLater();
            return {false,
                    tr("Nextcloud server's SSL certificate could not be verified. "
                       "Check that your server's certificate is valid and the chain "
                       "is correctly configured."),
                    {},
                    {},
                    {}};
        }
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}};
    }

    reply->readAll(); // drain body for hygiene -- we only care about the status
    reply->deleteLater();

    if (httpStatus == HttpMultiStatus || httpStatus == HttpOk) {
        // 207/200 means the configured remote path exists on the server. Populate
        // filePath with the remote path so the page-side caller can dispatch to
        // "Nextcloud connection successful." (file exists) vs "Connected. File
        // not found -- it will be created on first sync." (404 branch below,
        // empty filePath). Mirrors Dropbox's download-as-test-connection contract
        // where filePath emptiness signals first-sync state.
        return {true, {}, params->remotePath, {}, {}};
    }

    // testConnection probes credentials the user just typed into the
    // settings widget, so the actionable 401 message is "double-check what
    // you pasted" -- not the "Re-authorize in Settings" wording that
    // download() and upload() emit for an in-session 401. Same HTTP status,
    // different user-facing string per call-site. classifyError dispatches
    // both banners to ErrorKind::AuthExpired so retryOnAuthOnce treats them
    // uniformly.
    if (httpStatus == HttpUnauthorized) {
        // reply was already drained + deleteLater()-ed above.
        return {false,
                tr("Nextcloud rejected those credentials. Verify the username and app password."),
                {},
                {},
                {},
                ErrorKind::AuthExpired};
    }

    // 404 from a PROPFIND on the configured remote path means "auth accepted +
    // server reachable + endpoint correct, but the file doesn't exist yet" --
    // i.e. first-sync semantics. Mirror DropboxSyncProvider::download's 404
    // handling: return success=true with empty filePath so the page-side caller
    // and SyncEngine first-sync branch can both treat this as "auth OK, will be
    // created on first sync."
    if (httpStatus == HttpNotFound) {
        return {true, {}, {}, {}, {}};
    }

    // Per-status locked banners via mapWebdavStatusToMessage for any other
    // non-success status (403/412/423/507/5xx/etc).
    return {false, mapWebdavStatusToMessage(httpStatus), {}, {}, {}, mapWebdavStatusToKind(httpStatus)};
}

// ---------------------------------------------------------------------------
// checkIfInTrash() -- PROPFIND Depth: 1 against the trashbin.
// Permissive contains() match on body bytes -- handles deletion-suffix format
// variance. Any failure (no trashbin, network error, non-multistatus
// response) collapses to false ("not in trash") -- which the download()
// caller interprets as a first-sync silent success.
// ---------------------------------------------------------------------------
bool NextcloudSyncProvider::checkIfInTrash(const NextcloudSyncParams* params, const QString& filename)
{
    if (filename.isEmpty()) {
        return false;
    }

    // Build trashbin URL from the canonical base, preserving any subpath
    // (e.g. https://cloud.example.com/nextcloud -> .../nextcloud/remote.php/dav/...)
    const QString canonicalBase = canonicalizeServerBaseUrl(params->serverBaseUrl);
    QUrl trashUrl(canonicalBase);
    const QString encodedLogin = QString::fromUtf8(QUrl::toPercentEncoding(params->loginName));
    QString trashPath =
        trashUrl.path() + QStringLiteral("/remote.php/dav/trashbin/") + encodedLogin + QStringLiteral("/trash");
    trashUrl.setPath(trashPath, QUrl::DecodedMode);

    QByteArray basicCreds = (params->loginName + QLatin1Char(':') + params->appPassword).toUtf8();
    QByteArray authHeader = QByteArray("Basic ") + basicCreds.toBase64();

    auto makeRequest = [this, &trashUrl, &authHeader]() -> QNetworkReply* {
        QNetworkRequest req(trashUrl);
        req.setRawHeader("Authorization", authHeader);
        req.setRawHeader("Depth", "1");
        req.setRawHeader("Content-Type", "application/xml");
        req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);

        QNetworkReply* reply = m_nam->sendCustomRequest(req, "PROPFIND", propfindBody);
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, params->timeoutMsec, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }
    authHeader.fill('\0');
    basicCreds.fill('\0');

    if (!reply) {
        return false;
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus != HttpMultiStatus && httpStatus != HttpOk) {
        // 404 (no trashbin endpoint), 401 (auth issue), 5xx (server error) all
        // collapse to false -- "not in trash" / first-sync semantics.
        reply->readAll();
        reply->deleteLater();
        return false;
    }

    QByteArray body = reply->readAll();
    reply->deleteLater();

    // Permissive parse: look for the filename anywhere in the response body.
    // Nextcloud appends a deletion-timestamp suffix (e.g. ".d1234567890") to
    // the filename in <d:href>; the contains() match works across suffix
    // variants. The filename itself is always present byte-for-byte before
    // the suffix.
    return body.contains(filename.toUtf8());
}

// ---------------------------------------------------------------------------
// upload() WebDAV PUT with three-way conditional header selection + ETag
// round-trip + 412 conflict surface.
// No qDebug/qWarning prints of m_lastETag, appPassword, loginName, fileData,
// or the Authorization header value (no-secrets-in-logs).
// Do NOT call QNetworkReply::ignoreSslErrors here or anywhere in this
// provider. SSL handshake failures surface to the user as an actionable
// banner; the provider assumes a publicly-trusted CA.
//
// Three-way conditional-header tree (RFC 7232 §2.3):
//   m_serverEmitsWeakETags=true        -> NEITHER (silent overwrite)
//   m_lastETag empty                   -> If-None-Match: *  (first upload)
//   m_lastETag non-empty               -> If-Match: <verbatim>  (update)
// ---------------------------------------------------------------------------
RemoteHandler::RemoteResult NextcloudSyncProvider::upload(const QString& filePath, const RemoteSyncParams* params)
{
    // Public-method body is a single-line wrapper around
    // retryOnAuthOnce(uploadImpl). Abort flag reset hoisted here (see
    // download() comment).
    m_abortFlag.storeRelease(0);
    return retryOnAuthOnce([this, &filePath, params]() { return uploadImpl(filePath, params); });
}

RemoteHandler::RemoteResult NextcloudSyncProvider::uploadImpl(const QString& filePath, const RemoteSyncParams* params)
{
    auto* ncParams = static_cast<const NextcloudSyncParams*>(params);

    if (ncParams->serverBaseUrl.isEmpty()) {
        return {false, tr("Nextcloud server URL is required"), {}, {}, {}};
    }
    if (ncParams->loginName.isEmpty()) {
        return {false, tr("Nextcloud login name is required"), {}, {}, {}};
    }
    if (ncParams->remotePath.isEmpty() || !ncParams->remotePath.startsWith(QLatin1Char('/'))) {
        return {false, tr("Remote path must start with '/'"), {}, {}, {}};
    }
    if (filePath.isEmpty()) {
        return {false, tr("Local file path is required"), {}, {}, {}};
    }

    // Read file contents into memory. Body is sent in a single PUT (no
    // chunked-upload split -- Nextcloud 16+ accepts up to 512 MiB by
    // default; we cap at MaxDatabaseSize=256 MiB which is well under).
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {false, tr("Failed to open file for upload: %1").arg(filePath), {}, {}, {}};
    }
    if (file.size() > MaxDatabaseSize) {
        return {false, tr("File exceeds size limit (%1 bytes)").arg(file.size()), {}, {}, {}};
    }
    QByteArray fileData = file.readAll();
    file.close();

    ensureNam();

    const QString canonicalBase = canonicalizeServerBaseUrl(ncParams->serverBaseUrl);
    const QUrl resourceUrl = buildResourceUrl(canonicalBase, ncParams->loginName, ncParams->remotePath);

    QByteArray basicCreds = (ncParams->loginName + QLatin1Char(':') + ncParams->appPassword).toUtf8();
    QByteArray authHeader = QByteArray("Basic ") + basicCreds.toBase64();

    // Snapshot the conditional-header decision BEFORE the network call. The
    // m_lastETag and m_serverEmitsWeakETags fields are stable across the
    // single PUT (we don't run any parallel downloads), but capturing the
    // values into locals makes the lambda's intent explicit.
    const bool weakSession = m_serverEmitsWeakETags;
    const QByteArray lastEtagBytes = m_lastETag.toLatin1();
    const bool firstUpload = lastEtagBytes.isEmpty();

    auto makeRequest =
        [this, &resourceUrl, &authHeader, &fileData, weakSession, firstUpload, &lastEtagBytes]() -> QNetworkReply* {
        QNetworkRequest req(resourceUrl);
        req.setRawHeader("Authorization", authHeader);
        req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));

        // Three-way conditional-header tree -- mutually exclusive per RFC 7232.
        if (weakSession) {
            // Silent overwrite: NEITHER conditional header. Sending
            // If-Match: W/"..." would be an RFC 7232 §2.3 violation; sending
            // If-None-Match: * after we already know the file exists would
            // make every upload fail. The session flag is the only sound
            // option once the server is known to emit weak ETags.
        } else if (firstUpload) {
            // First-upload: file MUST NOT exist on remote.
            req.setRawHeader("If-None-Match", "*");
        } else {
            // Update: ETag MUST match prior download verbatim (quotes
            // preserved byte-for-byte).
            req.setRawHeader("If-Match", lastEtagBytes);
        }

        QNetworkReply* reply = m_nam->put(req, fileData);
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
        return reply;
    };

    RetryPolicy policy;
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, ncParams->timeoutMsec, &m_abortFlag);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = nullptr;
    }
    authHeader.fill('\0');
    basicCreds.fill('\0');

    if (!reply) {
        return {false, tr("Network request failed"), {}, {}, {}, ErrorKind::Network};
    }

    if (m_abortFlag.loadAcquire() != 0) {
        reply->deleteLater();
        return {false, tr("Operation cancelled"), {}, {}, {}, ErrorKind::Aborted};
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Network-level errors (no HTTP status received). SSL handshake gets the
    // locked banner; everything else falls through to a generic
    // "Network error: <reason>" string.
    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
            reply->deleteLater();
            return {false,
                    tr("Nextcloud server's SSL certificate could not be verified. "
                       "Check that your server's certificate is valid and the chain "
                       "is correctly configured."),
                    {},
                    {},
                    {}};
        }
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}};
    }

    // 412 Precondition Failed: the conditional header didn't match. This is
    // the conflict surface. Banner string is verbatim from Dropbox's
    // path/conflict branch -- cross-provider consistency means users learn
    // the conflict semantic once.
    //
    // m_lastETag is NOT cleared here: the captured ETag was still VALID at
    // the time of capture; the conflict is about a server-side concurrent
    // change, not our ETag becoming stale. The next download will refresh
    // m_lastETag naturally; clearing it now would force an unnecessary
    // first-upload retry that 409s against the now-existing file.
    if (httpStatus == HttpPreconditionFailed) {
        reply->readAll(); // drain body for hygiene
        reply->deleteLater();
        return {false,
                tr("Remote file changed since last download. Re-sync to merge changes."),
                {},
                {},
                {},
                ErrorKind::Conflict};
    }

    // Non-success non-412 -- per-status locked banners via the centralized
    // mapWebdavStatusToMessage helper. 401/403/423/507 get their locked
    // banners; 5xx gets a generic server-error banner.
    if (httpStatus != HttpOk && httpStatus != HttpCreated && httpStatus != HttpNoContent) {
        reply->readAll(); // drain
        QString errorMsg = mapWebdavStatusToMessage(httpStatus);
        ErrorKind kind = mapWebdavStatusToKind(httpStatus);
        reply->deleteLater();
        return {false, errorMsg, {}, {}, {}, kind};
    }

    // Success (200/201/204) -- refresh m_lastETag from response headers so
    // the next upload in the same session uses the new value as If-Match
    // (mirrors Dropbox m_lastRev refresh). OC-Etag fallback. Weak ETags on
    // PUT response do NOT update m_lastETag and do NOT flip the session flag
    // here -- the session flag is download-driven (set during the prior
    // download's 200 branch); a weak ETag on the PUT response just means
    // "skip the refresh" since we can't safely use it as If-Match next time.
    QByteArray newEtagRaw = reply->rawHeader("ETag");
    if (newEtagRaw.isEmpty()) {
        newEtagRaw = reply->rawHeader("OC-Etag");
    }
    QString newEtag = QString::fromLatin1(newEtagRaw);
    if (!newEtag.isEmpty() && !newEtag.startsWith(QLatin1String("W/")) && newEtag != QLatin1String("\"\"")) {
        m_lastETag = newEtag;
    }

    reply->deleteLater();
    return {true, {}, {}, {}, {}};
}

RemoteHandler::RemoteResult NextcloudSyncProvider::refreshAuth(const RemoteSyncParams* params)
{
    // App-password authentication has no token refresh: the Basic-auth credential
    // is long-lived until the user revokes it via Nextcloud settings.
    Q_UNUSED(params)
    return {true, {}, {}, {}, {}};
}

// ---------------------------------------------------------------------------
// abort() implementation. Mirrors DropboxSyncProvider::abort byte-for-byte
// (storeRelease(1) + locked-mutex reply->abort()). The flag is checked by
// HttpRetryHelper::execute between retries (and during the retry-delay loop,
// polled every 100ms) so abort is responsive even when the provider is
// sleeping between retries. The post-execute abort check inside
// download/upload/testConnection surfaces the cancelled banner ("Operation
// cancelled") byte-for-byte.
//
// Mutex protects m_activeReply against the race where abort() and the
// transport function (download/upload/etc.) are on different threads.
// QMetaObject::invokeMethod with QueuedConnection marshals the reply
// abort to the reply's owning thread for thread safety.
// ---------------------------------------------------------------------------
void NextcloudSyncProvider::abort()
{
    m_abortFlag.storeRelease(1);

    QMutexLocker locker(&m_replyMutex);
    if (m_activeReply) {
        // Marshal abort() to the reply's owning thread for thread safety.
        QMetaObject::invokeMethod(m_activeReply, "abort", Qt::QueuedConnection);
    }
}

// ---------------------------------------------------------------------------
// retryOnAuthOnce -- single-401-retry-after-backoff helper. Wraps download()
// / upload() / testConnection() with a "try once, if AuthExpired retry once
// after a backoff" policy.
//
// Contract:
//   - First 401 is a TRANSIENT-ERROR opportunity: maybe the user's session
//     was rotated mid-call by a Nextcloud admin password reset, maybe the
//     server hiccupped, maybe a brief proxy auth issue. Retry once after a
//     2-second pause (production); if the second attempt also returns 401,
//     surface the existing AuthExpired banner and let the user re-authorize.
//   - First 401 NEVER auto-wipes stored credentials in CustomData. The
//     retry uses the SAME credentials the first attempt used; we do not
//     mutate params or provider state.
//   - The internal AuthRevoked semantic (two consecutive 401s) maps to the
//     SAME user-facing banner as AuthExpired (single 401 followed by retry
//     fail) because the user action is identical -- "Re-authorize in
//     Database > Settings > Cloud Sync." or "Verify the username and app
//     password" depending on call site.
//
// HttpRetryHelper is deliberately NOT extended to host this behavior: the
// retry-once-on-401 policy is Nextcloud-specific (Dropbox uses OAuth2
// refresh-then-retry; the contract is fundamentally different). The helper
// stays inside NextcloudSyncProvider.
//
// Abort polling during backoff: m_abortFlag is checked every 100ms during
// the backoff window so that abort() called between the first attempt and
// the second attempt returns the first-attempt result early WITHOUT making
// the second attempt. The 100ms slice is the same cadence HttpRetryHelper
// uses for its retry-delay loop -- the user-perceived abort latency is
// bounded at 100ms regardless of backoff length.
// ---------------------------------------------------------------------------
RemoteHandler::RemoteResult NextcloudSyncProvider::retryOnAuthOnce(std::function<RemoteHandler::RemoteResult()> op)
{
    RemoteHandler::RemoteResult first = op();
    // Branch on the provider-set kind, not on the localized errorMessage.
    // Substring-matching tr()'d strings silently breaks retry behavior on
    // every non-English build.
    if (first.success || first.kind != ErrorKind::AuthExpired) {
        return first;
    }

    // Single backoff before retry. Stored credentials in CustomData are
    // NEVER touched here -- caller's params still hold the same appPassword.
    //
    // Poll m_abortFlag every 100ms during the backoff so abort() during
    // retry returns early without making the second attempt.
    constexpr int sliceMs = 100;
    int waited = 0;
    while (waited < m_retryBackoffMs) {
        if (m_abortFlag.loadAcquire() != 0) {
            return first; // Abort wins -- return the first-attempt result.
        }
        const int thisSlice = qMin(sliceMs, m_retryBackoffMs - waited);
        QThread::msleep(static_cast<unsigned long>(thisSlice));
        waited += thisSlice;
    }
    // Final abort check after the loop, in case abort() arrived in the
    // last slice's window. Without this an abort firing in the final
    // sleep slice would not be observed before the second attempt.
    if (m_abortFlag.loadAcquire() != 0) {
        return first;
    }

    return op();
}

// ---------------------------------------------------------------------------
// Centralized HTTP-status -> locked banner-string mapper.
// Pure function of httpStatus (no instance state). Called from download(),
// upload(), and testConnection() non-success branches. Every banner here
// matches byte-for-byte the user-facing strings the classifyError dispatch
// pattern-matches against -- changing one without changing the other breaks
// UI dispatch.
//
// 500-series wording is not locked (only 401/403/404/412/423/507 are). The
// "Nextcloud server error (HTTP %1). Try again later." choice maps to
// ErrorKind::ServerError via the "server error" fragment in classifyError.
// ---------------------------------------------------------------------------
QString NextcloudSyncProvider::mapWebdavStatusToMessage(int httpStatus)
{
    switch (httpStatus) {
    case HttpUnauthorized: // 401
        return tr("Nextcloud authorization expired. Re-authorize in Database > Settings > Cloud Sync.");
    case HttpForbidden: // 403
        return tr("Nextcloud denied access to this path. Verify the file path and your account permissions.");
    case HttpNotFound: // 404 (testConnection-side; download's 404-trash distinction routes through checkIfInTrash)
        return tr("Nextcloud could not find the configured remote path. Verify your settings.");
    case HttpPreconditionFailed: // 412
        return tr("Remote file changed since last download. Re-sync to merge changes.");
    case HttpLocked: // 423
        return tr("Nextcloud file is locked. Try again in a moment.");
    case HttpInsufficientStorage: // 507
        return tr("Nextcloud server is out of storage. Free space and try again.");
    default:
        if (httpStatus >= 500 && httpStatus < 600) {
            return tr("Nextcloud server error (HTTP %1). Try again later.").arg(httpStatus);
        }
        return tr("Nextcloud returned HTTP %1.").arg(httpStatus);
    }
}

RemoteHandler::ErrorKind NextcloudSyncProvider::mapWebdavStatusToKind(int httpStatus)
{
    switch (httpStatus) {
    case HttpUnauthorized: // 401
        return ErrorKind::AuthExpired;
    case HttpForbidden: // 403
        return ErrorKind::Permission;
    case HttpNotFound: // 404
        return ErrorKind::NotFound;
    case HttpPreconditionFailed: // 412
        return ErrorKind::Conflict;
    case HttpLocked: // 423
        return ErrorKind::RateLimit;
    case HttpInsufficientStorage: // 507
        return ErrorKind::Quota;
    default:
        if (httpStatus >= 500 && httpStatus < 600) {
            return ErrorKind::ServerError;
        }
        return ErrorKind::Other;
    }
}

// ---------------------------------------------------------------------------
// classifyError -- banner-text -> ErrorKind dispatch for the MessageWidget
// banner path. Mirrors the DropboxSyncProvider::classifyError keyword-pattern;
// case-insensitive substring matching against fragment keywords from the
// locked banner table. Each fragment is unique to its ErrorKind so accidental
// cross-matches are impossible.
//
// Conflict-surface invariant: the 412 banner string ("Remote file changed
// since last download...") dispatches to ErrorKind::Conflict here AND
// surfaces verbatim from upload(). The "restart" semantic is the next user
// sync, not an automatic in-engine loop.
//
// classifyError returns ErrorKind::Network for the SSL banner string. The
// provider NEVER calls QNetworkReply::ignoreSslErrors; SSL handshake failures
// surface to the user as an actionable banner. The provider assumes a
// publicly-trusted CA.
// ---------------------------------------------------------------------------
RemoteSyncProvider::ErrorKind NextcloudSyncProvider::classifyError(const QString& errorMessage) const
{
    // Order matters where fragments could in principle overlap; chosen here so
    // the most specific match wins. Conflict's "Remote file changed" fragment
    // is unique; trash's "Nextcloud trash" fragment is unique; etc.
    if (errorMessage.contains(QStringLiteral("authorization expired"), Qt::CaseInsensitive)) {
        return ErrorKind::AuthExpired;
    }
    // The credential-rejection banner from testConnection's 401 branch
    // dispatches to AuthExpired (same as the standard 401 banner above) so
    // retryOnAuthOnce treats both call-site banners uniformly. The fragment
    // below cannot collide with any other Nextcloud banner string.
    if (errorMessage.contains(QStringLiteral("rejected those credentials"), Qt::CaseInsensitive)) {
        return ErrorKind::AuthExpired;
    }
    if (errorMessage.contains(QStringLiteral("denied access"), Qt::CaseInsensitive)) {
        return ErrorKind::Permission;
    }
    if (errorMessage.contains(QStringLiteral("Nextcloud trash"), Qt::CaseInsensitive)
        || errorMessage.contains(QStringLiteral("could not find the configured remote path"), Qt::CaseInsensitive)) {
        return ErrorKind::NotFound;
    }
    if (errorMessage.contains(QStringLiteral("Remote file changed"), Qt::CaseInsensitive)) {
        return ErrorKind::Conflict;
    }
    if (errorMessage.contains(QStringLiteral("file is locked"), Qt::CaseInsensitive)) {
        return ErrorKind::RateLimit;
    }
    if (errorMessage.contains(QStringLiteral("out of storage"), Qt::CaseInsensitive)) {
        return ErrorKind::Quota;
    }
    if (errorMessage.contains(QStringLiteral("SSL certificate"), Qt::CaseInsensitive)) {
        // SSL handshake is a network-level failure for the abstraction; the
        // ErrorKind enum has no dedicated SslHandshake value.
        return ErrorKind::Network;
    }
    if (errorMessage.contains(QStringLiteral("server error"), Qt::CaseInsensitive)) {
        return ErrorKind::ServerError;
    }
    return ErrorKind::Other;
}

bool NextcloudSyncProvider::isAuthorized(const QJsonObject& config) const
{
    // Operational contract: a config is "authorized" only if it has every
    // field required for a successful sync round-trip.
    //   - loginName + appPassword: Basic-auth credential pair
    //   - serverBaseUrl: WebDAV endpoint base; sync has no usable default
    //   - remotePath: target path under the user's files namespace
    return !config.value(QStringLiteral("loginName")).toString().isEmpty()
           && !config.value(QStringLiteral("appPassword")).toString().isEmpty()
           && !config.value(QStringLiteral("serverBaseUrl")).toString().isEmpty()
           && !config.value(QStringLiteral("remotePath")).toString().isEmpty();
}

// ---------------------------------------------------------------------------
// URL canonicalization helper.
// Idempotent -- applying twice is the same as applying once.
// ---------------------------------------------------------------------------
QString NextcloudSyncProvider::canonicalizeServerBaseUrl(QString input)
{
    input = input.trimmed();
    if (input.isEmpty()) {
        return {};
    }

    // 1. Default scheme to https:// when absent
    QUrl url(input);
    if (url.scheme().isEmpty()) {
        url = QUrl(QStringLiteral("https://") + input);
    }

    // 2. Transport-security gate. We reject cleartext http:// for non-loopback
    // hosts: the app-password is sent via the Authorization: Basic header on
    // every download/upload/testConnection, so allowing http://example.com
    // would leak the credential to any on-path observer. Loopback http is
    // permitted as a dev / self-hosted local escape hatch. Any other scheme
    // (ftp/file/etc.) is also rejected -- WebDAV-over-Basic only meaningfully
    // makes sense over https or local http.
    const QString scheme = url.scheme();
    if (scheme == QStringLiteral("http")) {
        if (!isLoopbackHost(url)) {
            return {};
        }
    } else if (scheme != QStringLiteral("https")) {
        return {};
    }

    // 3. Strip trailing slash from path (preserve subpath segments)
    QString path = url.path();
    while (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    url.setPath(path, QUrl::DecodedMode);

    // 4. Drop fragment / query -- not part of a server-base URL.
    url.setFragment(QString());
    url.setQuery(QString());

    return url.toString(QUrl::FullyEncoded);
}

bool NextcloudSyncProvider::isLoopbackHost(const QUrl& url)
{
    const QString host = url.host(QUrl::FullyDecoded);
    if (host.isEmpty()) {
        return false;
    }
    // Match the literal hostname "localhost" (case-insensitive). QHostAddress
    // does not resolve hostnames, so this exact match is the only way to
    // accept the hostname-form. Anything else like "localhost.example.com" is
    // NOT loopback and must not match.
    if (host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    // For IP literals, defer to QHostAddress::isLoopback (covers 127.0.0.0/8
    // and ::1). QUrl::host returns IPv6 without surrounding brackets, which
    // is the format QHostAddress accepts.
    const QHostAddress addr(host);
    return !addr.isNull() && addr.isLoopback();
}

// ---------------------------------------------------------------------------
// validateServerUrl -- single source of truth for "is this user-typed URL
// acceptable as a Nextcloud server base?". Mirrors canonicalizeServerBaseUrl's
// gates but returns a 4-way enum so callers can dispatch per-case banners
// instead of collapsing every failure to a single generic message.
//
// Order of checks reflects the precision of the message a caller can emit:
//   1. Empty       -> "URL field blank" (Warning)
//   2. NotSecure   -> "Plain HTTP only for loopback ..." (Error, anti-cleartext)
//   3. Malformed   -> "Invalid URL" (Warning)
//   4. Ok          -> proceed; canonicalOut filled
//
// Steps 2 and 3 only run when input is non-empty. Step 2 fires BEFORE step 3
// so a user who typed "http://example.com" sees the actionable cleartext-
// policy message rather than the generic malformed-fallback (their URL is
// syntactically fine -- it's the scheme that the policy rejects).
// ---------------------------------------------------------------------------
NextcloudSyncProvider::ServerUrlValidity
NextcloudSyncProvider::validateServerUrl(const QString& input, QString* canonicalOut)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return ServerUrlValidity::Empty;
    }

    // Default scheme to https:// when absent (mirrors canonicalizeServerBaseUrl).
    QUrl url(trimmed);
    if (url.scheme().isEmpty()) {
        url = QUrl(QStringLiteral("https://") + trimmed);
    }

    // Transport-security gate. NotSecure is reported BEFORE Malformed so the
    // user who typed a syntactically-valid http://example.com sees the
    // specific cleartext-policy banner rather than a generic "invalid URL".
    const QString scheme = url.scheme();
    if (scheme == QStringLiteral("http")) {
        if (!isLoopbackHost(url)) {
            return ServerUrlValidity::NotSecure;
        }
    } else if (scheme != QStringLiteral("https")) {
        return ServerUrlValidity::Malformed;
    }

    // Syntax + host gate. QUrl::isValid is permissive (e.g. accepts
    // "https://" with empty host). We require a non-empty host so inputs
    // like "https://" or "https:///foo" -- which would canonicalize to
    // something that fails only at request-issue time downstream -- are
    // caught here at the trust boundary.
    if (!url.isValid() || url.host().isEmpty()) {
        return ServerUrlValidity::Malformed;
    }

    if (canonicalOut) {
        *canonicalOut = canonicalizeServerBaseUrl(trimmed);
    }
    return ServerUrlValidity::Ok;
}

// ---------------------------------------------------------------------------
// Remote-path NFC normalization helper.
// Nextcloud's WebDAV layer treats the path as a byte sequence -- a decomposed
// form (NFD, e.g. macOS Finder paste) won't match a server-stored NFC path
// for the same logical filename. Normalize to NFC at the UI boundary so the
// "NFC-normalized at save" contract in RemoteSyncParams.h holds in practice.
// Idempotent.
// ---------------------------------------------------------------------------
QString NextcloudSyncProvider::normalizeRemotePath(QString input)
{
    return input.trimmed().normalized(QString::NormalizationForm_C);
}

// ---------------------------------------------------------------------------
// WebDAV path composition helper.
// Uses QUrl::setPath(decoded, QUrl::DecodedMode) for correct per-segment
// encoding -- avoids double-encoding of spaces, parens, non-ASCII, '@' etc.
// ---------------------------------------------------------------------------
QUrl NextcloudSyncProvider::buildResourceUrl(const QString& canonicalBase,
                                             const QString& loginName,
                                             const QString& remotePath)
{
    QUrl url(canonicalBase);

    // Per-segment encoding for the loginName (Qt's path encoder treats '@' as
    // a valid pchar and won't escape it, but Nextcloud expects '%40' here).
    // The remotePath is left as raw decoded characters so its '/' separators
    // survive. TolerantMode preserves the loginName's existing %-encoding
    // while still encoding spaces, non-ASCII, etc. in remotePath.
    const QString encodedLogin = QString::fromUtf8(QUrl::toPercentEncoding(loginName));
    QString fullPath = url.path() // preserves subpath if present
                       + QStringLiteral("/remote.php/dav/files/") + encodedLogin
                       + (remotePath.startsWith(QLatin1Char('/')) ? remotePath : QStringLiteral("/") + remotePath);

    url.setPath(fullPath, QUrl::TolerantMode);
    return url;
}
