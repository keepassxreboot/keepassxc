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

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/Icons.h"

#include <QFileInfo>

namespace FdoSecrets
{

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
        return 3;
    }

    QVariant SettingsDatabaseModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return {};
        }

        if (role != Qt::DisplayRole) {
            return {};
        }

        switch (section) {
        case 0:
            return tr("File Name");
        case 1:
            return tr("Group");
        case 2:
            return tr("Manage");
        default:
            return {};
        }
    }

    QVariant SettingsDatabaseModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        const auto& dbWidget = m_dbs[index.row()];
        if (!dbWidget) {
            return {};
        }

        switch (index.column()) {
        case 0:
            return dataForName(dbWidget, role);
        case 1:
            return dataForExposedGroup(dbWidget, role);
        case 2:
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
                return group->iconPixmap();
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

    SettingsSessionModel::SettingsSessionModel(FdoSecretsPlugin* plugin, QObject* parent)
        : QAbstractTableModel(parent)
        , m_service(nullptr)
    {
        setService(plugin->serviceInstance());
        connect(plugin, &FdoSecretsPlugin::secretServiceStarted, this, [plugin, this]() {
            setService(plugin->serviceInstance());
        });
        connect(plugin, &FdoSecretsPlugin::secretServiceStopped, this, [this]() { setService(nullptr); });
    }

    void SettingsSessionModel::setService(Service* service)
    {
        auto old = m_service;
        m_service = service;
        if (old != m_service) {
            populateModel();
        }
    }

    int SettingsSessionModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_sessions.size();
    }

    int SettingsSessionModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return 2;
    }

    QVariant SettingsSessionModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return {};
        }

        if (role != Qt::DisplayRole) {
            return {};
        }

        switch (section) {
        case 0:
            return tr("Application");
        case 1:
            return tr("Manage");
        default:
            return {};
        }
    }

    QVariant SettingsSessionModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        const auto& sess = m_sessions[index.row()];
        if (!sess) {
            return {};
        }

        switch (index.column()) {
        case 0:
            return dataForApplication(sess, role);
        case 1:
            return dataForManage(sess, role);
        default:
            return {};
        }
    }

    QVariant SettingsSessionModel::dataForApplication(Session* sess, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
            return sess->peer();
        default:
            return {};
        }
    }

    QVariant SettingsSessionModel::dataForManage(Session* sess, int role) const
    {
        switch (role) {
        case Qt::EditRole: {
            return QVariant::fromValue(sess);
        }
        default:
            return {};
        }
    }

    void SettingsSessionModel::populateModel()
    {
        beginResetModel();

        m_sessions.clear();

        if (m_service) {
            // Add existing database tabs
            for (const auto& sess : m_service->sessions()) {
                sessionAdded(sess, false);
            }

            // connect signals
            connect(m_service, &Service::sessionOpened, this, [this](Session* sess) { sessionAdded(sess, true); });
            connect(m_service, &Service::sessionClosed, this, &SettingsSessionModel::sessionRemoved);
        }

        endResetModel();
    }

    void SettingsSessionModel::sessionAdded(Session* sess, bool emitSignals)
    {
        int row = m_sessions.size();
        if (emitSignals) {
            beginInsertRows({}, row, row);
        }

        m_sessions.append(sess);

        if (emitSignals) {
            endInsertRows();
        }
    }

    void SettingsSessionModel::sessionRemoved(Session* sess)
    {
        for (int i = 0; i != m_sessions.size(); i++) {
            if (m_sessions[i] == sess) {
                beginRemoveRows({}, i, i);

                m_sessions[i]->disconnect(this);
                m_sessions.removeAt(i);

                endRemoveRows();
                break;
            }
        }
    }

} // namespace FdoSecrets
