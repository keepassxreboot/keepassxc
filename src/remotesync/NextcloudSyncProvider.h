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

#ifndef KEEPASSXC_NEXTCLOUDSYNCPROVIDER_H
#define KEEPASSXC_NEXTCLOUDSYNCPROVIDER_H

#include "RemoteSyncProvider.h"

#include <QAtomicInt>
#include <QMutex>
#include <QString>
#include <QUrl>

#include <functional>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
struct NextcloudSyncParams;

/**
 * Nextcloud WebDAV provider: PUT/GET/PROPFIND with ETag-based conflict detection.
 * Authenticates via long-lived app password (Basic auth); single-401-retry policy
 * routes transient auth failures through retryOnAuthOnce before surfacing.
 */
class NextcloudSyncProvider : public RemoteSyncProvider
{
    Q_OBJECT

public:
    explicit NextcloudSyncProvider(QObject* parent = nullptr);
    ~NextcloudSyncProvider() override;

    RemoteHandler::RemoteResult download(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) override;
    void abort() override;

    // PROPFIND Depth:0 against the configured remote path. Returns success on
    // 200/207, mapWebdavStatusToMessage banner on any other status.
    RemoteHandler::RemoteResult testConnection(const NextcloudSyncParams* params);

    QString displayName() const override;
    RemoteSyncParams* createParams() const override;
    RemoteSyncParams* buildParamsFromConfig(const QJsonObject& config) const override;
    ErrorKind classifyError(const QString& errorMessage) const override;
    bool isAuthorized(const QJsonObject& config) const override;

    // Inject a QNetworkAccessManager for testing (mock QNAM whose
    // get/put/sendCustomRequest returns MockNetworkReply). Caller retains
    // ownership: the injected NAM must outlive this object, is never
    // delete-d or reparented by the setter, and calling with nullptr does
    // not free a previously-set NAM.
    void setNetworkAccessManager(QNetworkAccessManager* nam);

    // Public static URL helpers -- pure functions, no side effects, called by
    // both this provider and the settings widget. Idempotent.
    //
    // canonicalizeServerBaseUrl also enforces the transport security policy:
    // an empty string is returned for any URL that would let the Basic-auth
    // app-password header leave the box unprotected. Concretely:
    //   - missing scheme: defaulted to https
    //   - https: accepted
    //   - http with a loopback host (localhost / 127.x / ::1): accepted
    //   - http with any other host: REJECTED (returns "")
    //   - anything else (ftp/file/etc.): REJECTED
    // Callers should pre-validate at the user-input boundary to surface a
    // specific error; an empty return here is the fail-closed fallback.
    static QString canonicalizeServerBaseUrl(QString input);
    static QUrl buildResourceUrl(const QString& canonicalBase, const QString& loginName, const QString& remotePath);

    // Returns true iff the URL's host is a loopback address (the literal
    // "localhost", any 127.x.x.x IPv4, or ::1 IPv6). Used by the transport-
    // security gate in canonicalizeServerBaseUrl AND by the page so the
    // settings UI surfaces a specific error before a save / authorize
    // attempts to send creds.
    static bool isLoopbackHost(const QUrl& url);

    // Outcome of validateServerUrl. Lets the page-side dispatch per-case
    // user-facing banners (empty field vs unsupported scheme vs insecure
    // scheme vs syntactically malformed URL) rather than collapsing all
    // failures to one generic message.
    enum class ServerUrlValidity
    {
        Ok, // non-empty, scheme allowed, host present
        Empty, // input.trimmed().isEmpty()
        NotSecure, // http scheme with a non-loopback host
        Malformed, // unsupported scheme, syntactically invalid, or no host
    };

    // Validate a user-typed Nextcloud server URL against the transport-
    // security policy AND the syntactic shape downstream callers (Login Flow
    // v2 initiate, WebDAV PROPFIND, browser deep-link) require. Pure
    // function -- no side effects, no signal emission. Idempotent.
    //
    // canonicalOut: optional. When non-null AND result == Ok, receives the
    // canonicalizeServerBaseUrl form. Left untouched on any non-Ok result so
    // a caller can pass &m_config-bound storage without risking partial fill.
    //
    // Intentionally does NOT emit showMessage: the 3 click handlers that
    // consume this surface different banner palettes (Warning for the
    // "fill the field" prompts, Error for the cleartext-policy rejection).
    // Centralizing emission here would couple this primitive to the UI.
    static ServerUrlValidity validateServerUrl(const QString& input, QString* canonicalOut = nullptr);

    // NFC-normalize + trim a Nextcloud remote path at the UI boundary.
    // Nextcloud's WebDAV layer is case-/byte-sensitive on the path, so a
    // decomposed (NFD) sequence pasted by the user (e.g. macOS Finder) will
    // not match a path stored on the server as NFC. Applying NormalizationForm_C
    // at the page-to-config boundary makes the NFC contract on
    // NextcloudSyncParams::remotePath actually hold. Idempotent.
    static QString normalizeRemotePath(QString input);

    Q_DISABLE_COPY(NextcloudSyncProvider)

#ifdef QT_TEST_LIB
    // Test-only inspection of the captured ETag. Production code never reads
    // this -- it is consumed only by upload(). Surfaced here so unit tests can
    // assert byte-for-byte preservation of opaque-tag quotes without leaking
    // m_lastETag into the public API.
public:
    QString lastETagForTest() const
    {
        return m_lastETag;
    }

    // Weak-ETag session-flag accessor for tests. The flag is "sticky" within a
    // single provider lifetime (once a W/"..." is observed, all subsequent
    // uploads silent-overwrite). Tests assert the flag is set after the first
    // weak detection and stays set through subsequent strong ETags.
    bool serverEmitsWeakETagsForTest() const
    {
        return m_serverEmitsWeakETags;
    }

    // Seed m_lastETag for upload-side tests that need to assert
    // `If-Match: <verbatim>` round-trip without first running a download.
    void setLastETagForTest(const QString& v)
    {
        m_lastETag = v;
    }

    // Seed m_serverEmitsWeakETags for upload-side tests that need to assert
    // silent-overwrite semantics (NEITHER If-Match NOR If-None-Match: * sent)
    // without first running a weak-ETag download.
    void setServerEmitsWeakETagsForTest(bool v)
    {
        m_serverEmitsWeakETags = v;
    }

    // Seed m_abortFlag so a test can assert the entry-point reset contract
    // (the flag must be cleared at the top of download() / upload() /
    // testConnection() / refreshAuth() before any HTTP work).
    void setAbortFlagForTest(int v)
    {
        m_abortFlag.storeRelease(v);
    }

    // Read m_abortFlag without taking the reply mutex. Tests use this to
    // assert abort() set the flag. Production code never reads this.
    int abortFlagForTest() const
    {
        return m_abortFlag.loadAcquire();
    }

    // Shrink the retryOnAuthOnce backoff from the production 2-second default
    // to milliseconds so unit tests run in well under 1 second wall-clock per
    // test. Production code never calls this; the default 2000ms is set in
    // the member initializer below.
    void setRetryBackoffMsForTest(int ms)
    {
        m_retryBackoffMs = ms;
    }
#endif

signals:
    // Emitted once per session when a weak ETag (W/"...") is detected from
    // the server.
    void weakEtagDetected();

private:
    // Trashbin lookup for the 404 distinction. PROPFIND Depth:1 against
    // `<base>/remote.php/dav/trashbin/<loginName>/trash`. Returns true when
    // the file's basename appears anywhere in any `<d:href>` element in the
    // response body (permissive contains() match -- handles deletion-suffix
    // format variance). Returns false for any failure (no trashbin, network
    // error, etc.) -- treats the absence as "first sync" silent success
    // rather than an error.
    bool checkIfInTrash(const NextcloudSyncParams* params, const QString& filename);

    // Lazy-construct or return the injected QNetworkAccessManager (mirrors
    // Dropbox).
    void ensureNam();

    // Centralized HTTP-status -> locked banner-string mapper. Pure function
    // of httpStatus (no instance state); called from download(), upload(),
    // and testConnection() non-success branches.
    static QString mapWebdavStatusToMessage(int httpStatus);

    // Companion to mapWebdavStatusToMessage: the same HTTP status maps to
    // a machine-readable ErrorKind that retry/dispatch logic can branch on
    // without parsing the (localized) banner string.
    static RemoteHandler::ErrorKind mapWebdavStatusToKind(int httpStatus);

    // Single-401-retry-after-backoff policy wrapper. Invokes op() once. If
    // the result is success OR classifyError(result.errorMessage) !=
    // ErrorKind::AuthExpired, returns that first result immediately.
    // Otherwise sleeps m_retryBackoffMs -- polling m_abortFlag every 100ms
    // so abort() during the backoff returns the first-attempt result early
    // without making the second attempt -- then invokes op() once more and
    // returns that second result.
    //
    // First 401 NEVER mutates params or provider state. Stored creds in
    // CustomData are NEVER auto-wiped here -- the helper only retries. Two
    // consecutive 401s surface the standard 401 banner (or, on the manual-
    // paste credential-validation path through testConnection, the credential-
    // rejection banner); the user action is the same in both cases
    // ("Re-authorize" or "Verify creds").
    RemoteHandler::RemoteResult retryOnAuthOnce(std::function<RemoteHandler::RemoteResult()> op);

    // Implementation bodies. The public download() / upload() /
    // testConnection() entry points are thin wrappers around
    // retryOnAuthOnce([this, params]() { return *Impl(...); }); the actual
    // transport logic lives here. The abort-flag reset
    // (m_abortFlag.storeRelease(0)) is performed in each public wrapper so a
    // fresh op call always starts with a clean abort flag.
    RemoteHandler::RemoteResult downloadImpl(const RemoteSyncParams* params);
    RemoteHandler::RemoteResult uploadImpl(const QString& filePath, const RemoteSyncParams* params);
    RemoteHandler::RemoteResult testConnectionImpl(const NextcloudSyncParams* params);

    QNetworkAccessManager* m_nam = nullptr;

    // HTTP status constants for WebDAV operations.
    static constexpr int HttpOk = 200;
    static constexpr int HttpCreated = 201;
    static constexpr int HttpNoContent = 204;
    static constexpr int HttpMultiStatus = 207;
    static constexpr int HttpUnauthorized = 401;
    static constexpr int HttpForbidden = 403;
    static constexpr int HttpNotFound = 404;
    static constexpr int HttpPreconditionFailed = 412;
    static constexpr int HttpLocked = 423;
    static constexpr int HttpInsufficientStorage = 507;
    static constexpr int MaxDatabaseSize = 256 * 1024 * 1024; // 256 MB sanity limit

    QString m_lastETag; // ETag from last download, used for upload If-Match
    bool m_serverEmitsWeakETags = false; // Sticky session flag once a W/"..." is seen
    QNetworkReply* m_activeReply = nullptr; // For abort support
    mutable QMutex m_replyMutex; // Protects m_activeReply across threads
    QAtomicInt m_abortFlag; // Atomic flag checked by HttpRetryHelper between retries
    int m_retryBackoffMs = 2000; // 2-second backoff between first-401 and retry attempt;
                                 // setRetryBackoffMsForTest shrinks this to ms in unit tests.
};

#endif // KEEPASSXC_NEXTCLOUDSYNCPROVIDER_H
