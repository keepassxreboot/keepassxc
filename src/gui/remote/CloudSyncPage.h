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

#ifndef KEEPASSX_CLOUDSYNCPAGE_H
#define KEEPASSX_CLOUDSYNCPAGE_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

class RemoteSettings;
class RemoteSyncProvider;

// Abstract base for per-provider settings sub-pages hosted by
// DatabaseSettingsWidgetCloudSync's providerStackedWidget. Each concrete
// subclass (DropboxCloudSyncPage, NextcloudCloudSyncPage) owns the
// provider-specific widgets via its own .ui file. The parent widget drives
// the page through this contract -- never via dynamic_cast.
class CloudSyncPage : public QWidget
{
    Q_OBJECT

public:
    explicit CloudSyncPage(QWidget* parent = nullptr);
    ~CloudSyncPage() override = default;

    // Factory for the built-in per-provider pages. Returns one
    // new-allocated page per registered provider. Caller takes ownership
    // of each returned page (typically by reparenting to a host QWidget).
    // The parent argument is forwarded to each page's constructor.
    // Defined out-of-line so the parent widget never directly names
    // the concrete subclasses.
    static QList<CloudSyncPage*> createBuiltinPages(QWidget* parent = nullptr);

    // Inject the provider this page configures. Parent owns the provider
    // lifetime; subclasses must NOT take ownership (pointer is borrowed).
    virtual void setProvider(RemoteSyncProvider* provider) = 0;

    // Provider type tag (e.g. "dropbox", "nextcloud"). Used by the parent
    // to match the page to RemoteSettings::activeProvider() and to drive
    // RemoteSettings::getProviderConfig(type, ...) /
    // setProviderConfig(type, ...).
    virtual QString providerType() const = 0;

    // Provider display name forwarded from RemoteSyncProvider::displayName().
    // Used by the parent to populate the provider dropdown.
    virtual QString providerDisplayName() const = 0;

    // Populate the page's fields from the persisted config object.
    // Called by parent when the user opens the dialog.
    virtual void loadFromConfig(const QJsonObject& config) = 0;

    // Serialize current field values to a config object.
    // Called by parent on Apply.
    virtual QJsonObject saveToConfig() const = 0;

    // Whether the user has edited any field since the last loadFromConfig.
    // Drives Apply-button-enabled state.
    virtual bool isModified() const = 0;

    // Inject the RemoteSettings instance the parent owns; pages use it to
    // persist token updates from refresh/auth flows. Pointer is borrowed;
    // parent retains ownership. Default: no-op (subclasses that don't
    // persist tokens can ignore).
    virtual void setRemoteSettings(RemoteSettings* settings);

    // Notify the page that the parent's mutual-exclusivity check (Script
    // Sync configured) is active and the page's fields should be disabled.
    // Default: no-op.
    virtual void setMutualExclusivityWarning(bool active);

signals:
    // Emitted when the user clicks the per-page Authorize button.
    void requestAuthorize();

    // Emitted when the user clicks the per-page Remove button.
    void requestRemove();

    // Emitted when the user clicks the per-page Test Connection button.
    void requestTestConnection();

    // Emitted when the user clicks the per-page Sync Now button.
    void requestSync();

    // Emitted when any field on the page changes (drives Apply enabled state).
    void modified();

    // Forwarded to the parent's MessageWidget for status/error display.
    // `messageType` matches the KMessageWidget::MessageType enum (passed as
    // int to keep this header free of MessageWidget includes); valid values
    // are 0=Positive, 1=Information, 2=Warning, 3=Error.
    // Optional `disableAutoHide` overrides the default auto-hide policy.
    void showMessage(const QString& text, int messageType, bool disableAutoHide = false);

    // Emitted when the page wants the parent's messageWidget cleared/hidden.
    void hideMessage();

private:
    Q_DISABLE_COPY(CloudSyncPage)
};

#endif // KEEPASSX_CLOUDSYNCPAGE_H
