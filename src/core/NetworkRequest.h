
#ifndef KEEPASSXC_NETWORKREQUEST_H
#define KEEPASSXC_NETWORKREQUEST_H

#include <QObject>
#include <QHash>
#include <QTimer>
class QNetworkReply;
class QNetworkAccessManager;

class NetworkRequest : public QObject {
        Q_OBJECT

        QNetworkAccessManager* m_manager;
        QNetworkReply* m_reply;
        QByteArray m_bytes;

        QString m_content_type;
        QHash<QString, QString> m_content_type_parameters;
        QTimer m_timeout;

        int m_maxRedirects;
        int m_redirects;
        std::chrono::milliseconds m_timeoutDuration;
    public:
        explicit NetworkRequest(int maxRedirects, std::chrono::milliseconds timeoutDuration, QNetworkAccessManager* manager = nullptr);
        ~NetworkRequest() override;

        void setMaxRedirects(int maxRedirects);
        void setTimeout(std::chrono::milliseconds timeoutDuration);

        void fetch(const QUrl& url);
        void cancel();

        QUrl url() const;
        /***
         * @return The MIME Type set in the Content-Type header. Empty string if Content-Type was not set.
         */
        const QString& ContentType() const;
        /***
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

#endif // KEEPASSXC_NETWORKREQUEST_H
