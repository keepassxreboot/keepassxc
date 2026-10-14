/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkReply>
#include <QUrl>

/**
 * Standalone HTTPS coordination provider (proof of concept).
 * This is not wired to KeePassXC database open/save lifecycle yet.
 * Never mark a database clean if publish() returns false.
 */
class KdbxCoordinatorClient : public QObject
{
    Q_OBJECT

public:
    KdbxCoordinatorClient(const QUrl& service,
                          const QString& databaseId,
                          const QByteArray& accessToken,
                          const QString& owner,
                          QObject* parent = nullptr);
    ~KdbxCoordinatorClient() override;

    // Blocking operations. Caller MUST use a worker thread, not the Qt GUI thread.
    bool acquire(QString* error);
    bool download(QByteArray* encryptedDatabase, bool* exists, QString* error);
    bool publish(const QByteArray& encryptedDatabase, QString* error);
    bool renew(QString* error);
    bool release(QString* error);
    bool hasLease() const { return !m_lease.isEmpty() && !m_lost; }
    bool isBusy() const { return m_busy; }

signals:
    // Emitted once whenever a held editing lease becomes unusable.
    void leaseLost();

private:
    struct Response {
        int status = 0;
        QByteArray bytes;
        QByteArray etag;
        QString transportError;
    };
    Response request(const QByteArray& method,
                     const QString& action,
                     const QByteArray& payload = {},
                     const QByteArray& lease = {},
                     const QByteArray& expectedRevision = {});
    static QByteArray field(const QByteArray& body, const QString& key);
    static QByteArray hash(const QByteArray& bytes);
    void invalidate();

    QUrl m_service;
    QString m_databaseId;
    QByteArray m_accessToken;
    QString m_owner;
    QNetworkAccessManager m_network;
    QTimer m_renewTimer;
    QByteArray m_lease;
    QByteArray m_revision;
    bool m_lost = false;
    bool m_busy = false;
};
