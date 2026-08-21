/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "SettingsModels.h"

#include "fdosecrets/ClientAuth.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/dbus/DBusMgr.h"
#include "fdosecrets/widgets/ClientAuthModels.h"

#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/Icons.h"

#include <QFileInfo>
#include <QFont>

namespace FdoSecrets
{
    // static constexpr still requires definition before c++17
    constexpr const char* SettingsDatabaseModel::ColumnNames[];

    SettingsDatabaseModel::SettingsDatabaseModel(DatabaseTabWidget* dbTabs, QObject* parent)
        : QAbstractTableModel(parent)
        , m_dbTabs(nullptr)
    {
        setTabWidget(dbTabs);
    }

    void SettingsDatabaseModel::setTabWidget(DatabaseTabWidget* dbTabs)
    {
        auto old = m_dbTabs;
        m_dbTabs = dbTabs;
        if (old != m_dbTabs) {
            populateModel();
        }
    }

    int SettingsDatabaseModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_dbs.size();
    }

    int SettingsDatabaseModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return sizeof(ColumnNames) / sizeof(ColumnNames[0]);
    }

    QVariant SettingsDatabaseModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return {};
        }

        if (role != Qt::DisplayRole) {
            return {};
        }

        if (section < 0 || section >= columnCount({})) {
            return {};
        }

        return qApp->translate(metaObject()->className(), ColumnNames[section]);
    }

    QVariant SettingsDatabaseModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        if (index.model() != this) {
            return {};
        }
        if (index.row() >= rowCount({}) || index.column() >= columnCount({})) {
            return {};
        }

        const auto& dbWidget = m_dbs[index.row()];
        if (!dbWidget) {
            return {};
        }

        switch (index.column()) {
        case ColumnFileName:
            return dataForName(dbWidget, role);
        case ColumnGroup:
            return dataForExposedGroup(dbWidget, role);
        case ColumnManage:
            return dataForManage(dbWidget, role);
        default:
            return {};
        }
    }

    QVariant SettingsDatabaseModel::dataForName(DatabaseWidget* db, int role) const
    {
        switch (role) {
        case Qt::DisplayRole: {
            QFileInfo fi(db->database()->filePath());
            return fi.fileName();
        }
        case Qt::ToolTipRole:
            return db->database()->filePath();
        default:
            return {};
        }
    }

    QVariant SettingsDatabaseModel::dataForExposedGroup(DatabaseWidget* dbWidget, int role)
    {
        if (dbWidget->isLocked()) {
            switch (role) {
            case Qt::DisplayRole:
                return tr("Unlock to show");
            case Qt::DecorationRole:
                return icons()->icon(QStringLiteral("object-locked"));
            case Qt::FontRole: {
                QFont font;
                font.setItalic(true);
                return font;
            }
            default:
                return {};
            }
        }
        auto db = dbWidget->database();
        auto group = db->rootGroup()->findGroupByUuid(FdoSecrets::settings()->exposedGroup(db));
        if (group) {
            switch (role) {
            case Qt::DisplayRole:
                return group->name();
            case Qt::DecorationRole:
                return Icons::groupIconPixmap(group);
            case Qt::FontRole:
                if (group->isExpired()) {
                    QFont font;
                    font.setStrikeOut(true);
                    return font;
                } else {
                    return {};
                }
            default:
                return {};
            }
        } else {
            switch (role) {
            case Qt::DisplayRole:
                return tr("None");
            case Qt::DecorationRole:
                return icons()->icon(QStringLiteral("paint-none"));
            default:
                return {};
            }
        }
    }

    QVariant SettingsDatabaseModel::dataForManage(DatabaseWidget* db, int role) const
    {
        switch (role) {
        case Qt::EditRole:
            return QVariant::fromValue(db);
        default:
            return {};
        }
    }

    void SettingsDatabaseModel::populateModel()
    {
        beginResetModel();

        m_dbs.clear();

        if (m_dbTabs) {
            // Add existing database tabs
            for (int idx = 0; idx != m_dbTabs->count(); ++idx) {
                auto dbWidget = m_dbTabs->databaseWidgetFromIndex(idx);
                databaseAdded(dbWidget, false);
            }
            // connect signals
            connect(m_dbTabs, &DatabaseTabWidget::databaseOpened, this, [this](DatabaseWidget* db) {
                databaseAdded(db, true);
            });
            connect(m_dbTabs, &DatabaseTabWidget::databaseClosed, this, &SettingsDatabaseModel::databaseRemoved);
        }

        endResetModel();
    }

    void SettingsDatabaseModel::databaseAdded(DatabaseWidget* db, bool emitSignals)
    {
        int row = m_dbs.size();
        if (emitSignals) {
            beginInsertRows({}, row, row);
        }

        m_dbs.append(db);
        connect(db, &DatabaseWidget::databaseLocked, this, [row, this]() {
            emit dataChanged(index(row, 1), index(row, 2));
        });
        connect(db, &DatabaseWidget::databaseUnlocked, this, [row, this]() {
            emit dataChanged(index(row, 1), index(row, 2));
        });
        connect(db, &DatabaseWidget::databaseModified, this, [row, this]() {
            emit dataChanged(index(row, 0), index(row, 2));
        });
        connect(db, &DatabaseWidget::databaseFilePathChanged, this, [row, this]() {
            emit dataChanged(index(row, 0), index(row, 2));
        });

        if (emitSignals) {
            endInsertRows();
        }
    }

    void SettingsDatabaseModel::databaseRemoved(const QString& filePath)
    {
        for (int i = 0; i != m_dbs.size(); i++) {
            if (m_dbs[i] && m_dbs[i]->database()->filePath() == filePath) {
                beginRemoveRows({}, i, i);

                m_dbs[i]->disconnect(this);
                m_dbs.removeAt(i);

                endRemoveRows();
                break;
            }
        }
    }

    // static constexpr still requires definition before c++17
    constexpr const char* SettingsClientModel::ColumnNames[];

    SettingsClientModel::SettingsClientModel(DBusMgr& dbus, DatabaseTabWidget* dbTabs, QObject* parent)
        : QAbstractTableModel(parent)
        , m_dbus(dbus)
        , m_dbTabs(dbTabs)
    {
        populateModel();
    }

    int SettingsClientModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_clients.size();
    }

    int SettingsClientModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return sizeof(ColumnNames) / sizeof(ColumnNames[0]);
    }

    QVariant SettingsClientModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return {};
        }

        if (role != Qt::DisplayRole) {
            return {};
        }

        if (section < 0 || section >= columnCount({})) {
            return {};
        }

        return qApp->translate(metaObject()->className(), ColumnNames[section]);
    }

    QVariant SettingsClientModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        if (index.model() != this) {
            return {};
        }
        if (index.row() >= rowCount({}) || index.column() >= columnCount({})) {
            return {};
        }

        const auto& client = m_clients[index.row()];
        if (!client) {
            return {};
        }

        switch (index.column()) {
        case ColumnApplication:
            return dataForApplication(client, role);
        case ColumnPID:
            return dataForPID(client, role);
        case ColumnDBus:
            return dataForDBus(client, role);
        case ColumnAuthorization:
            return dataForAuthorization(client, role);
        default:
            return {};
        }
    }

    QVariant SettingsClientModel::dataForApplication(const DBusClientPtr& client, int role) const
    {
        const auto& info = client->processInfo();
        switch (role) {
        case Qt::DisplayRole:
            if (info.exePath().isEmpty()) {
                return tr("Unknown");
            }
            return info.exePath();
        case Qt::ToolTipRole:
            if (!info.valid) {
                return tr("Non-existing/inaccessible executable path. Please double-check the client is legit.");
            }
            return {};
        case Qt::DecorationRole:
            // give some visual clues if the path is invalid
            if (!info.valid) {
                return icons()->icon(QStringLiteral("dialog-warning"));
            }
            return {};
        default:
            return {};
        }
    }

    QVariant SettingsClientModel::dataForPID(const DBusClientPtr& client, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
            return client->pid();
        default:
            return {};
        }
    }

    QVariant SettingsClientModel::dataForDBus(const DBusClientPtr& client, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
            return client->address();
        default:
            return {};
        }
    }

    QVariant SettingsClientModel::dataForAuthorization(const DBusClientPtr& client, int role) const
    {
        const auto& authz = authorization(client);
        switch (role) {
        case Qt::DisplayRole:
            return authz.summary.isEmpty() ? tr("none") : authz.summary;
        case Qt::ToolTipRole:
            return authz.toolTip;
        case Qt::FontRole:
            if (authz.summary.isEmpty()) {
                QFont font;
                font.setItalic(true);
                return font;
            }
            return {};
        default:
            return {};
        }
    }

    const SettingsClientModel::Authorization& SettingsClientModel::authorization(const DBusClientPtr& client) const
    {
        auto it = m_authorizations.find(client->address());
        if (it != m_authorizations.end()) {
            return it.value();
        }

        Authorization authz;
        QStringList toolTipLines;
        for (int idx = 0; m_dbTabs && idx != m_dbTabs->count(); ++idx) {
            auto dbWidget = m_dbTabs->databaseWidgetFromIndex(idx);
            // a locked database cannot be searched for records, and one that is
            // not exposed never authorizes anything
            if (!dbWidget || dbWidget->isLocked() || !dbWidget->database()
                || FdoSecrets::settings()->exposedGroup(dbWidget->database()).isNull()) {
                continue;
            }
            const auto db = dbWidget->database();
            const auto name = QFileInfo(db->filePath()).fileName();
            const auto res = resolveClient(db.data(), *client);
            if (res.record.isValid()) {
                authz.summary += (authz.summary.isEmpty() ? QString() : QStringLiteral("; "))
                                 + QStringLiteral("%1: %2").arg(name, res.record.name);
                toolTipLines << QStringLiteral("%1: %2").arg(name, ClientRecordsModel::rulesSummary(res.record));
            } else if (res.fingerprintChanged) {
                authz.summary += (authz.summary.isEmpty() ? QString() : QStringLiteral("; "))
                                 + tr("%1: %2 (executable changed)").arg(name, res.changed.name);
                toolTipLines << tr("The executable of this application changed since it was authorized as \"%1\" "
                                   "in %2. It has to be authorized again.")
                                    .arg(res.changed.name, name);
            }
        }
        authz.toolTip = toolTipLines.join(QLatin1Char('\n'));
        return *m_authorizations.insert(client->address(), authz);
    }

    void SettingsClientModel::invalidateAuthorizations()
    {
        if (m_authorizations.isEmpty()) {
            return;
        }
        m_authorizations.clear();
        if (!m_clients.isEmpty()) {
            emit dataChanged(index(0, ColumnAuthorization), index(m_clients.size() - 1, ColumnAuthorization));
        }
    }

    void SettingsClientModel::populateModel()
    {
        beginResetModel();

        m_clients.clear();

        // Add existing database tabs
        for (const auto& client : m_dbus.clients()) {
            clientConnected(client, false);
        }

        // connect signals
        connect(&m_dbus, &DBusMgr::clientConnected, this, [this](const DBusClientPtr& client) {
            clientConnected(client, true);
        });
        connect(&m_dbus, &DBusMgr::clientDisconnected, this, &SettingsClientModel::clientDisconnected);

        // a resolution only holds as long as the databases it was computed from
        if (m_dbTabs) {
            connect(m_dbTabs, &DatabaseTabWidget::databaseOpened, this, [this](DatabaseWidget* dbWidget) {
                connectDatabase(dbWidget);
                invalidateAuthorizations();
            });
            connect(m_dbTabs, &DatabaseTabWidget::databaseClosed, this, [this]() { invalidateAuthorizations(); });
            for (int idx = 0; idx != m_dbTabs->count(); ++idx) {
                connectDatabase(m_dbTabs->databaseWidgetFromIndex(idx));
            }
        }

        endResetModel();
    }

    void SettingsClientModel::connectDatabase(DatabaseWidget* dbWidget)
    {
        if (!dbWidget) {
            return;
        }
        connect(dbWidget, &DatabaseWidget::databaseLocked, this, &SettingsClientModel::invalidateAuthorizations);
        connect(dbWidget, &DatabaseWidget::databaseUnlocked, this, &SettingsClientModel::invalidateAuthorizations);
        // records are edited in the database settings, and written by prompts
        connect(dbWidget, &DatabaseWidget::databaseModified, this, &SettingsClientModel::invalidateAuthorizations);
    }

    void SettingsClientModel::clientConnected(const DBusClientPtr& client, bool emitSignals)
    {
        int row = m_clients.size();
        if (emitSignals) {
            beginInsertRows({}, row, row);
        }

        m_clients.append(client);

        if (emitSignals) {
            endInsertRows();
        }
    }

    void SettingsClientModel::clientDisconnected(const DBusClientPtr& client)
    {
        for (int i = 0; i != m_clients.size(); i++) {
            if (m_clients[i] == client) {
                beginRemoveRows({}, i, i);

                m_clients.removeAt(i);

                endRemoveRows();
                break;
            }
        }
    }

} // namespace FdoSecrets
