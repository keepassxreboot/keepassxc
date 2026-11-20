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

#include "WhisperClient.h"

#include "NetworkManager.h"
#include "core/Clock.h"
#include "core/Entry.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUuid>

namespace
{
    constexpr int RequestTimeoutMs = 15000;

    QString joinUrl(const QString& base, const QString& path)
    {
        QString url = base;
        while (url.endsWith('/')) {
            url.chop(1);
        }
        return url + path;
    }

    // The Whisper viewer only accepts unpadded base64url
    QString toBase64Url(const QByteArray& data)
    {
        return QString::fromLatin1(data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }

    QString toJsonString(const QString& value)
    {
        const auto json = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
        return QString::fromUtf8(json.mid(1, json.size() - 2));
    }

    // QJsonObject sorts its keys, but the viewer lists the fields in document order
    QString toOrderedJsonObject(const QList<QPair<QString, QString>>& fields)
    {
        QStringList members;
        for (const auto& field : fields) {
            members << toJsonString(field.first) + ":" + toJsonString(field.second);
        }
        return "{" + members.join(",") + "}";
    }
} // namespace

WhisperClient::WhisperClient(QObject* parent)
    : QObject(parent)
{
}

WhisperClient::~WhisperClient()
{
    abort();
}

QString WhisperClient::buildPayload(const Entry* entry, const Fields& fields)
{
    QList<QPair<QString, QString>> values;
    auto add = [&](bool selected, const QString& key, const QString& value) {
        const auto resolved = selected ? entry->resolveMultiplePlaceholders(value) : QString();
        if (!resolved.isEmpty()) {
            values.append({key, resolved});
        }
    };
    add(fields.username, "username", entry->username());
    add(fields.password, "password", entry->password());
    add(fields.url, "url", entry->url());
    add(fields.notes, "notes", entry->notes());

    if (values.size() == 1) {
        return values.first().second;
    }
    return values.isEmpty() ? QString() : toOrderedJsonObject(values);
}

bool WhisperClient::encrypt(const QString& plaintext, QByteArray& key, QByteArray& payload)
{
    const auto mode = SymmetricCipher::Aes256_GCM;
    key = randomGen()->randomArray(SymmetricCipher::keySize(mode));
    // Whisper expects a 12-byte nonce, defaultIvSize() is 16 for GCM
    const auto nonce = randomGen()->randomArray(SymmetricCipher::ivSize(mode));

    QByteArray data = plaintext.toUtf8();
    SymmetricCipher cipher;
    if (!cipher.init(mode, SymmetricCipher::Encrypt, key, nonce) || !cipher.finish(data)) {
        key.clear();
        return false;
    }
    payload = nonce + data;
    return true;
}

QString WhisperClient::parseSecretId(const QByteArray& responseBody)
{
    const auto id = QJsonDocument::fromJson(responseBody)["id"].toString();
    // The id ends up in the share link, so only accept a UUID
    return QUuid::fromString(id).isNull() ? QString() : id;
}

QUrl WhisperClient::buildShareUrl(const QString& serverUrl, const QString& secretId, const QByteArray& key)
{
    QUrl url(joinUrl(serverUrl, "/get_secret"));
    url.setQuery(QUrlQuery{{"shared_secret_id", secretId}});
    // Browsers never send the fragment to the server, so the key stays with the link holder
    url.setFragment("k=" + toBase64Url(key));
    return url;
}

void WhisperClient::shareSecret(const QString& serverUrl,
                                const QString& secret,
                                int expirationSeconds,
                                bool selfDestruct)
{
    abort();
    const QUrl server(serverUrl);
    if (server.scheme() != "https" || server.host().isEmpty()) {
        emit shareFailed(tr("The Whisper server URL must start with https://"));
        return;
    }
    QByteArray payload;
    if (!encrypt(secret, m_key, payload)) {
        emit shareFailed(tr("Failed to encrypt the secret."));
        return;
    }
    m_serverUrl = serverUrl;

    QNetworkRequest request(QUrl(joinUrl(serverUrl, "/v1/ephemeral")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(RequestTimeoutMs);
    const QJsonObject body{{"payload", toBase64Url(payload)},
                           {"expiration", static_cast<qint64>(Clock::currentSecondsSinceEpoch()) + expirationSeconds},
                           {"self_destruct", selfDestruct}};
    m_reply = getNetMgr()->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &WhisperClient::replyFinished);
}

void WhisperClient::abort()
{
    m_key.clear();
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void WhisperClient::replyFinished()
{
    auto reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();
    const auto key = m_key;
    m_key.clear();

    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto id = parseSecretId(reply->readAll());
    if (status == 201 && !id.isEmpty()) {
        emit shareSucceeded(buildShareUrl(m_serverUrl, id, key));
    } else if (reply->error() != QNetworkReply::NoError) {
        emit shareFailed(reply->errorString());
    } else {
        emit shareFailed(tr("Unexpected response from the Whisper server (HTTP %1).").arg(status));
    }
}
