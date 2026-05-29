/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "TestSyncEngine.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "config-keepassx-tests.h"
#include "core/Database.h"
#include "crypto/Crypto.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "remotesync/RemoteSyncParams.h"
#include "remotesync/RemoteSyncProvider.h"
#include "remotesync/SyncEngine.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestSyncEngine)

namespace
{
    const QString g_dbFile = QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx");
    const QString g_dbFileDifferentPassword =
        QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/SyncDatabaseDifferentPassword.kdbx");

    // In-test test-double for RemoteSyncProvider. Records call counts and
    // returns canned results. Lives in the anonymous namespace so it can't
    // leak into other translation units.
    class StubSyncProvider : public RemoteSyncProvider
    {
    public:
        explicit StubSyncProvider(QObject* parent = nullptr)
            : RemoteSyncProvider(parent)
        {
        }

        RemoteHandler::RemoteResult download(const RemoteSyncParams*) override
        {
            ++downloadCalls;
            return downloadResult;
        }
        RemoteHandler::RemoteResult upload(const QString&, const RemoteSyncParams*) override
        {
            ++uploadCalls;
            return uploadResult;
        }
        RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams*) override
        {
            ++refreshAuthCalls;
            return refreshAuthResult;
        }
        void abort() override
        {
            ++abortCalls;
        }
        QString displayName() const override
        {
            return QStringLiteral("Stub");
        }
        RemoteSyncParams* createParams() const override
        {
            return new CommandSyncParams;
        }
        bool applyRefreshedTokens(const QString&, RemoteSyncParams*) override
        {
            ++applyTokenCalls;
            return applyTokenResult;
        }

        RemoteHandler::RemoteResult downloadResult{.success = true};
        RemoteHandler::RemoteResult uploadResult{.success = true};
        RemoteHandler::RemoteResult refreshAuthResult{.success = true};
        int downloadCalls = 0;
        int uploadCalls = 0;
        int refreshAuthCalls = 0;
        int abortCalls = 0;
        int applyTokenCalls = 0;
        bool applyTokenResult = true;
    };

    QSharedPointer<CompositeKey> makeKey(const QString& password)
    {
        auto key = QSharedPointer<CompositeKey>::create();
        key->addKey(QSharedPointer<PasswordKey>::create(password));
        return key;
    }

    // Open a fresh Database from a temp copy of `g_dbFile` (password "a").
    // The TemporaryFile object outlives the function via the returned handle.
    QSharedPointer<Database> openTempDb(TemporaryFile& tempFile)
    {
        bool copied = tempFile.copyFromFile(g_dbFile);
        Q_ASSERT(copied);
        Q_UNUSED(copied);

        auto db = QSharedPointer<Database>::create();
        QString error;
        bool ok = db->open(tempFile.fileName(), makeKey(QStringLiteral("a")), &error);
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        return db;
    }

    // A SaveFn that does a real Database::save and reports success/error.
    SyncEngine::SaveFn makeRealSaveFn(const QSharedPointer<Database>& db)
    {
        return [db](QString& errorMessage) { return db->save(Database::Atomic, {}, &errorMessage); };
    }

    // Copy `source` to a freshly-generated path under QDir::tempPath() and
    // return that path. The caller is responsible for deletion (we can't use
    // TemporaryFile here because its destructor unconditionally removes the
    // file -- and several of these tests are explicitly checking that the
    // engine, not the test harness, removed the file).
    QString copyToOwnedTempPath(const QString& source)
    {
        const QString path = QDir::tempPath() + QStringLiteral("/keepassxc-syncengine-test-")
                             + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".kdbx");
        QFile::remove(path); // best-effort, in case of collision
        bool ok = QFile::copy(source, path);
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        return path;
    }
} // namespace

void TestSyncEngine::initTestCase()
{
    QVERIFY(Crypto::init());
    qRegisterMetaType<SyncEngine::State>("SyncEngine::State");
}

// ---------------------------------------------------------------------------
// State machine basics
// ---------------------------------------------------------------------------

void TestSyncEngine::testInitialState_isIdle()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    QCOMPARE(engine.state(), SyncEngine::State::Idle);
    QCOMPARE(engine.lastErrorKind(), RemoteHandler::ErrorKind::Other);
}

void TestSyncEngine::testStartSync_whenAlreadyRunning_returnsFalseAndEmitsError()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    // Re-enter startSync from inside the stateChanged slot while the first
    // call is still in flight (state != Idle). This is the only way to hit
    // the guard since startSync is synchronous start-to-finish.
    bool secondCallReturn = true;
    int secondCallAttempts = 0;
    QObject::connect(&engine, &SyncEngine::stateChanged, [&](SyncEngine::State s) {
        if (s == SyncEngine::State::Authenticating && secondCallAttempts == 0) {
            ++secondCallAttempts;
            secondCallReturn = engine.startSync(&provider, params.data());
        }
    });

    QSignalSpy errorSpy(&engine, &SyncEngine::syncError);

    QVERIFY(engine.startSync(&provider, params.data()));
    QCOMPARE(secondCallAttempts, 1);
    QCOMPARE(secondCallReturn, false);
    QCOMPARE(errorSpy.count(), 1);
}

void TestSyncEngine::testHappyPath_runsToCompletion()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    // First-sync convention: empty filePath means "no remote file yet".
    provider.downloadResult = {.success = true};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(provider.refreshAuthCalls, 1);
    QCOMPARE(provider.downloadCalls, 1);
    QCOMPARE(provider.uploadCalls, 1);
    QCOMPARE(engine.state(), SyncEngine::State::Idle);
}

void TestSyncEngine::testFirstSync_skipsMerge()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    provider.downloadResult = {.success = true}; // first-sync (empty filePath)
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy stateSpy(&engine, &SyncEngine::stateChanged);

    QVERIFY(engine.startSync(&provider, params.data()));

    QList<SyncEngine::State> states;
    for (const auto& args : stateSpy) {
        states.append(args.at(0).value<SyncEngine::State>());
    }
    QVERIFY2(!states.contains(SyncEngine::State::Merging),
             "first-sync (empty filePath) must skip the Merging step entirely");
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

void TestSyncEngine::testRefreshAuthFails_emitsSyncFinishedFalseAndSetsLastErrorKind()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    provider.refreshAuthResult = {
        .success = false, .errorMessage = "boom", .kind = RemoteHandler::ErrorKind::AuthExpired};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    auto args = finishedSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
    QCOMPARE(args.at(1).toString(), QStringLiteral("boom"));
    QCOMPARE(engine.lastErrorKind(), RemoteHandler::ErrorKind::AuthExpired);
    QCOMPARE(provider.downloadCalls, 0);
    QCOMPARE(provider.uploadCalls, 0);
    QCOMPARE(engine.state(), SyncEngine::State::Idle);
}

void TestSyncEngine::testDownloadFails_emitsSyncFinishedFalseAndSetsLastErrorKind()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    provider.downloadResult = {.success = false, .errorMessage = "net down", .kind = RemoteHandler::ErrorKind::Network};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), false);
    QCOMPARE(engine.lastErrorKind(), RemoteHandler::ErrorKind::Network);
    QCOMPARE(provider.uploadCalls, 0);
    QCOMPARE(engine.state(), SyncEngine::State::Idle);
}

void TestSyncEngine::testUploadFails_emitsSyncFinishedFalseAndSetsLastErrorKind()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    // first-sync (no merge) -> save -> upload-fail
    provider.uploadResult = {
        .success = false, .errorMessage = "server boom", .kind = RemoteHandler::ErrorKind::ServerError};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), false);
    QCOMPARE(engine.lastErrorKind(), RemoteHandler::ErrorKind::ServerError);

    // Local save MUST have succeeded -- upload failure does not roll the
    // save back. Verify the .kdbx on disk is parseable with the original key.
    auto reopened = QSharedPointer<Database>::create();
    QString err;
    bool ok = reopened->open(tempDb.fileName(), makeKey(QStringLiteral("a")), &err);
    QVERIFY2(ok, qPrintable(err));
}

void TestSyncEngine::testSaveFails_emitsSyncFinishedFalse()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);

    SyncEngine::SaveFn failingSave = [](QString& err) {
        err = QStringLiteral("disk full");
        return false;
    };
    SyncEngine engine(db, failingSave);

    StubSyncProvider provider;
    provider.downloadResult = {.success = true}; // first-sync -> straight to save
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    auto args = finishedSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
    QVERIFY(args.at(1).toString().contains(QStringLiteral("disk full")));
    QCOMPARE(provider.uploadCalls, 0);
}

// ---------------------------------------------------------------------------
// Cancel semantics
// ---------------------------------------------------------------------------

void TestSyncEngine::testCancel_betweenRefreshAuthAndDownload()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QObject::connect(&engine, &SyncEngine::stateChanged, [&](SyncEngine::State s) {
        if (s == SyncEngine::State::Authenticating) {
            engine.cancel();
        }
    });

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    auto args = finishedSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
    QVERIFY(args.at(1).toString().contains(QStringLiteral("cancelled"), Qt::CaseInsensitive));
    QCOMPARE(provider.downloadCalls, 0);
}

void TestSyncEngine::testCancel_inIdle_isNoop()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);
    QSignalSpy errorSpy(&engine, &SyncEngine::syncError);
    QSignalSpy stateSpy(&engine, &SyncEngine::stateChanged);

    engine.cancel();

    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(engine.state(), SyncEngine::State::Idle);
}

// ---------------------------------------------------------------------------
// applyRefreshedTokens
// ---------------------------------------------------------------------------

void TestSyncEngine::testApplyRefreshedTokens_failureSurfacesAsAuthError()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    // Non-empty stdOutput so SyncEngine attempts applyRefreshedTokens.
    provider.refreshAuthResult = {.success = true, .stdOutput = QStringLiteral("malformed-json{")};
    provider.applyTokenResult = false;
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    auto args = finishedSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
    QVERIFY(args.at(1).toString().contains(QStringLiteral("Re-authorize")));
    QCOMPARE(provider.downloadCalls, 0);
}

void TestSyncEngine::testRefreshedTokenData_signalFiresOnRefreshSuccess()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    const QString tokenJson = QStringLiteral("{\"accessToken\":\"new\"}");
    provider.refreshAuthResult = {.success = true, .stdOutput = tokenJson};
    provider.applyTokenResult = true;
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy tokenSpy(&engine, &SyncEngine::refreshedTokenData);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(tokenSpy.count(), 1);
    QCOMPARE(tokenSpy.takeFirst().at(0).toString(), tokenJson);
}

// ---------------------------------------------------------------------------
// remoteDbNeedsKey (key mismatch)
// ---------------------------------------------------------------------------

void TestSyncEngine::testRemoteDbNeedsKey_whenLocalAndRemoteKeysMismatch()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    // The "remote" file: a copy of SyncDatabaseDifferentPassword.kdbx
    // (password "b"). Managed manually -- we need to assert it still exists
    // after the engine's hand-off signal, so it must outlive any RAII wrapper.
    const QString remotePath = copyToOwnedTempPath(g_dbFileDifferentPassword);

    StubSyncProvider provider;
    provider.downloadResult = {.success = true, .filePath = remotePath};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy needsKeySpy(&engine, &SyncEngine::remoteDbNeedsKey);

    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(needsKeySpy.count(), 1);
    const QString handedOffPath = needsKeySpy.takeFirst().at(0).toString();
    QCOMPARE(handedOffPath, remotePath);
    // CRITICAL: file MUST still exist -- ownership was transferred to the
    // receiver and the engine destructor must not race to delete it.
    QVERIFY2(QFile::exists(handedOffPath), "remoteDbNeedsKey hand-off must leave the file in place for the receiver");
    QCOMPARE(engine.state(), SyncEngine::State::Idle);
    QCOMPARE(provider.uploadCalls, 0);

    QFile::remove(handedOffPath);
}

// ---------------------------------------------------------------------------
// syncPreviousKey integration
// ---------------------------------------------------------------------------

void TestSyncEngine::testClearSyncPreviousKey_onSuccessfulUpload()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);

    auto prev = makeKey(QStringLiteral("previous"));
    db->setSyncPreviousKey(prev);
    QVERIFY(db->syncPreviousKey());

    SyncEngine engine(db, makeRealSaveFn(db));
    StubSyncProvider provider;
    provider.downloadResult = {.success = true}; // first-sync, no merge
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);
    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), true);
    QVERIFY2(!db->syncPreviousKey(), "successful upload must clear syncPreviousKey");
}

// ---------------------------------------------------------------------------
// Temp file cleanup
// ---------------------------------------------------------------------------

void TestSyncEngine::testTempFileRemovedOnSuccess()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    // Make a real temp .kdbx (copy of NewDatabase.kdbx, same key "a") so the
    // merge step opens cleanly. Manual path management -- we want to verify
    // that the engine's cleanup() (not any RAII wrapper) removed it.
    const QString downloadedPath = copyToOwnedTempPath(g_dbFile);

    StubSyncProvider provider;
    provider.downloadResult = {.success = true, .filePath = downloadedPath};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);
    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), true);
    QVERIFY2(!QFile::exists(downloadedPath), "cleanup() must remove the downloaded temp file on success");
    QFile::remove(downloadedPath); // safety net if assertion fails
}

void TestSyncEngine::testTempFileRemovedOnFailure()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    const QString downloadedPath = copyToOwnedTempPath(g_dbFile);

    StubSyncProvider provider;
    provider.downloadResult = {.success = true, .filePath = downloadedPath};
    provider.uploadResult = {.success = false, .errorMessage = "boom", .kind = RemoteHandler::ErrorKind::ServerError};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy finishedSpy(&engine, &SyncEngine::syncFinished);
    QVERIFY(engine.startSync(&provider, params.data()));

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), false);
    QVERIFY2(!QFile::exists(downloadedPath), "cleanup() runs even on upload failure -- temp file must be removed");
    QFile::remove(downloadedPath); // safety net if assertion fails
}

// ---------------------------------------------------------------------------
// State change signal sequence
// ---------------------------------------------------------------------------

void TestSyncEngine::testStateChangeSequence_happyPath()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    StubSyncProvider provider;
    provider.downloadResult = {.success = true}; // first-sync, no merge
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy stateSpy(&engine, &SyncEngine::stateChanged);

    QVERIFY(engine.startSync(&provider, params.data()));

    QList<SyncEngine::State> states;
    for (const auto& args : stateSpy) {
        states.append(args.at(0).value<SyncEngine::State>());
    }
    const QList<SyncEngine::State> expected{
        SyncEngine::State::Authenticating,
        SyncEngine::State::Downloading,
        SyncEngine::State::Saving,
        SyncEngine::State::Uploading,
        SyncEngine::State::Idle,
    };
    QCOMPARE(states, expected);
}

void TestSyncEngine::testStateChangeSequence_withMerge()
{
    TemporaryFile tempDb;
    auto db = openTempDb(tempDb);
    SyncEngine engine(db, makeRealSaveFn(db));

    // Real remote file (same key) so the merge step is reached.
    const QString downloadedPath = copyToOwnedTempPath(g_dbFile);

    StubSyncProvider provider;
    provider.downloadResult = {.success = true, .filePath = downloadedPath};
    QScopedPointer<RemoteSyncParams> params(provider.createParams());

    QSignalSpy stateSpy(&engine, &SyncEngine::stateChanged);

    QVERIFY(engine.startSync(&provider, params.data()));

    QList<SyncEngine::State> states;
    for (const auto& args : stateSpy) {
        states.append(args.at(0).value<SyncEngine::State>());
    }
    const QList<SyncEngine::State> expected{
        SyncEngine::State::Authenticating,
        SyncEngine::State::Downloading,
        SyncEngine::State::Merging,
        SyncEngine::State::Saving,
        SyncEngine::State::Uploading,
        SyncEngine::State::Idle,
    };
    QCOMPARE(states, expected);

    // Engine should have cleaned the temp file too.
    QVERIFY(!QFile::exists(downloadedPath));
    QFile::remove(downloadedPath); // safety net if assertion fails
}
