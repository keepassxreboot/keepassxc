/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 */

#include "PersistentQuickUnlock.h"

#include "keys/ChallengeResponseKey.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "quickunlock/QuickUnlockInterface.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
    constexpr quint32 formatVersion = 2;
    constexpr quint32 legacyFormatVersion = 1;
    constexpr qsizetype maximumRecordSize = 64 * 1024;
    const QByteArray envelopeMagic("KPXC-PERSISTENT-QUICK-UNLOCK");
    const QByteArray payloadType("composite-key");
    const QByteArray legacyPayloadType("password-composite-key");

    void clearBytes(QByteArray& data)
    {
        data.fill('\0');
        data.clear();
    }
} // namespace

PersistentQuickUnlock::PersistentQuickUnlock(PersistentQuickUnlockBackend* backend, const QString& storagePath)
    : m_backend(backend)
    , m_storagePath(storagePath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                                + QStringLiteral("/persistent-quick-unlock")
                                          : storagePath)
{
}

bool PersistentQuickUnlock::isAvailable() const
{
    return m_backend && m_backend->isAvailable() && !m_storagePath.isEmpty();
}

bool PersistentQuickUnlock::setKey(const QUuid& dbUuid, const QSharedPointer<const CompositeKey>& key)
{
    m_error.clear();
    if (!isAvailable() || dbUuid.isNull() || !key) {
        m_error = QObject::tr("Persistent Quick Unlock is not available.");
        return false;
    }
    if (!isSupported(*key)) {
        m_error = QObject::tr("Persistent Quick Unlock does not support this database key composition.");
        return false;
    }

    auto envelope = serializeEnvelope(dbUuid, *key);
    QByteArray protectedData;
    const auto protectedOk = m_backend->protect(envelope, protectedData);
    clearBytes(envelope);
    if (!protectedOk) {
        m_error = m_backend->errorString();
        return false;
    }
    if (protectedData.isEmpty() || protectedData.size() > maximumRecordSize) {
        clearBytes(protectedData);
        m_error = QObject::tr("Windows Hello returned invalid protected key data.");
        return false;
    }

    if (!QDir().mkpath(m_storagePath)) {
        clearBytes(protectedData);
        m_error = QObject::tr("Failed to create persistent Quick Unlock storage.");
        return false;
    }

    QSaveFile file(recordPath(dbUuid));
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        clearBytes(protectedData);
        m_error = QObject::tr("Failed to open persistent Quick Unlock storage.");
        return false;
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const auto protectedSize = protectedData.size();
    const auto bytesWritten = file.write(protectedData);
    clearBytes(protectedData);
    if (bytesWritten != protectedSize || !file.commit()) {
        m_error = QObject::tr("Failed to save persistent Quick Unlock data.");
        return false;
    }
    return true;
}

bool PersistentQuickUnlock::getKey(const QUuid& dbUuid, QSharedPointer<CompositeKey>& key)
{
    key.clear();
    m_error.clear();
    if (!isAvailable() || dbUuid.isNull()) {
        m_error = QObject::tr("Persistent Quick Unlock is not available.");
        return false;
    }

    QFile file(recordPath(dbUuid));
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > maximumRecordSize) {
        m_error = QObject::tr("Persistent Quick Unlock data is missing or invalid.");
        return false;
    }
    auto protectedData = file.readAll();
    file.close();

    QByteArray envelope;
    const auto unprotected = m_backend->unprotect(protectedData, envelope);
    clearBytes(protectedData);
    if (!unprotected) {
        m_error = m_backend->errorString();
        clearBytes(envelope);
        return false;
    }

    const auto decoded = deserializeEnvelope(dbUuid, envelope, key);
    clearBytes(envelope);
    return decoded;
}

bool PersistentQuickUnlock::hasKey(const QUuid& dbUuid) const
{
    if (!isAvailable() || dbUuid.isNull()) {
        return false;
    }
    QFileInfo record(recordPath(dbUuid));
    return record.isFile() && record.size() > 0 && record.size() <= maximumRecordSize;
}

bool PersistentQuickUnlock::reset(const QUuid& dbUuid)
{
    m_error.clear();
    const auto path = recordPath(dbUuid);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return true;
    }
    if (!QFile::remove(path)) {
        m_error = QObject::tr("Failed to remove persistent Quick Unlock data.");
        return false;
    }
    return true;
}

QString PersistentQuickUnlock::errorString() const
{
    return m_error;
}

QString PersistentQuickUnlock::recordPath(const QUuid& dbUuid) const
{
    if (dbUuid.isNull() || m_storagePath.isEmpty()) {
        return {};
    }
    return QDir(m_storagePath).filePath(dbUuid.toString(QUuid::WithoutBraces) + QStringLiteral(".pqu"));
}

QByteArray PersistentQuickUnlock::serializeEnvelope(const QUuid& dbUuid, const CompositeKey& key) const
{
    QByteArray data;
    auto keyData = key.serialize();
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << envelopeMagic << formatVersion << dbUuid.toRfc4122() << payloadType << keyData;
    clearBytes(keyData);
    return data;
}

bool PersistentQuickUnlock::deserializeEnvelope(const QUuid& dbUuid,
                                                const QByteArray& data,
                                                QSharedPointer<CompositeKey>& key)
{
    QByteArray magic;
    QByteArray uuidData;
    QByteArray type;
    QByteArray keyData;
    quint32 version = 0;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_12);
    stream >> magic >> version >> uuidData >> type >> keyData;

    const auto supportedEnvelope = (version == formatVersion && type == payloadType)
                                   || (version == legacyFormatVersion && type == legacyPayloadType);
    if (stream.status() != QDataStream::Ok || !stream.atEnd() || magic != envelopeMagic || !supportedEnvelope
        || uuidData != dbUuid.toRfc4122() || keyData.isEmpty() || keyData.size() > maximumRecordSize) {
        clearBytes(keyData);
        m_error = QObject::tr("Persistent Quick Unlock data is corrupt or belongs to another database.");
        return false;
    }

    auto decodedKey = QSharedPointer<CompositeKey>::create();
    decodedKey->deserialize(keyData);
    auto canonicalData = decodedKey->serialize();
    const auto valid = isSupported(*decodedKey) && canonicalData == keyData;
    clearBytes(canonicalData);
    clearBytes(keyData);
    if (!valid) {
        m_error = QObject::tr("Persistent Quick Unlock contains an unsupported key payload.");
        return false;
    }

    key = decodedKey;
    return true;
}

bool PersistentQuickUnlock::isSupported(const CompositeKey& key)
{
    for (const auto& component : key.keys()) {
        if ((component->uuid() != PasswordKey::UUID && component->uuid() != FileKey::UUID)
            || component->rawKey().size() != 32) {
            return false;
        }
    }

    for (const auto& component : key.challengeResponseKeys()) {
        if (component->uuid() != ChallengeResponseKey::UUID) {
            return false;
        }
    }

    return true;
}

PersistentQuickUnlock* getPersistentQuickUnlock()
{
    static PersistentQuickUnlock instance(dynamic_cast<PersistentQuickUnlockBackend*>(getQuickUnlock()));
    return &instance;
}
