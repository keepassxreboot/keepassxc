/*
 *  Copyright (C) 2023 Patrick Sean Klein <patrick@libklein.com>
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
#ifndef KEEPASSXC_NETWORKREQUEST_H
#define KEEPASSXC_NETWORKREQUEST_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QUrl>
class QNetworkReply;
class QNetworkAccessManager;

/**
 * Implements a simple HTTP request with a timeout and a maximum number of redirects.
 * The result of the request is emitted as a signal, specifically success() or failure().
 * The result is a QByteArray containing the response body. Further information about the
 * response can be obtained by calling the url(), ContentType(), and ContentTypeParameters()
 * methods.
 */
class NetworkRequest : public QObject
{
    Q_OBJECT

    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    QByteArray m_bytes;

    // Response information
    QString m_content_type;
    QHash<QString, QString> m_content_type_parameters;

    // Request parameters
    QTimer m_timeout;
    int m_maxRedirects;
    int m_redirects;
    std::chrono::milliseconds m_timeoutDuration;
    QList<QPair<QString, QString>> m_headers;
    QUrl m_url;

public:
    // TODO Disallow insecure connections by default?
    explicit NetworkRequest(int maxRedirects,
                            std::chrono::milliseconds timeoutDuration,
                            QList<QPair<QString, QString>> headers,
                            QNetworkAccessManager* manager = nullptr);
    ~NetworkRequest() override;

    void setMaxRedirects(int maxRedirects);
    void setTimeout(std::chrono::milliseconds timeoutDuration);

    // TODO Should it be single shot vs multiple shot?
    void fetch(const QUrl& url);
    void cancel();

    QUrl url() const;
    /**
     * @return The MIME Type set in the Content-Type header. Empty string if Content-Type was not set.
     */
    const QString& ContentType() const;
    /**
     * @return Any parameters set in the Content-Type header.
     */
    const QHash<QString, QString>& ContentTypeParameters() const;

private:
    void reset();
private slots:
    void fetchFinished();
    void fetchReadyRead();
    void fetchTimeout();

signals:
    void success(QByteArray);
    void failure();
};

/**
 * Creates a NetworkRequest with default parameters.
 * @param maxRedirects
 * @param timeoutDuration
 * @param headers
 * @param manager
 * @return
 */
NetworkRequest createRequest(int maxRedirects = 5,
                             std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(5000),
                             QList<QPair<QString, QString>> additionalHeaders = {},
                             QNetworkAccessManager* manager = nullptr);

#endif // KEEPASSXC_NETWORKREQUEST_H
