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

#include "TestRemoteSync.h"

#include "config-keepassx-tests.h"
#include "config-keepassx.h"

#include "mock/MockRemoteProcess.h"
#include "remotesync/CommandSyncProvider.h"
#include "remotesync/RemoteSyncParams.h"
#include "remotesync/RemoteSyncProvider.h"

#include "gui/remote/RemoteHandler.h"
#include "gui/remote/RemoteProcess.h"

#include <QFile>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTest>

QTEST_GUILESS_MAIN(TestRemoteSync)

namespace
{
    // In-test stub: a concrete subclass that only overrides the pure virtuals,
    // leaving every optional virtual at the base-class default. Used to assert
    // the default behavior of those optional virtuals.
    class StubProvider : public RemoteSyncProvider
    {
    public:
        explicit StubProvider(QObject* parent = nullptr)
            : RemoteSyncProvider(parent)
        {
        }

        RemoteHandler::RemoteResult download(const RemoteSyncParams*) override
        {
            return RemoteHandler::RemoteResult{true, {}, {}, {}, {}};
        }
        RemoteHandler::RemoteResult upload(const QString&, const RemoteSyncParams*) override
        {
            return RemoteHandler::RemoteResult{true, {}, {}, {}, {}};
        }
        RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams*) override
        {
            return RemoteHandler::RemoteResult{true, {}, {}, {}, {}};
        }
        void abort() override
        {
        }
        QString displayName() const override
        {
            return QStringLiteral("stub");
        }
        RemoteSyncParams* createParams() const override
        {
            return new RemoteSyncParams();
        }
    };
} // namespace

void TestRemoteSync::cleanup()
{
    // Always clear the test override so tests can't pollute each other through
    // the static factory-override slot.
    RemoteSyncProvider::clearFactoryOverrideForTest();
}

void TestRemoteSync::testFactoryDispatch_command()
{
    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("command"), nullptr));
    QVERIFY(p);
    QCOMPARE(p->displayName(), QStringLiteral("Command"));
    QVERIFY(qobject_cast<CommandSyncProvider*>(p.data()) != nullptr);
}

#ifdef KPXC_FEATURE_NETWORK
void TestRemoteSync::testFactoryDispatch_dropbox()
{
    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("dropbox"), nullptr));
    QVERIFY(p);
    QCOMPARE(p->displayName(), QStringLiteral("Dropbox"));
}

void TestRemoteSync::testFactoryDispatch_nextcloud()
{
    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("nextcloud"), nullptr));
    QVERIFY(p);
    QCOMPARE(p->displayName(), QStringLiteral("Nextcloud"));
}
#endif

void TestRemoteSync::testFactoryDispatch_unknown()
{
    // Source emits a qWarning for unknown types; suppress it so the test log
    // is clean.
    QTest::ignoreMessage(QtWarningMsg, "RemoteSyncProvider: Unknown provider type 'totally-bogus'");
    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("totally-bogus"), nullptr));
    QVERIFY(p.isNull());
}

void TestRemoteSync::testFactoryOverride_routesThroughOverride()
{
    RemoteSyncProvider::setFactoryOverrideForTest(
        [](const QString&, QObject* parent) -> RemoteSyncProvider* { return new StubProvider(parent); });

    {
        QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("anything"), nullptr));
        QVERIFY(p);
        QVERIFY(dynamic_cast<StubProvider*>(p.data()) != nullptr);
        // It also bypassed default dispatch entirely: a real "command" would have
        // been a CommandSyncProvider, not a StubProvider.
        QVERIFY(qobject_cast<CommandSyncProvider*>(p.data()) == nullptr);
    }

    RemoteSyncProvider::clearFactoryOverrideForTest();

    // After clearing, default dispatch must come back.
    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("command"), nullptr));
    QVERIFY(p);
    QVERIFY(qobject_cast<CommandSyncProvider*>(p.data()) != nullptr);
}

void TestRemoteSync::testFactoryOverride_nullptrFallsThrough()
{
    // An override that returns nullptr means "I don't handle this; use the default".
    // The factory must NOT short-circuit and return nullptr to the caller.
    RemoteSyncProvider::setFactoryOverrideForTest(
        [](const QString&, QObject*) -> RemoteSyncProvider* { return nullptr; });

    QScopedPointer<RemoteSyncProvider> p(RemoteSyncProvider::create(QStringLiteral("command"), nullptr));
    QVERIFY(p);
    QVERIFY(qobject_cast<CommandSyncProvider*>(p.data()) != nullptr);
}

void TestRemoteSync::testDefaultVirtuals()
{
    StubProvider provider;

    // classifyError defaults to Other regardless of the message text.
    QCOMPARE(provider.classifyError(QStringLiteral("anything")), RemoteSyncProvider::ErrorKind::Other);
    QCOMPARE(provider.classifyError(QStringLiteral("401 unauthorized")), RemoteSyncProvider::ErrorKind::Other);

    // isAuthorized defaults to false (fail-closed).
    QCOMPARE(provider.isAuthorized(QJsonObject{}), false);

    // applyRefreshedTokens defaults to no-op success.
    QScopedPointer<RemoteSyncParams> params(provider.createParams());
    QVERIFY(params);
    QCOMPARE(provider.applyRefreshedTokens(QStringLiteral("anything"), params.data()), true);

    // buildParamsFromConfig defaults to createParams() (non-null).
    QScopedPointer<RemoteSyncParams> built(provider.buildParamsFromConfig(QJsonObject{}));
    QVERIFY(built);

    // persistRefreshedTokens is a no-op; passing nullptr settings must not crash.
    provider.persistRefreshedTokens(QStringLiteral("anything"), QStringLiteral("key"), nullptr);
}

void TestRemoteSync::testCommand_createParams_returnsCommandSyncParams()
{
    CommandSyncProvider provider;
    QScopedPointer<RemoteSyncParams> params(provider.createParams());
    QVERIFY(params);
    // Critical: download()/upload() static_cast<const CommandSyncParams*> the
    // params they receive. If createParams() ever returned a bare RemoteSyncParams,
    // downstream code would read undefined fields. Pin it.
    QVERIFY(dynamic_cast<CommandSyncParams*>(params.data()) != nullptr);
}

void TestRemoteSync::testCommand_refreshAuth_isNoopSuccess()
{
    CommandSyncProvider provider;
    // refreshAuth ignores its params arg for command providers, so nullptr is fine.
    auto result = provider.refreshAuth(nullptr);
    QCOMPARE(result.success, true);
    QVERIFY(result.errorMessage.isEmpty());
    QVERIFY(result.stdOutput.isEmpty());
}

void TestRemoteSync::testCommand_displayName()
{
    CommandSyncProvider provider;
    QCOMPARE(provider.displayName(), QStringLiteral("Command"));
}

void TestRemoteSync::testCommand_downloadDelegatesToRemoteHandler()
{
    // Wire RemoteHandler to the mock process. MockRemoteProcess::start() copies
    // a real kdbx file to the temp-file location, which is what RemoteHandler::download
    // checks for existence + non-zero size to declare success.
    const QString sourceDb = QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/SyncDatabase.kdbx");
    RemoteHandler::setRemoteProcessFunc([sourceDb](QObject* parent) {
        return QScopedPointer<RemoteProcess>(new MockRemoteProcess(parent, sourceDb));
    });

    CommandSyncProvider provider;
    CommandSyncParams params;
    params.name = QStringLiteral("test");
    params.downloadCommand = QStringLiteral("fake-cmd");
    params.downloadInput = QStringLiteral("foo");
    params.downloadTimeoutMsec = 10000;

    auto result = provider.download(&params);

    // Verifies CommandSyncProvider::download delegates to RemoteHandler via
    // the mock process, which populates the temp file on success.
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(!result.filePath.isEmpty());
    QVERIFY(QFile::exists(result.filePath));

    // Cleanup: remove the temp file the handler created and reset the
    // process-factory back to the default so subsequent tests aren't poisoned.
    QFile::remove(result.filePath);
    RemoteHandler::setRemoteProcessFunc([](QObject* parent) {
        return QScopedPointer<RemoteProcess>(new RemoteProcess(parent));
    });
}
