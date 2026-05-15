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

#ifndef KEEPASSXC_REMOTESYNCPROVIDER_H
#define KEEPASSXC_REMOTESYNCPROVIDER_H

#include <QObject>

#include <functional>

#include "gui/remote/RemoteHandler.h" // For RemoteHandler::RemoteResult

class QJsonObject;
class RemoteSettings;
struct RemoteSyncParams;

class RemoteSyncProvider : public QObject
{
    Q_OBJECT

public:
    // Alias for RemoteHandler::ErrorKind, which is the canonical definition
    // (carried on RemoteHandler::RemoteResult).
    using ErrorKind = RemoteHandler::ErrorKind;

    explicit RemoteSyncProvider(QObject* parent = nullptr);
    ~RemoteSyncProvider() override = default;

    // Core sync operations -- synchronous, blocking
    virtual RemoteHandler::RemoteResult download(const RemoteSyncParams* params) = 0;
    virtual RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) = 0;

    // Auth refresh -- no-op for command provider, real for OAuth providers
    virtual RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) = 0;

    // Cancel support -- abort in-flight operation
    virtual void abort() = 0;

    // Untranslated provider identifier shown in user-visible chrome.
    // E.g. "Dropbox", "Nextcloud". UI applies tr() at the call site.
    virtual QString displayName() const = 0;

    // Allocate a fresh RemoteSyncParams subclass for this provider.
    // Caller takes ownership.
    virtual RemoteSyncParams* createParams() const = 0;

    // Build a fresh RemoteSyncParams from a persisted config object.
    // Used by orchestration code that has the JSON config but no knowledge
    // of the concrete params subclass. Default impl: calls createParams()
    // and returns it without populating any fields. Providers should override
    // to populate their type-specific fields. Caller takes ownership.
    virtual RemoteSyncParams* buildParamsFromConfig(const QJsonObject& config) const;

    // Apply refreshed-token JSON (from refreshAuth's stdOutput) to in-memory params.
    // Returns false on parse failure (engine treats as AuthExpired and surfaces banner).
    // Default: no-op true (providers without token refresh inherit).
    virtual bool applyRefreshedTokens(const QString& stdOutput, RemoteSyncParams* params);

    // Classify a provider error message into an ErrorKind for UI dispatch.
    // Default: Other (no false positives from accidental keyword matches).
    virtual ErrorKind classifyError(const QString& errorMessage) const;

    // Returns true if the given config blob contains every field this
    // provider needs to perform an authorized sync. Each provider knows
    // its own auth shape. Callers use this through the abstraction
    // rather than peeking at provider-specific config keys directly.
    // Default: false (fail-closed; providers must opt in).
    virtual bool isAuthorized(const QJsonObject& config) const;

    // Persist refreshed tokens back to RemoteSettings (single-provider model:
    // updates whatever cloud config is currently stored if it matches this
    // provider's type). Called by orchestration code after refreshAuth
    // succeeds. Default: no-op.
    virtual void persistRefreshedTokens(const QString& stdOutput, RemoteSettings* settings) const;

    // Factory method -- returns correct subclass from config type string
    // Returns nullptr for unknown types
    static RemoteSyncProvider* create(const QString& type, QObject* parent = nullptr);

    // Test seam: when set, create() routes through this factory instead of
    // the default dispatch, letting tests substitute mock providers so no
    // real network calls are issued. Returning nullptr from the override
    // falls back to the default behavior.
    using FactoryOverride = std::function<RemoteSyncProvider*(const QString& type, QObject* parent)>;
    static void setFactoryOverrideForTest(FactoryOverride factory);
    static void clearFactoryOverrideForTest();

    Q_DISABLE_COPY(RemoteSyncProvider)
};

#endif // KEEPASSXC_REMOTESYNCPROVIDER_H
