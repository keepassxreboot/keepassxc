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

    // After a Remove the page reset its config; we don't need to react beyond
    // what the page already did locally (it persists via RemoteSettings).
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
    for (auto* page : m_pages) {
        QJsonObject config = m_remoteSettings->getProviderConfig(page->providerType(),
                                                                 page->providerType() + QStringLiteral("-default"));
        page->setMutualExclusivityWarning(false);
        page->loadFromConfig(config);
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

    const QString configKey = active->providerType() + QStringLiteral("-default");

    // Single-provider model: a database has at most one cloud-sync provider
    // configured at a time. Two cloud backends synchronizing the same .kdbx
    // would diverge irreversibly (no merge protocol between Dropbox revisions
    // and Nextcloud ETags), so Apply replaces the previous provider when the
    // new page reaches an authorized state. Before that, we still want to
    // preserve the user's draft (URLs typed, paths, etc.) without destroying
    // a previously-working provider config -- otherwise trying out Nextcloud
    // would wipe a working Dropbox before Nextcloud is ever authorized.
    QScopedPointer<RemoteSyncProvider> probe(RemoteSyncProvider::create(active->providerType()));
    const bool authorized = probe && probe->isAuthorized(config);

    if (authorized) {
        for (auto* page : m_pages) {
            if (page != active) {
                m_remoteSettings->removeProviderConfig(page->providerType(),
                                                       page->providerType() + QStringLiteral("-default"));
                // Reload the displaced page from the now-empty config so its
                // UI reflects the wipe in the current dialog session. Without
                // this the page still shows its old line-edit text, token
                // status, and cached m_config -- visually contradicting the
                // single-provider model the wipe just enforced, and leading
                // users to believe the database wasn't actually wiped either.
                page->loadFromConfig(QJsonObject{});
            }
        }
        m_remoteSettings->setActiveProvider(active->providerType());
    }

    m_remoteSettings->setProviderConfig(active->providerType(), configKey, config);
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
