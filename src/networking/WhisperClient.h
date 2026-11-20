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

#ifndef KEEPASSXC_WHISPERCLIENT_H
#define KEEPASSXC_WHISPERCLIENT_H

#include "config-keepassx.h"

#include <QObject>
#include <QUrl>

#ifndef KPXC_FEATURE_NETWORK
#error This file requires KeePassXC to be built with network support.
#endif

class Entry;
class QNetworkReply;

class WhisperClient : public QObject
{
    Q_OBJECT

public:
    struct Fields
    {
        bool username = false;
        bool password = false;
        bool url = false;
        bool notes = false;
    };

    explicit WhisperClient(QObject* parent = nullptr);
    ~WhisperClient() override;

    void shareSecret(const QString& serverUrl, const QString& secret, int expirationSeconds, bool selfDestruct);

    static QString buildPayload(const Entry* entry, const Fields& fields);
    static bool encrypt(const QString& plaintext, QByteArray& key, QByteArray& payload);
    static QString parseSecretId(const QByteArray& responseBody);
    static QUrl buildShareUrl(const QString& serverUrl, const QString& secretId, const QByteArray& key);

signals:
    void shareSucceeded(const QUrl& shareLink);
    void shareFailed(const QString& errorMessage);

private slots:
    void replyFinished();

private:
    void abort();

    QString m_serverUrl;
    QByteArray m_key;
    QNetworkReply* m_reply = nullptr;

    Q_DISABLE_COPY(WhisperClient)
};

#endif // KEEPASSXC_WHISPERCLIENT_H
