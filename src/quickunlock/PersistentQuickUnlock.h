/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 */

#ifndef KEEPASSXC_PERSISTENTQUICKUNLOCK_H
#define KEEPASSXC_PERSISTENTQUICKUNLOCK_H

#include <QByteArray>
#include <QSharedPointer>
#include <QString>
#include <QUuid>

class CompositeKey;

class PersistentQuickUnlockBackend
{
public:
    virtual ~PersistentQuickUnlockBackend() = default;

    virtual bool isAvailable() const = 0;
    virtual bool protect(const QByteArray& data, QByteArray& protectedData) = 0;
    virtual bool unprotect(const QByteArray& protectedData, QByteArray& data) = 0;
    virtual QString errorString() const = 0;
};

class PersistentQuickUnlock
{
public:
    explicit PersistentQuickUnlock(PersistentQuickUnlockBackend* backend, const QString& storagePath = {});

    bool isAvailable() const;
    bool setKey(const QUuid& dbUuid, const QSharedPointer<const CompositeKey>& key);
    bool getKey(const QUuid& dbUuid, QSharedPointer<CompositeKey>& key);
    bool hasKey(const QUuid& dbUuid) const;
    bool reset(const QUuid& dbUuid);
    QString errorString() const;

private:
    QString recordPath(const QUuid& dbUuid) const;
    QByteArray serializeEnvelope(const QUuid& dbUuid, const CompositeKey& key) const;
    bool deserializeEnvelope(const QUuid& dbUuid, const QByteArray& data, QSharedPointer<CompositeKey>& key);
    static bool isSupported(const CompositeKey& key);

    PersistentQuickUnlockBackend* m_backend;
    QString m_storagePath;
    QString m_error;
};

PersistentQuickUnlock* getPersistentQuickUnlock();

#endif // KEEPASSXC_PERSISTENTQUICKUNLOCK_H
