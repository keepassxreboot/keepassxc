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

#include "DatabaseSettingsWidgetCloudSync.h"
#include "ui_DatabaseSettingsWidgetCloudSync.h"

#include "RemoteSettings.h"
#include "gui/MessageWidget.h"
#include "remotesync/RemoteSyncParams.h"

#include <QTimer>

DatabaseSettingsWidgetCloudSync::DatabaseSettingsWidgetCloudSync(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_remoteSettings(new RemoteSettings(nullptr, nullptr))
    , m_ui(new Ui::DatabaseSettingsWidgetCloudSync())
{
    m_ui->setupUi(this);
    m_ui->messageWidget->setHidden(true);

    // Register provider pages via the CloudSyncPage factory. The factory
    // lives in CloudSyncPage.cpp so this file never names a concrete
    // subclass.
    for (auto* page : CloudSyncPage::createBuiltinPages(this)) {
        registerPage(page);
    }

    m_ui->providerStackedWidget->setCurrentIndex(0);

    // Connect provider selection
    connect(m_ui->providerComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DatabaseSettingsWidgetCloudSync::onProviderChanged);

    // Mirrors KeyComponentWidget::updateSize so the QStackedWidget reports the
    // size of the current page only, not max-of-children. Otherwise hidden
    // provider pages with larger sizeHints (e.g. Nextcloud's appPasswordGroupBox)
    // leak vertical slack into the visible page's QGroupBox.
    QTimer::singleShot(0, this, &DatabaseSettingsWidgetCloudSync::updateSize);
}

DatabaseSettingsWidgetCloudSync::~DatabaseSettingsWidgetCloudSync() = default;

void DatabaseSettingsWidgetCloudSync::registerPage(CloudSyncPage* page)
{
    m_pages.append(page);
    m_ui->providerStackedWidget->addWidget(page);
    m_ui->providerComboBox->addItem(page->providerDisplayName());

    // Inject the concrete provider via the RemoteSyncProvider factory. The
    // provider is parented to this widget so the QObject parent chain owns
    // its lifetime; pages borrow the pointer. The widget never names a
    // concrete provider class (the factory does).
    page->setProvider(RemoteSyncProvider::create(page->providerType(), this));

    // Forward page->modified() through to settingsModified for the dialog's
    // dirty-flag tracking.
    connect(page, &CloudSyncPage::modified, this, &DatabaseSettingsWidgetCloudSync::settingsModified);

    // Forward page-emitted message events to the parent's MessageWidget.
    connect(page, &CloudSyncPage::showMessage, this, &DatabaseSettingsWidgetCloudSync::onPageShowMessage);
    connect(page, &CloudSyncPage::hideMessage, this, &DatabaseSettingsWidgetCloudSync::onPageHideMessage);

    // Trigger Sync button on the page bubbles up through cloudSyncTriggered.
    connect(page, &CloudSyncPage::requestSync, this, &DatabaseSettingsWidgetCloudSync::onPageRequestSync);

    // After a Remove the page resets and persists its own config; the parent
    // dialog mirrors that into the Script Sync widget's lock via
    // cloudSyncRemoved so the reciprocal banner clears without a reopen.
    connect(page, &CloudSyncPage::requestRemove, this, &DatabaseSettingsWidgetCloudSync::cloudSyncRemoved);
}

CloudSyncPage* DatabaseSettingsWidgetCloudSync::activePage() const
{
    return m_pages.value(m_ui->providerComboBox->currentIndex(), nullptr);
}

void DatabaseSettingsWidgetCloudSync::initialize()
{
    m_ui->messageWidget->setHidden(true);
    m_remoteSettings->setDatabase(m_db);

    // Always reset per-page state to THIS database's stored config before
    // any mutual-exclusivity gate runs. The parent dialog widget is reused
    // across databases, so without this reset a subsequent Apply could
    // serialize stale state (including OAuth tokens) from a prior database
    // into the current database's CustomData.
    for (auto* page : m_pages) {
        page->setRemoteSettings(m_remoteSettings.data());
    }
    // Single-provider model: the active config (if any) lives in the one
    // KPXC_CLOUD_SYNC_SETTINGS slot. Hand it to the matching page; the
    // others get an empty config so they show a blank form.
    const QJsonObject activeConfig = m_remoteSettings->cloudSyncConfig();
    const QString activeType = activeConfig.value(RemoteSyncConfigKeys::Type).toString();
    for (auto* page : m_pages) {
        page->setMutualExclusivityWarning(false);
        page->loadFromConfig(page->providerType() == activeType ? activeConfig : QJsonObject{});
    }

    // Mutual-exclusivity gate: if Script Sync is configured, lock cloud-sync
    // editing entirely (one or the other, not both). We've already loaded the
    // current database's state above so the locked widgets show its config,
    // not a previous database's residue.
    m_lockedByScriptSync = hasScriptSyncConfig();
    if (m_lockedByScriptSync) {
        m_ui->messageWidget->showMessage(
            tr("Script Sync is configured for this database. Remove it in the Script Sync tab before setting up "
               "Cloud Sync."),
            MessageWidget::Warning,
            MessageWidget::DisableAutoHide);
        m_ui->messageWidget->setCloseButtonVisible(false);
        for (auto* page : m_pages) {
            page->setMutualExclusivityWarning(true);
        }
        m_ui->providerComboBox->setEnabled(false);
        return;
    }

    m_ui->providerComboBox->setEnabled(true);

    // Switch the combobox to the active provider, if one is set. For
    // single-provider DBs the active-provider lazy default in RemoteSettings
    // resolves to whichever page has stored credentials.
    const QString active = m_remoteSettings->activeProvider();
    if (!active.isEmpty()) {
        for (int i = 0; i < m_pages.size(); ++i) {
            if (m_pages[i]->providerType() == active) {
                m_ui->providerComboBox->setCurrentIndex(i);
                m_ui->providerStackedWidget->setCurrentIndex(i);
                break;
            }
        }
    }
}

void DatabaseSettingsWidgetCloudSync::uninitialize()
{
}

bool DatabaseSettingsWidgetCloudSync::saveSettings()
{
    // Init-time mutual-exclusivity gate: when Script Sync was configured
    // when the dialog opened, the cloud-sync tab is locked read-only. Skip
    // serialization entirely -- the page widgets are display-only at this
    // point, and persisting their (just-loaded) state would re-stamp the
    // database's CustomData without user intent.
    if (m_lockedByScriptSync) {
        return true;
    }

    // Runtime mutual-exclusivity check: catches the case where the dialog
    // opened with both modes empty (init lock = false on both sides) and the
    // user committed a Script Sync entry in this same dialog session before
    // we got here. DatabaseSettingsDialog::saveAllSettings runs the script
    // widget before the cloud widget, so by the time we reach this point
    // those changes are persisted in the database's CustomData. We query a
    // fresh RemoteSettings instance because m_remoteSettings was snapshot
    // when the dialog opened and would not reflect script changes committed
    // mid-session.
    if (m_db) {
        RemoteSettings current(m_db);
        if (!current.getAllRemoteParams().isEmpty()) {
            m_ui->messageWidget->showMessage(
                tr("Script Sync was added in this session. Remove it in the Script Sync tab before applying "
                   "Cloud Sync."),
                MessageWidget::Error,
                MessageWidget::DisableAutoHide);
            return false;
        }
    }

    auto* active = activePage();
    if (!active) {
        return true;
    }

    // saveToConfig always emits at least the type/name tags. The page
    // returns an empty QJsonObject when its fields are in a fresh-no-edit
    // state to signal "skip persistence" -- without this, opening the
    // dialog without touching anything would bloat KDBX files with empty
    // per-provider records.
    QJsonObject config = active->saveToConfig();
    if (config.isEmpty()) {
        return true;
    }

    // Single-provider model: a database has at most one cloud-sync provider
    // active. Two cloud backends syncing the same .kdbx would diverge
    // irreversibly (no merge between Dropbox revs and Nextcloud ETags), so
    // Apply replaces the previous provider once the new page reaches an
    // authorized state. Pre-authorization edits are not persisted -- the
    // single CustomData::CloudSyncSettings slot only ever holds an authorized
    // (or recently-revoked) config for the current provider.
    QScopedPointer<RemoteSyncProvider> probe(RemoteSyncProvider::create(active->providerType()));
    const bool authorized = probe && probe->isAuthorized(config);
    if (!authorized) {
        return true;
    }

    // Clear displaced pages' UI so it matches the new single-config reality;
    // otherwise the previously-active page still shows its old line-edits,
    // token status, and cached m_config in the current dialog session.
    for (auto* page : m_pages) {
        if (page != active) {
            page->loadFromConfig(QJsonObject{});
        }
    }

    m_remoteSettings->setCloudSyncConfig(config);
    m_remoteSettings->saveSettings();
    return true;
}

void DatabaseSettingsWidgetCloudSync::onProviderChanged(int index)
{
    m_ui->providerStackedWidget->setCurrentIndex(index);
    updateSize();
}

void DatabaseSettingsWidgetCloudSync::updateSize()
{
    auto* stack = m_ui->providerStackedWidget;
    for (int i = 0; i < stack->count(); ++i) {
        QSizePolicy policy = stack->widget(i)->sizePolicy();
        policy.setVerticalPolicy(stack->currentIndex() == i ? QSizePolicy::Preferred : QSizePolicy::Ignored);
        stack->widget(i)->setSizePolicy(policy);
    }
}

void DatabaseSettingsWidgetCloudSync::onPageShowMessage(const QString& text, int messageType, bool disableAutoHide)
{
    const auto type = static_cast<MessageWidget::MessageType>(messageType);
    if (disableAutoHide) {
        m_ui->messageWidget->showMessage(text, type, MessageWidget::DisableAutoHide);
    } else {
        m_ui->messageWidget->showMessage(text, type);
    }
}

void DatabaseSettingsWidgetCloudSync::onPageHideMessage()
{
    m_ui->messageWidget->hideMessage();
}

void DatabaseSettingsWidgetCloudSync::onPageRequestSync()
{
    emit cloudSyncTriggered();
}

bool DatabaseSettingsWidgetCloudSync::hasScriptSyncConfig() const
{
    return !m_remoteSettings->getAllRemoteParams().isEmpty();
}

void DatabaseSettingsWidgetCloudSync::onScriptSyncRemoved()
{
    // Nothing to do if we weren't locked -- avoid stomping on an unrelated
    // banner the user may currently be reading.
    if (!m_lockedByScriptSync) {
        return;
    }
    m_lockedByScriptSync = false;
    m_ui->messageWidget->hideMessage();
    m_ui->providerComboBox->setEnabled(true);
    for (auto* page : m_pages) {
        page->setMutualExclusivityWarning(false);
    }

    // Drop the stale script-params snapshot on our own RemoteSettings. The
    // script widget mutated only its own RemoteSettings instance (and only
    // in-memory -- script-sync persistence is deferred to dialog Apply), so
    // ours still holds the m_remoteParams it loaded at initialize(). Without
    // this, a subsequent saveSettings() (either dialog Apply with a now-
    // authorized cloud config, or the cloud provider's own onRemoveClicked
    // round-trip) would have RemoteSettings::saveSettings re-stamp the stale
    // script entries back into CustomData and resurrect what the user just
    // removed. Snapshot names first to avoid iterator invalidation by
    // removeRemoteParams.
    QStringList staleNames;
    for (auto* p : m_remoteSettings->getAllRemoteParams()) {
        staleNames << p->name;
    }
    for (const auto& name : staleNames) {
        m_remoteSettings->removeRemoteParams(name);
    }
}
