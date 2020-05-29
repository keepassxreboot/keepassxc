/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_ICONDOWNLOADER_H
#define KEEPASSXC_ICONDOWNLOADER_H

#include <QImage>
#include <QObject>
#include <QTimer>
#include <QUrl>

#include "core/Global.h"

class QNetworkReply;

class IconDownloader : public QObject
{
    Q_OBJECT

public:
    explicit IconDownloader(QObject* parent = nullptr);
    ~IconDownloader() override;

    void setUrl(const QString& entryUrl);
    void download();

signals:
    void finished(const QString& entryUrl, const QImage& image);

public slots:
    void abortDownload();

private slots:
    void fetchFinished();
    void fetchReadyRead();

private:
    void fetchFavicon(const QUrl& url);
    QImage parseImage(QByteArray& imageBytes) const;

    QString m_url;
    QUrl m_fetchUrl;
    QList<QUrl> m_urlsToTry;
    QByteArray m_bytesReceived;
    QNetworkReply* m_reply;
    QTimer m_timeout;
    int m_redirects;
};

#endif // KEEPASSXC_ICONDOWNLOADER_H