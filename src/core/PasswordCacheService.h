#ifndef PASSWORD_CACHE_SERVICE_H
#define PASSWORD_CACHE_SERVICE_H
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include "core/Entry.h"

enum class DecryptionError{
    initFailed,
    startProcessFailed
};

class GpgDecryptionThread : public QThread
{
    Q_OBJECT
    virtual void run() override;
    QString m_encryptedPassword;
public:
    GpgDecryptionThread(QObject* parent = nullptr):QThread(parent){}
    void setEncryptedPassword(const QString &password);
signals:
    void passwordDecrypted(const QString &s);
    void decryptionError(int error,const QString &extError);
};

class PasswordCacheService : public QObject
{
    Q_OBJECT

public:
    QString value(const Entry& entry) const;
    QString value(const QUuid& key) const;
    QString decodeAndStore(const Entry& entry,const QString& encryptedPassword);
    bool contains(const QUuid& key) const;
    void clear();
    void remove(const QUuid& key);
    static PasswordCacheService& instance();
    QString GpgDecryptPassword(const QString &username,const QString &title,const QString & gpgPassword);
private:
    explicit PasswordCacheService(QObject* parent = nullptr);
    static PasswordCacheService *s_instance;
    bool initialized;
    QString m_returned;
    int m_error;
    QString GpgDecryptPassword(const Entry& entry,const QString & gpgPassword);
    QMap<QUuid, QString> m_passwordMap;
    QPointer<QMessageBox> m_messageBox;
    QPointer<QTimer> m_messageTimer;
    QPointer<QThread> m_workerThread;
private slots : 
    void expirationTimerExpired();
    void touchTimerExpired();
    void passwordDecrypted(const QString &password);
    void passwordDecriptionFailed(int error,const QString &extError);
};
#endif