/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "KdbxCoordinatorClient.h"

#include <QCryptographicHash>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>

KdbxCoordinatorClient::KdbxCoordinatorClient(const QUrl& service,
                                               const QString& databaseId,
                                               const QByteArray& accessToken,
                                               const QString& owner,
                                               QObject* parent)
    : QObject(parent)
    , m_service(service)
    , m_databaseId(databaseId)
    , m_accessToken(accessToken)
    , m_owner(owner)
{
    m_renewTimer.setInterval(25000); // below server's 90-second expiry
    connect(&m_renewTimer, &QTimer::timeout, this, [this] {
        QString error;
        if (!renew(&error)) {
            invalidate(); // Fail closed in the database widget immediately.
        }
    });
}

KdbxCoordinatorClient::~KdbxCoordinatorClient()
{
    QString ignored;
    release(&ignored); // best effort; server lease expiry is the ultimate fallback
}

void KdbxCoordinatorClient::invalidate()
{
    if (m_lost) {
        return;
    }
    m_lost = true;
    m_renewTimer.stop();
    if (!m_lease.isEmpty()) {
        emit leaseLost();
    }
}

QByteArray KdbxCoordinatorClient::hash(const QByteArray& bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

QByteArray KdbxCoordinatorClient::field(const QByteArray& body, const QString& key)
{
    QJsonParseError parseError;
    auto document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }
    return document.object().value(key).toString().toUtf8();
}

KdbxCoordinatorClient::Response KdbxCoordinatorClient::request(const QByteArray& method,
                                                                 const QString& action,
                                                                 const QByteArray& payload,
                                                                 const QByteArray& lease,
                                                                 const QByteArray& expectedRevision)
{
    Response result;
    const auto host = m_service.host();
    if (m_service.scheme() != QStringLiteral("https") &&
        !(m_service.scheme() == QStringLiteral("http") &&
          (host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1")))) {
        result.transportError = QStringLiteral("HTTPS is required");
        return result;
    }
    if (!m_service.userInfo().isEmpty() || m_service.hasQuery() || m_service.hasFragment() ||
        !QRegularExpression(QStringLiteral("^[a-z0-9_-]{1,96}$")).match(m_databaseId).hasMatch() ||
        m_accessToken.size() < 32 || m_accessToken.contains('\n') || m_accessToken.contains('\r')) {
        result.transportError = QStringLiteral("Invalid coordinator configuration");
        return result;
    }

    QUrl url = m_service;
    url.setPath(m_service.path().replace(QRegularExpression(QStringLiteral("/$")), {}) +
                QStringLiteral("/v1/databases/") + m_databaseId + action);
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader("Authorization", "Bearer " + m_accessToken);
    req.setRawHeader("Cache-Control", "no-store");
    req.setRawHeader("Accept", "application/json, application/octet-stream");
    if (!lease.isEmpty()) {
        req.setRawHeader("X-KDBX-Lease", lease);
    }
    if (!expectedRevision.isEmpty()) {
        req.setRawHeader("If-Match", '"' + expectedRevision + '"');
    }
    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = m_network.get(req);
    } else if (method == "POST") {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        reply = m_network.post(req, payload);
    } else if (method == "PUT") {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        reply = m_network.put(req, payload);
    } else if (method == "DELETE") {
        reply = m_network.deleteResource(req);
    } else {
        result.transportError = QStringLiteral("Unsupported HTTP method");
        return result;
    }

    // Synchronous only inside a worker: no freezing the GUI.
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeout.start(40000);
    if (!reply->isFinished()) {
        loop.exec();
    }
    timeout.stop();
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.etag = reply->rawHeader("ETag");
    result.bytes = reply->readAll();
    if (reply->error() != QNetworkReply::NoError && result.status == 0) {
        result.transportError = reply->errorString();
    }
    reply->deleteLater();
    return result;
}

bool KdbxCoordinatorClient::acquire(QString* error)
{
    if (!m_lease.isEmpty()) {
        if (error) *error = QStringLiteral("Already holding a lease");
        return false;
    }
    const QJsonObject body{{QStringLiteral("owner"), m_owner}};
    auto result = request("POST", QStringLiteral("/lock"), QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_busy = result.status == 423;
    if (result.status != 200) {
        if (error) *error = m_busy ? QStringLiteral("Database is locked by another editor")
                                  : QStringLiteral("Acquire failed: HTTP %1 %2").arg(result.status).arg(result.transportError);
        return false;
    }
    const auto token = field(result.bytes, QStringLiteral("token"));
    const auto revision = field(result.bytes, QStringLiteral("revision"));
    const QRegularExpression hex(QStringLiteral("^[a-f0-9]{64}$"));
    if (!hex.match(QString::fromLatin1(token)).hasMatch() ||
        (revision != "0" && !hex.match(QString::fromLatin1(revision)).hasMatch())) {
        if (error) *error = QStringLiteral("Malformed coordinator lease response");
        return false;
    }
    m_lease = token;
    m_revision = revision;
    m_lost = false;
    m_renewTimer.start();
    return true;
}

bool KdbxCoordinatorClient::download(QByteArray* encryptedDatabase, bool* exists, QString* error)
{
    if (!hasLease()) {
        if (error) *error = QStringLiteral("Editing lease missing or lost");
        return false;
    }
    auto result = request("GET", QStringLiteral("/file"));
    if (result.status == 404 && m_revision == "0") {
        if (exists) *exists = false;
        if (encryptedDatabase) encryptedDatabase->clear();
        return true;
    }
    if (result.status != 200) {
        if (error) *error = QStringLiteral("Download failed: HTTP %1 %2").arg(result.status).arg(result.transportError);
        return false;
    }
    if (hash(result.bytes) != m_revision || result.etag != '"' + m_revision + '"') {
        invalidate();
        if (error) *error = QStringLiteral("Remote database changed unexpectedly");
        return false;
    }
    if (encryptedDatabase) *encryptedDatabase = result.bytes;
    if (exists) *exists = true;
    return hasLease();
}

bool KdbxCoordinatorClient::publish(const QByteArray& encryptedDatabase, QString* error)
{
    if (!hasLease() || encryptedDatabase.isEmpty()) {
        if (error) *error = QStringLiteral("No valid editing lease or empty database");
        return false;
    }
    const auto result = request("PUT", QStringLiteral("/file"), encryptedDatabase, m_lease, m_revision);
    if (result.status == 409 || result.status == 412) {
        invalidate();
    }
    if (result.status != 200) {
        if (error) *error = QStringLiteral("Publication failed: HTTP %1 %2").arg(result.status).arg(result.transportError);
        return false;
    }
    const auto expected = hash(encryptedDatabase);
    if (field(result.bytes, QStringLiteral("revision")) != expected || result.etag != '"' + expected + '"') {
        invalidate();
        if (error) *error = QStringLiteral("Publication acknowledgement mismatch");
        return false;
    }
    if (!hasLease()) {
        if (error) *error = QStringLiteral("Editing lease lost while publishing");
        return false;
    }
    m_revision = expected;
    return true;
}

bool KdbxCoordinatorClient::renew(QString* error)
{
    if (!hasLease()) {
        if (error) *error = QStringLiteral("Editing lease lost");
        return false;
    }
    const auto result = request("POST", QStringLiteral("/renew"), QByteArrayLiteral("{}"), m_lease);
    if (result.status != 200) {
        invalidate();
        if (error) *error = QStringLiteral("Lease renewal failed: HTTP %1 %2").arg(result.status).arg(result.transportError);
        return false;
    }
    return true;
}

bool KdbxCoordinatorClient::release(QString* error)
{
    m_renewTimer.stop();
    if (m_lease.isEmpty()) return true;
    const auto token = m_lease;
    m_lease.clear();
    m_revision.clear();
    m_lost = true;
    const auto result = request("DELETE", QStringLiteral("/lock"), {}, token);
    if (result.status != 204 && result.status != 409) {
        if (error) *error = QStringLiteral("Lease release not confirmed: HTTP %1 %2").arg(result.status).arg(result.transportError);
        return false;
    }
    return true;
}
