/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "SettingsWidgetFdoSecrets.h"
#include "ui_SettingsWidgetFdoSecrets.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/widgets/RowButtonHelper.h"
#include "fdosecrets/widgets/SettingsModels.h"
#include "objects/Service.h"

#include "gui/DatabaseWidget.h"

#include <QAction>
#include <QToolButton>

using FdoSecrets::DBusClientPtr;
using FdoSecrets::SettingsClientModel;
using FdoSecrets::SettingsDatabaseModel;

namespace
{
    class ManageDatabase : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(DatabaseWidget* dbWidget READ dbWidget WRITE setDbWidget USER true)

    public:
        explicit ManageDatabase(FdoSecretsPlugin* plugin, QWidget* parent = nullptr)
            : QWidget(parent)
            , m_plugin(plugin)
        {
            // db settings
            m_dbSettingsAct = new QAction(tr("Database settings"), this);
            m_dbSettingsAct->setIcon(icons()->icon(QStringLiteral("document-edit")));
            m_dbSettingsAct->setToolTip(tr("Edit database settings"));
            m_dbSettingsAct->setEnabled(false);
            connect(m_dbSettingsAct, &QAction::triggered, this, [this]() {
                if (!m_dbWidget) {
                    return;
                }
                m_plugin->serviceInstance()->doSwitchToDatabaseSettings(m_dbWidget);
            });

            // unlock/lock
            m_lockAct = new QAction(tr("Unlock database"), this);
            m_lockAct->setIcon(icons()->icon(QStringLiteral("object-unlocked")));
            m_lockAct->setToolTip(tr("Unlock database to show more information"));
            connect(m_lockAct, &QAction::triggered, this, [this]() {
                if (!m_dbWidget) {
                    return;
                }
                if (m_dbWidget->isLocked()) {
                    m_plugin->serviceInstance()->doUnlockDatabaseInDialog(m_dbWidget);
                } else {
                    m_dbWidget->lock();
                }
            });

            // layout
            auto dbSettingsBtn = new QToolButton(this);
            dbSettingsBtn->setAutoRaise(true);
            dbSettingsBtn->setDefaultAction(m_dbSettingsAct);

            auto lockBtn = new QToolButton(this);
            lockBtn->setAutoRaise(true);
            lockBtn->setDefaultAction(m_lockAct);

            auto layout = new QHBoxLayout(this);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);

            layout->addStretch();
            layout->addWidget(dbSettingsBtn);
            layout->addWidget(lockBtn);
            layout->addStretch();
        }

        DatabaseWidget* dbWidget() const
        {
            return m_dbWidget;
        }

        void setDbWidget(DatabaseWidget* dbWidget)
        {
            if (m_dbWidget == dbWidget) {
                return;
            }

            if (m_dbWidget) {
                disconnect();
            }

            m_dbWidget = dbWidget;

            reconnect();
        }

    private:
        void disconnect()
        {
            if (!m_dbWidget) {
                return;
            }
            m_dbWidget->disconnect(this);
        }

        void reconnect()
        {
            if (!m_dbWidget) {
                return;
            }
            connect(m_dbWidget, &DatabaseWidget::databaseLocked, this, [this]() {
                if (!m_lockAct || !m_dbSettingsAct) {
                    return;
                }
                m_lockAct->setText(tr("Unlock database"));
                m_lockAct->setIcon(icons()->icon(QStringLiteral("object-unlocked")));
                m_lockAct->setToolTip(tr("Unlock database to show more information"));
                m_dbSettingsAct->setEnabled(false);
            });
            connect(m_dbWidget, &DatabaseWidget::databaseUnlocked, this, [this]() {
                if (!m_lockAct || !m_dbSettingsAct) {
                    return;
                }
                m_lockAct->setText(tr("Lock database"));
                m_lockAct->setIcon(icons()->icon(QStringLiteral("object-locked")));
                m_lockAct->setToolTip(tr("Lock database"));
                m_dbSettingsAct->setEnabled(true);
            });
        }

    private:
        FdoSecretsPlugin* m_plugin = nullptr;
        QPointer<DatabaseWidget> m_dbWidget = nullptr;
        QAction* m_dbSettingsAct = nullptr;
        QAction* m_lockAct = nullptr;
    };

    class ManageSession : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(const DBusClientPtr& client READ client WRITE setClient USER true)

    public:
        explicit ManageSession(QWidget* parent = nullptr)
            : QWidget(parent)
        {
            auto disconnectAct = new QAction(tr("Disconnect"), this);
            disconnectAct->setIcon(icons()->icon(QStringLiteral("dialog-close")));
            disconnectAct->setToolTip(tr("Disconnect this application"));
            connect(disconnectAct, &QAction::triggered, this, [this]() {
                if (m_client) {
                    m_client->disconnectDBus();
                }
            });

            auto resetAct = new QAction(tr("Reset"), this);
            resetAct->setIcon(icons()->icon(QStringLiteral("refresh")));
            resetAct->setToolTip(tr("Reset any remembered decisions for this application"));
            connect(resetAct, &QAction::triggered, this, [this]() {
                if (m_client) {
                    m_client->clearAuthorization();
                }
            });

            // layout
            auto disconnectBtn = new QToolButton(this);
            disconnectBtn->setAutoRaise(true);
            disconnectBtn->setDefaultAction(disconnectAct);

            auto resetBtn = new QToolButton(this);
            resetBtn->setAutoRaise(true);
            resetBtn->setDefaultAction(resetAct);

            auto layout = new QHBoxLayout(this);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);

            layout->addStretch();
            layout->addWidget(resetBtn);
            layout->addWidget(disconnectBtn);
            layout->addStretch();
        }

        const DBusClientPtr& client() const
        {
            return m_client;
        }

        void setClient(DBusClientPtr client)
        {
            m_client = std::move(client);
        }

    private:
        DBusClientPtr m_client{};
    };
} // namespace

SettingsWidgetFdoSecrets::SettingsWidgetFdoSecrets(FdoSecretsPlugin* plugin, QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidgetFdoSecrets())
    , m_plugin(plugin)
{
    m_ui->setupUi(this);
    m_ui->warningMsg->setHidden(true);
    m_ui->warningMsg->setCloseButtonVisible(false);

    auto clientModel = new SettingsClientModel(*plugin->dbus(), this);
    m_ui->tableClients->setModel(clientModel);
    installWidgetItemDelegate<ManageSession>(m_ui->tableClients,
                                             SettingsClientModel::ColumnManage,
                                             [](QWidget* p, const QModelIndex&) { return new ManageSession(p); });

    // config header after setting model, otherwise the header doesn't have enough sections
    auto clientViewHeader = m_ui->tableClients->horizontalHeader();
    clientViewHeader->setSelectionMode(QAbstractItemView::NoSelection);
    clientViewHeader->setSectionsClickable(false);
    clientViewHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    clientViewHeader->setSectionResizeMode(SettingsClientModel::ColumnApplication, QHeaderView::Stretch);

    auto dbModel = new SettingsDatabaseModel(plugin->dbTabs(), this);
    m_ui->tableDatabases->setModel(dbModel);
    installWidgetItemDelegate<ManageDatabase>(
        m_ui->tableDatabases, SettingsDatabaseModel::ColumnManage, [plugin](QWidget* p, const QModelIndex&) {
            return new ManageDatabase(plugin, p);
        });

    // config header after setting model, otherwise the header doesn't have enough sections
    auto dbViewHeader = m_ui->tableDatabases->horizontalHeader();
    dbViewHeader->setSelectionMode(QAbstractItemView::NoSelection);
    dbViewHeader->setSectionsClickable(false);
    dbViewHeader->setSectionResizeMode(QHeaderView::Stretch);
    dbViewHeader->setSectionResizeMode(SettingsDatabaseModel::ColumnManage, QHeaderView::ResizeToContents);

    // prompt the user to save settings before the sections are enabled
    connect(m_plugin, &FdoSecretsPlugin::secretServiceStarted, this, &SettingsWidgetFdoSecrets::updateServiceState);
    connect(m_plugin, &FdoSecretsPlugin::secretServiceStopped, this, &SettingsWidgetFdoSecrets::updateServiceState);
    connect(m_ui->enableFdoSecretService, &QCheckBox::toggled, this, &SettingsWidgetFdoSecrets::updateServiceState);
    updateServiceState();

    // background checking
    m_checkTimer.setInterval(2000);
    connect(&m_checkTimer, &QTimer::timeout, this, &SettingsWidgetFdoSecrets::checkDBusName);
    connect(m_plugin, &FdoSecretsPlugin::secretServiceStarted, &m_checkTimer, &QTimer::stop);
    connect(m_plugin, SIGNAL(secretServiceStopped()), &m_checkTimer, SLOT(start()));
}

SettingsWidgetFdoSecrets::~SettingsWidgetFdoSecrets() = default;

void SettingsWidgetFdoSecrets::loadSettings()
{
    m_ui->enableFdoSecretService->setChecked(FdoSecrets::settings()->isEnabled());
    m_ui->showNotification->setChecked(FdoSecrets::settings()->showNotification());
    m_ui->confirmDeleteItem->setChecked(FdoSecrets::settings()->confirmDeleteItem());
    m_ui->confirmAccessItem->setChecked(FdoSecrets::settings()->confirmAccessItem());
    m_ui->unlockBeforeSearch->setChecked(FdoSecrets::settings()->unlockBeforeSearch());
}

void SettingsWidgetFdoSecrets::saveSettings()
{
    FdoSecrets::settings()->setEnabled(m_ui->enableFdoSecretService->isChecked());
    FdoSecrets::settings()->setShowNotification(m_ui->showNotification->isChecked());
    FdoSecrets::settings()->setConfirmDeleteItem(m_ui->confirmDeleteItem->isChecked());
    FdoSecrets::settings()->setConfirmAccessItem(m_ui->confirmAccessItem->isChecked());
    FdoSecrets::settings()->setUnlockBeforeSearch(m_ui->unlockBeforeSearch->isChecked());
}

void SettingsWidgetFdoSecrets::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &SettingsWidgetFdoSecrets::checkDBusName);
    m_checkTimer.start();
}

void SettingsWidgetFdoSecrets::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_checkTimer.stop();
}

void SettingsWidgetFdoSecrets::checkDBusName()
{
    if (m_plugin->serviceInstance()) {
        // only need checking if the service is not started or failed to start.
        return;
    }

    if (m_plugin->dbus()->serviceOccupied()) {
        m_ui->warningMsg->showMessage(
            tr("<b>Warning:</b> ") + m_plugin->dbus()->reportExistingService(), MessageWidget::Warning, -1);
        m_ui->enableFdoSecretService->setChecked(false);
        m_ui->enableFdoSecretService->setEnabled(false);
        return;
    }
    m_ui->warningMsg->hideMessage();
    m_ui->enableFdoSecretService->setEnabled(true);
}

void SettingsWidgetFdoSecrets::updateServiceState()
{
    m_ui->tabWidget->setEnabled(m_plugin->serviceInstance() != nullptr);
    if (m_ui->enableFdoSecretService->isChecked() && !m_plugin->serviceInstance()) {
        m_ui->tabWidget->setToolTip(
            tr("Save current changes to activate the plugin and enable editing of this section."));
    } else {
        m_ui->tabWidget->setToolTip("");
    }
}

#include "SettingsWidgetFdoSecrets.moc"
