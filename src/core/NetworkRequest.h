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

#include "config-keepassx.h"

#ifdef WITH_XC_NETWORKING

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
    bool m_finished;

    // Response information
    QString m_content_type;
    QHash<QString, QString> m_content_type_parameters;

    // Request parameters
    QTimer m_timeout;
    unsigned int m_maxRedirects;
    unsigned int m_redirects;
    QList<QPair<QString, QString>> m_headers;
    QUrl m_requested_url;
    bool m_allowInsecure;

public:
    static constexpr auto UNLIMITED_REDIRECTS = std::numeric_limits<unsigned int>::max();

    explicit NetworkRequest(QUrl targetURL, bool allowInsecure, unsigned int maxRedirects,
                            std::chrono::milliseconds timeoutDuration,
                            QList<QPair<QString, QString>> headers,
                            QNetworkAccessManager* manager = nullptr);

    void fetch();

    void cancel();

    /**
     * @return The URL of the original request. Not updated after redirects. Use reply()->url() for the final URL including redirects.
     */
    QUrl URL() const;

    /**
     * @return The QNetworkReply of the final request.
     */
    QNetworkReply* Reply() const;

    /**
     * @return The MIME Type set in the Content-Type header of the last request.
     * Empty string if Content-Type was not set.
     */
    const QString& ContentType() const;

    /**
     * @return Any parameters set in the Content-Type header.
     */
    const QHash<QString, QString>& ContentTypeParameters() const;

    ~NetworkRequest() override;
private:
    void reset();
    void fetch(const QUrl& url);
    void fail();
private slots:
    void fetchFinished();
    void fetchReadyRead();
    void fetchTimeout();

signals:
    void success(QByteArray);
    void failure();
private:
    Q_DISABLE_COPY(NetworkRequest)
};

class NetworkRequestBuilder {
    QUrl m_url;
    bool m_allowInsecure;
    unsigned int m_maxRedirects;
    std::chrono::milliseconds m_timeoutDuration;
    QList<QPair<QString, QString>> m_headers;
    QNetworkAccessManager* m_manager;

    NetworkRequestBuilder();
public:
    explicit NetworkRequestBuilder(QUrl url);

    /**
     * Sets the URL to request.
     * @param url The URL to request.
     */
    NetworkRequestBuilder& setUrl(QUrl url);

    /**
     * Sets whether insecure connections are allowed.
     * @param allowInsecure
     */
    NetworkRequestBuilder& setAllowInsecure(bool allowInsecure);

    /**
     * Sets the maximum number of redirects to follow.
     * @param maxRedirects The maximum number of redirects to follow. Must be >= 0.
     */
    NetworkRequestBuilder& setMaxRedirects(unsigned int maxRedirects);

    /**
     * Sets the timeout duration for the request. This is the maximum time the request may take, including redirects.
     * @param timeoutDuration The duration of the timeout in milliseconds.
     */
    NetworkRequestBuilder& setTimeout(std::chrono::milliseconds timeoutDuration);

    /**
     * Sets the headers to send with the request.
     * @param headers
     */
    NetworkRequestBuilder& setHeaders(QList<QPair<QString, QString>> headers);

    /**
     * Sets the QNetworkAccessManager to use for the request.
     * @param manager
     */
    NetworkRequestBuilder& setManager(QNetworkAccessManager* manager);

    /**
     * Builds a new NetworkRequest based on the configured parameters.
     */
    NetworkRequest build();
};

#endif

#endif // KEEPASSXC_NETWORKREQUEST_H
