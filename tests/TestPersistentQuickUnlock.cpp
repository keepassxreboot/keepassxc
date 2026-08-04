/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 */

#include "TestPersistentQuickUnlock.h"

#include "crypto/CryptoHash.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/Key.h"
#include "keys/PasswordKey.h"
#include "quickunlock/PersistentQuickUnlock.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace
{
    class UnsupportedKey : public Key
    {
    public:
        UnsupportedKey()
            : Key(QUuid(QStringLiteral("{4e5518ae-028d-42a6-a913-e7130d61eb5d}")))
        {
        }

        QByteArray rawKey() const override
        {
            return QByteArray(32, '\x2a');
        }

        void setRawKey(const QByteArray&) override
        {
        }

        QByteArray serialize() const override
        {
            return QByteArrayLiteral("unsupported-key");
        }

        void deserialize(const QByteArray&) override
        {
        }
    };

    class FakeProtectionBackend : public PersistentQuickUnlockBackend
    {
    public:
        bool isAvailable() const override
        {
            return available;
        }

        bool protect(const QByteArray& data, QByteArray& protectedData) override
        {
            if (cancel) {
                error = QStringLiteral("Authentication canceled");
                return false;
            }
            protectedData = QByteArrayLiteral("FAKE") + data + CryptoHash::hash(data, CryptoHash::Sha256);
            return true;
        }

        bool unprotect(const QByteArray& protectedData, QByteArray& data) override
        {
            data.clear();
            if (cancel) {
                error = QStringLiteral("Authentication canceled");
                return false;
            }
            if (!protectedData.startsWith(QByteArrayLiteral("FAKE")) || protectedData.size() <= 36) {
                error = QStringLiteral("Invalid protected data");
                return false;
            }
            const auto payload = protectedData.mid(4, protectedData.size() - 36);
            const auto digest = protectedData.right(32);
            if (CryptoHash::hash(payload, CryptoHash::Sha256) != digest) {
                error = QStringLiteral("Protected data authentication failed");
                return false;
            }
            data = payload;
            return true;
        }

        QString errorString() const override
        {
            return error;
        }

        bool available = true;
        bool cancel = false;
        QString error;
    };

    QSharedPointer<CompositeKey> passwordKey(const QString& password = QStringLiteral("unit-test-password"))
    {
        auto key = QSharedPointer<CompositeKey>::create();
        key->addKey(QSharedPointer<PasswordKey>::create(password));
        return key;
    }

    QSharedPointer<FileKey> fileKey()
    {
        auto key = QSharedPointer<FileKey>::create();
        key->setRawKey(QByteArray(32, '\x5a'));
        return key;
    }

    QString recordPath(const QString& directory, const QUuid& uuid)
    {
        return QDir(directory).filePath(uuid.toString(QUuid::WithoutBraces) + QStringLiteral(".pqu"));
    }
} // namespace

void TestPersistentQuickUnlock::testPersistAndReload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FakeProtectionBackend backend;
    const auto uuid = QUuid::createUuid();
    const auto expected = passwordKey();

    {
        PersistentQuickUnlock writer(&backend, directory.path());
        QVERIFY(writer.setKey(uuid, expected));
        QVERIFY(writer.hasKey(uuid));
    }

    PersistentQuickUnlock afterRestart(&backend, directory.path());
    QSharedPointer<CompositeKey> actual;
    QVERIFY(afterRestart.getKey(uuid, actual));
    QVERIFY(actual);
    QCOMPARE(actual->serialize(), expected->serialize());
}

void TestPersistentQuickUnlock::testLegacyPasswordRecord()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FakeProtectionBackend backend;
    const auto uuid = QUuid::createUuid();
    const auto expected = passwordKey();

    QByteArray envelope;
    QDataStream stream(&envelope, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << QByteArrayLiteral("KPXC-PERSISTENT-QUICK-UNLOCK") << quint32(1) << uuid.toRfc4122()
           << QByteArrayLiteral("password-composite-key") << expected->serialize();

    QByteArray protectedData;
    QVERIFY(backend.protect(envelope, protectedData));
    QFile record(recordPath(directory.path(), uuid));
    QVERIFY(record.open(QIODevice::WriteOnly));
    QCOMPARE(record.write(protectedData), protectedData.size());
    record.close();

    PersistentQuickUnlock unlock(&backend, directory.path());
    QSharedPointer<CompositeKey> actual;
    QVERIFY(unlock.getKey(uuid, actual));
    QCOMPARE(actual->serialize(), expected->serialize());
}

void TestPersistentQuickUnlock::testDatabaseBinding()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());
    const auto firstUuid = QUuid::createUuid();
    const auto secondUuid = QUuid::createUuid();
    QVERIFY(unlock.setKey(firstUuid, passwordKey()));
    QVERIFY(QFile::copy(recordPath(directory.path(), firstUuid), recordPath(directory.path(), secondUuid)));

    QSharedPointer<CompositeKey> key;
    QVERIFY(!unlock.getKey(secondUuid, key));
    QVERIFY(!key);
    QVERIFY(unlock.errorString().contains(QStringLiteral("another database")));
}

void TestPersistentQuickUnlock::testTamperedPayload()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());
    const auto uuid = QUuid::createUuid();
    QVERIFY(unlock.setKey(uuid, passwordKey()));

    QFile record(recordPath(directory.path(), uuid));
    QVERIFY(record.open(QIODevice::ReadWrite));
    auto data = record.readAll();
    QVERIFY(data.size() > 8);
    data[8] = static_cast<char>(data[8] ^ 0x55);
    QVERIFY(record.seek(0));
    QCOMPARE(record.write(data), data.size());
    record.close();

    QSharedPointer<CompositeKey> key;
    QVERIFY(!unlock.getKey(uuid, key));
    QVERIFY(!key);
}

void TestPersistentQuickUnlock::testResetAndCredentialChange()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());
    const auto uuid = QUuid::createUuid();
    QVERIFY(unlock.setKey(uuid, passwordKey(QStringLiteral("old-password"))));

    // Credential changes invalidate the old record before a new key can be enrolled.
    QVERIFY(unlock.reset(uuid));
    QVERIFY(!unlock.hasKey(uuid));
    QSharedPointer<CompositeKey> key;
    QVERIFY(!unlock.getKey(uuid, key));

    const auto replacement = passwordKey(QStringLiteral("new-password"));
    QVERIFY(unlock.setKey(uuid, replacement));
    QVERIFY(unlock.getKey(uuid, key));
    QCOMPARE(key->serialize(), replacement->serialize());
}

void TestPersistentQuickUnlock::testCompositeKeyVariants()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());

    QList<QSharedPointer<CompositeKey>> variants;
    variants << QSharedPointer<CompositeKey>::create();
    variants << passwordKey(QString{});

    auto fileOnly = QSharedPointer<CompositeKey>::create();
    fileOnly->addKey(fileKey());
    variants << fileOnly;

    auto passwordAndFile = passwordKey(QString{});
    passwordAndFile->addKey(fileKey());
    variants << passwordAndFile;

    auto challengeResponse = passwordKey();
    challengeResponse->addChallengeResponseKey(QSharedPointer<ChallengeResponseKey>::create());
    variants << challengeResponse;

    for (const auto& expected : variants) {
        const auto uuid = QUuid::createUuid();
        QVERIFY(unlock.setKey(uuid, expected));
        QSharedPointer<CompositeKey> actual;
        QVERIFY(unlock.getKey(uuid, actual));
        QCOMPARE(actual->serialize(), expected->serialize());
    }
}

void TestPersistentQuickUnlock::testUnsupportedKey()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());
    const auto uuid = QUuid::createUuid();

    auto unsupportedKey = QSharedPointer<CompositeKey>::create();
    unsupportedKey->addKey(QSharedPointer<UnsupportedKey>::create());
    QVERIFY(!unlock.setKey(uuid, unsupportedKey));
    QVERIFY(!unlock.hasKey(uuid));
}

void TestPersistentQuickUnlock::testCanceledAuthentication()
{
    QTemporaryDir directory;
    FakeProtectionBackend backend;
    PersistentQuickUnlock unlock(&backend, directory.path());
    const auto uuid = QUuid::createUuid();
    QVERIFY(unlock.setKey(uuid, passwordKey()));

    backend.cancel = true;
    QSharedPointer<CompositeKey> key;
    QVERIFY(!unlock.getKey(uuid, key));
    QVERIFY(!key);
    QVERIFY(unlock.hasKey(uuid));
    QCOMPARE(unlock.errorString(), QStringLiteral("Authentication canceled"));
}

void TestPersistentQuickUnlock::testDisabledByDefault()
{
    QTemporaryDir directory;
    PersistentQuickUnlock unlock(nullptr, directory.path());
    QVERIFY(!unlock.isAvailable());
    QVERIFY(!unlock.setKey(QUuid::createUuid(), passwordKey()));
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files).size(), 0);
}

QTEST_GUILESS_MAIN(TestPersistentQuickUnlock)
