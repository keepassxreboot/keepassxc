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
#include "fdosecrets/objects/Session.h"
#include "fdosecrets/widgets/SettingsModels.h"

#include "gui/DatabaseWidget.h"
#include "gui/Icons.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QVariant>

using FdoSecrets::Session;
using FdoSecrets::SettingsDatabaseModel;
using FdoSecrets::SettingsSessionModel;

namespace
{
    class ManageDatabase : public QToolBar
    {
        Q_OBJECT

        Q_PROPERTY(DatabaseWidget* dbWidget READ dbWidget WRITE setDbWidget USER true)

    public:
        explicit ManageDatabase(FdoSecretsPlugin* plugin, QWidget* parent = nullptr)
            : QToolBar(parent)
            , m_plugin(plugin)
        {
            setFloatable(false);
            setMovable(false);

            // use a dummy widget to center the buttons
            auto spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            spacer->setVisible(true);
            addWidget(spacer);

            // db settings
            m_dbSettingsAct = new QAction(tr("Database settings"), this);
            m_dbSettingsAct->setIcon(icons()->icon(QStringLiteral("document-edit")));
            m_dbSettingsAct->setToolTip(tr("Edit database settings"));
            m_dbSettingsAct->setEnabled(false);
            connect(m_dbSettingsAct, &QAction::triggered, this, [this]() {
                if (!m_dbWidget) {
                    return;
                }
                auto db = m_dbWidget;
                m_plugin->serviceInstance()->doSwitchToDatabaseSettings(m_dbWidget);
            });
            addAction(m_dbSettingsAct);

            // unlock/lock
            m_lockAct = new QAction(tr("Unlock database"), this);
            m_lockAct->setIcon(icons()->icon(QStringLiteral("object-locked")));
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

            addAction(m_lockAct);

            // use a dummy widget to center the buttons
            spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            spacer->setVisible(true);
            addWidget(spacer);
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
                m_lockAct->setText(tr("Unlock database"));
                m_lockAct->setIcon(icons()->icon(QStringLiteral("object-locked")));
                m_lockAct->setToolTip(tr("Unlock database to show more information"));
                m_dbSettingsAct->setEnabled(false);
            });
            connect(m_dbWidget, &DatabaseWidget::databaseUnlocked, this, [this]() {
                m_lockAct->setText(tr("Lock database"));
                m_lockAct->setIcon(icons()->icon(QStringLiteral("object-unlocked")));
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

    class ManageSession : public QToolBar
    {
        Q_OBJECT

        Q_PROPERTY(Session* session READ session WRITE setSession USER true)

    public:
        explicit ManageSession(FdoSecretsPlugin*, QWidget* parent = nullptr)
            : QToolBar(parent)
        {
            setFloatable(false);
            setMovable(false);

            // use a dummy widget to center the buttons
            auto spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            spacer->setVisible(true);
            addWidget(spacer);

            m_disconnectAct = new QAction(tr("Disconnect"), this);
            m_disconnectAct->setIcon(icons()->icon(QStringLiteral("dialog-close")));
            m_disconnectAct->setToolTip(tr("Disconnect this application"));
            connect(m_disconnectAct, &QAction::triggered, this, [this]() {
                if (m_session) {
                    m_session->close();
                }
            });
            addAction(m_disconnectAct);

            // use a dummy widget to center the buttons
            spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            spacer->setVisible(true);
            addWidget(spacer);
        }

        Session* session()
        {
            return m_session;
        }

        void setSession(Session* sess)
        {
            m_session = sess;
        }

    private:
        Session* m_session = nullptr;
        QAction* m_disconnectAct = nullptr;
    };

    template <typename T> class Creator : public QItemEditorCreatorBase
    {
    public:
        inline explicit Creator(FdoSecretsPlugin* plugin)
            : QItemEditorCreatorBase()
            , m_plugin(plugin)
            , m_propertyName(T::staticMetaObject.userProperty().name())
        {
        }

        inline QWidget* createWidget(QWidget* parent) const override
        {
            return new T(m_plugin, parent);
        }

        inline QByteArray valuePropertyName() const override
        {
            return m_propertyName;
        }

    private:
        FdoSecretsPlugin* m_plugin;
        QByteArray m_propertyName;
    };
} // namespace

SettingsWidgetFdoSecrets::SettingsWidgetFdoSecrets(FdoSecretsPlugin* plugin, QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidgetFdoSecrets())
    , m_factory(new QItemEditorFactory)
    , m_plugin(plugin)
{
    m_ui->setupUi(this);
    m_ui->warningMsg->setHidden(true);
    m_ui->warningMsg->setCloseButtonVisible(false);

    auto sessModel = new SettingsSessionModel(plugin, this);
    m_ui->tableSessions->setModel(sessModel);
    setupView(m_ui->tableSessions, 1, qMetaTypeId<Session*>(), new Creator<ManageSession>(m_plugin));

    // config header after setting model, otherwise the header doesn't have enough sections
    auto sessViewHeader = m_ui->tableSessions->horizontalHeader();
    sessViewHeader->setSelectionMode(QAbstractItemView::NoSelection);
    sessViewHeader->setSectionsClickable(false);
    sessViewHeader->setSectionResizeMode(0, QHeaderView::Stretch); // application
    sessViewHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents); // disconnect button

    auto dbModel = new SettingsDatabaseModel(plugin->dbTabs(), this);
    m_ui->tableDatabases->setModel(dbModel);
    setupView(m_ui->tableDatabases, 2, qMetaTypeId<DatabaseWidget*>(), new Creator<ManageDatabase>(m_plugin));

    // config header after setting model, otherwise the header doesn't have enough sections
    auto dbViewHeader = m_ui->tableDatabases->horizontalHeader();
    dbViewHeader->setSelectionMode(QAbstractItemView::NoSelection);
    dbViewHeader->setSectionsClickable(false);
    dbViewHeader->setSectionResizeMode(0, QHeaderView::Stretch); // file name
    dbViewHeader->setSectionResizeMode(1, QHeaderView::Stretch); // group
    dbViewHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents); // manage button

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

void SettingsWidgetFdoSecrets::setupView(QAbstractItemView* view,
                                         int manageColumn,
                                         int editorTypeId,
                                         QItemEditorCreatorBase* creator)
{
    auto manageButtonDelegate = new QStyledItemDelegate(this);
    m_factory->registerEditor(editorTypeId, creator);
    manageButtonDelegate->setItemEditorFactory(m_factory.data());
    view->setItemDelegateForColumn(manageColumn, manageButtonDelegate);
    connect(view->model(),
            &QAbstractItemModel::rowsInserted,
            this,
            [view, manageColumn](const QModelIndex&, int first, int last) {
                for (int i = first; i <= last; ++i) {
                    auto idx = view->model()->index(i, manageColumn);
                    view->openPersistentEditor(idx);
                }
            });
}

SettingsWidgetFdoSecrets::~SettingsWidgetFdoSecrets() = default;

void SettingsWidgetFdoSecrets::loadSettings()
{
    m_ui->enableFdoSecretService->setChecked(FdoSecrets::settings()->isEnabled());
    m_ui->showNotification->setChecked(FdoSecrets::settings()->showNotification());
    m_ui->noConfirmDeleteItem->setChecked(FdoSecrets::settings()->noConfirmDeleteItem());
}

void SettingsWidgetFdoSecrets::saveSettings()
{
    FdoSecrets::settings()->setEnabled(m_ui->enableFdoSecretService->isChecked());
    FdoSecrets::settings()->setShowNotification(m_ui->showNotification->isChecked());
    FdoSecrets::settings()->setNoConfirmDeleteItem(m_ui->noConfirmDeleteItem->isChecked());
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

    auto reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral(DBUS_SERVICE_SECRET));
    if (!reply.isValid()) {
        m_ui->warningMsg->showMessage(
            tr("<b>Error:</b> Failed to connect to DBus. Please check your DBus setup."), MessageWidget::Error, -1);
        m_ui->enableFdoSecretService->setChecked(false);
        m_ui->enableFdoSecretService->setEnabled(false);
        return;
    }
    if (reply.value()) {
        m_ui->warningMsg->showMessage(
            tr("<b>Warning:</b> ") + m_plugin->reportExistingService(), MessageWidget::Warning, -1);
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
