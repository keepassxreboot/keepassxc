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

#include "RemoteSyncProvider.h"

#include "config-keepassx.h"

#include "CommandSyncProvider.h"
#ifdef KPXC_FEATURE_NETWORK
#include "DropboxSyncProvider.h"
#include "NextcloudSyncProvider.h"
#endif

#ifdef QT_TEST_LIB
#include "tests/mock/MockSyncProvider.h"
#endif

#include <QDebug>
#include <QJsonObject>

namespace
{
    RemoteSyncProvider::FactoryOverride& factoryOverride()
    {
        static RemoteSyncProvider::FactoryOverride instance;
        return instance;
    }
} // namespace

RemoteSyncProvider::RemoteSyncProvider(QObject* parent)
    : QObject(parent)
{
}

void RemoteSyncProvider::setFactoryOverrideForTest(FactoryOverride factory)
{
    factoryOverride() = std::move(factory);
}

void RemoteSyncProvider::clearFactoryOverrideForTest()
{
    factoryOverride() = nullptr;
}

RemoteSyncParams* RemoteSyncProvider::buildParamsFromConfig(const QJsonObject& config) const
{
    // Default: return a bare-allocated params struct of the correct subclass with
    // no fields populated. Providers SHOULD override to lift their type-specific
    // fields out of the persisted config. Caller takes ownership.
    Q_UNUSED(config)
    return createParams();
}

bool RemoteSyncProvider::applyRefreshedTokens(const QString& stdOutput, RemoteSyncParams* params)
{
    // Default: no-op success. Providers without token refresh (e.g. command, app-password
    // providers) inherit this; providers with OAuth-style refresh override.
    Q_UNUSED(stdOutput)
    Q_UNUSED(params)
    return true;
}

RemoteSyncProvider::ErrorKind RemoteSyncProvider::classifyError(const QString& errorMessage) const
{
    // Default: Other -- no false positives from accidental keyword matches.
    // Providers MUST override to surface auth/network/etc. categories.
    Q_UNUSED(errorMessage)
    return ErrorKind::Other;
}

bool RemoteSyncProvider::isAuthorized(const QJsonObject& config) const
{
    // Default: not authorized. Providers override to declare their auth shape.
    Q_UNUSED(config)
    return false;
}

void RemoteSyncProvider::persistRefreshedTokens(const QString& stdOutput, RemoteSettings* settings) const
{
    // Default: no-op. Only providers that issue refreshable tokens override.
    Q_UNUSED(stdOutput)
    Q_UNUSED(settings)
}

RemoteSyncProvider* RemoteSyncProvider::create(const QString& type, QObject* parent)
{
    if (const auto& override = factoryOverride()) {
        if (auto* p = override(type, parent)) {
            return p;
        }
        // Override returned nullptr -> fall through to default dispatch so a
        // test that only mocks "dropbox" still gets a real CommandSyncProvider
        // for other types.
    }

    if (type == QStringLiteral("command")) {
        return new CommandSyncProvider(parent);
    }
#ifdef KPXC_FEATURE_NETWORK
    if (type == QStringLiteral("dropbox")) {
        return new DropboxSyncProvider(parent);
    }
    if (type == QStringLiteral("nextcloud")) {
        return new NextcloudSyncProvider(parent);
    }
#endif
#ifdef QT_TEST_LIB
    if (type == QStringLiteral("mock")) {
        return new MockSyncProvider(parent);
    }
#endif
    qWarning("RemoteSyncProvider: Unknown provider type '%s'", qPrintable(type));
    return nullptr;
}
