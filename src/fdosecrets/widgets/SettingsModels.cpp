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

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/dbus/DBusMgr.h"

#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/Icons.h"

#include <QFileInfo>
#include <iterator>

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
    constexpr const char* SettingsAliasesModel::ColumnNames[];

    SettingsAliasesModel::SettingsAliasesModel(const DatabaseTabWidget* dbTabs, QObject* parent)
        : QAbstractTableModel(parent)
        , m_databases(dbTabs)
    {
    }

    void SettingsAliasesModel::setAliases(QVariantMap aliases)
    {
        beginResetModel();
        m_aliases = aliases;
        endResetModel();
    }

    const QVariantMap& SettingsAliasesModel::aliases() const
    {
        return m_aliases;
    }

    int SettingsAliasesModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return newRowIndex() + extraNewRow;
    }

    int SettingsAliasesModel::newRowIndex() const
    {
        return m_aliases.size();
    }

    int SettingsAliasesModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            return 0;
        }
        return sizeof(ColumnNames) / sizeof(ColumnNames[0]);
    }

    QVariant SettingsAliasesModel::headerData(int section, Qt::Orientation orientation, int role) const
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

    QVariantMap::const_iterator SettingsAliasesModel::rowAlias(const QModelIndex& index) const
    {
        return std::next(m_aliases.cbegin(), index.row());
    }

    QVariantMap::iterator SettingsAliasesModel::rowAlias(const QModelIndex& index)
    {
        return std::next(m_aliases.begin(), index.row());
    }

    QVariant SettingsAliasesModel::data(const QModelIndex& index, int role) const
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
        if (index.row() == newRowIndex()) { // final empty row, to allow adding more aliases
            return {};
        }

        switch (index.column()) {
        case ColumnAlias:
            return dataForCollectionAlias(rowAlias(index).key(), role);
        case ColumnDatabase:
            return dataForDatabase(rowAlias(index)->toUuid(), role);
        default:
            return {};
        }
    }

    QVariant SettingsAliasesModel::dataForCollectionAlias(const QString& alias, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            return alias;
        }
        default:
            return {};
        }
    }

    QVariant SettingsAliasesModel::dataForDatabase(const QUuid& publicUuid, int role) const
    {
        if (role == Qt::EditRole) {
            return publicUuid; // initial value for editor for this cell
        }

        auto dbWidget = m_databases->databaseWidgetFromPublicUuid(publicUuid);
        if (dbWidget) {
            auto db = dbWidget->database();
            switch (role) {
            case Qt::DisplayRole: {
                return dbWidget->displayName();
            }
            case Qt::ToolTipRole:
                return db->filePath();
            default:
                return {};
            }
        } else if (publicUuid.isNull()) {
            switch (role) {
            case Qt::DisplayRole: {
                return tr("No database selected.");
            }
            case Qt::FontRole: {
                QFont font;
                font.setItalic(true);
                return font;
            }
            default:
                return {};
            }
        } else {
            switch (role) {
            case Qt::DisplayRole: {
                return publicUuid;
            }
            case Qt::ToolTipRole:
                return tr("The database with this UUID is not currently opened.");
            case Qt::FontRole: {
                QFont font;
                font.setItalic(true);
                return font;
            }
            default:
                return {};
            }
        }
    }

    void SettingsAliasesModel::moveAlias(const QModelIndex& index,
                                         const QString& prevAlias,
                                         QString nextAlias,
                                         QVariant database)
    {
        const auto destIt = std::as_const(m_aliases).lowerBound(nextAlias);
        // beginMoveRows takes that element's current index, in front of which we insert
        auto destRowIdx = std::distance(m_aliases.cbegin(), destIt);
        // inserting directly above or below the element to be removed doesn't move
        const bool willMove = destRowIdx != index.row() && destRowIdx - 1 != index.row();
        if (willMove) {
            // move index.row() to destRowIdx
            beginMoveRows({}, index.row(), index.row(), {}, destRowIdx);
        }
        const bool newRow = index.row() == newRowIndex();
        m_aliases.insert(std::move(destIt), std::move(nextAlias), std::move(database));
        if (newRow) {
            // remove the special "newRow", which does not correspond to m_aliases
            extraNewRow = false; // cannot nest beginMoveRows and beginInsertRows
        } else {
            // remove a normal row, which does correspond to an m_aliases entry
            m_aliases.remove(prevAlias);
        }
        if (willMove) {
            // one insert and one remove -> we moved a row
            endMoveRows();
        }
        // during this move, we might have also changed the cell values
        if (index.row() < destRowIdx) {
            --destRowIdx; // shifted up, when we removed index.row()
        }
        emit dataChanged(this->index(destRowIdx, ColumnAlias), this->index(destRowIdx, ColumnDatabase));
        if (newRow) {
            // create a new "newRow", if we (re)moved the old one
            beginInsertRows({}, newRowIndex(), newRowIndex());
            extraNewRow = true; // increases this->rowCount() by one
            endInsertRows();
        }
    }

    bool SettingsAliasesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        switch (index.column()) {
        case ColumnAlias: {
            QString prevAlias = "";
            QString nextAlias = value.toString();
            QVariant database = QUuid();
            if (index.row() == newRowIndex()) {
                if (nextAlias.isEmpty())
                    return false; // don't add empty alias
                if (m_aliases.contains(nextAlias))
                    return false; // don't override existing alias
            } else {
                auto prevRow = rowAlias(index);
                prevAlias = prevRow.key();
                database = prevRow.value();
                if (nextAlias.isEmpty()) {
                    beginRemoveRows({}, index.row(), index.row());
                    m_aliases.erase(prevRow);
                    endRemoveRows();
                    return true; // edit to empty -> remove row
                }
                if (nextAlias == prevAlias) {
                    return true; // editor didn't change this value
                }
                if (m_aliases.contains(nextAlias)) {
                    return false; // refuse duplicate alias
                }
            }
            moveAlias(index, prevAlias, std::move(nextAlias), std::move(database));
            return true;
        }
        case ColumnDatabase: {
            auto nextDatabase = value.toUuid();
            QString alias = "default";
            if (index.row() == newRowIndex()) {
                // find a new unique alias name
                int i = 1;
                while (m_aliases.contains(alias)) {
                    alias = QString("alias%1").arg(i++);
                }
                // insert new alias for selected database
                moveAlias(index, "", std::move(alias), std::move(nextDatabase));
            } else {
                // just update this value (won't move/insert/remove any rows)
                *rowAlias(index) = nextDatabase;
                emit dataChanged(index, index);
            }
            return true;
        }
        default:
            return QAbstractTableModel::setData(index, value, role);
        }
    }

    Qt::ItemFlags SettingsAliasesModel::flags(const QModelIndex& index) const
    {
        // all table cells are editable (see SettingsAliasesModel::setData())
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }

    void SettingsAliasesModel::removeRow(int row)
    {
        beginRemoveRows({}, row, row);
        m_aliases.erase(std::next(m_aliases.begin(), row));
        endRemoveRows();
    }

    // static constexpr still requires definition before c++17
    constexpr const char* SettingsClientModel::ColumnNames[];

    SettingsClientModel::SettingsClientModel(DBusMgr& dbus, QObject* parent)
        : QAbstractTableModel(parent)
        , m_dbus(dbus)
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
        case ColumnManage:
            return dataForManage(client, role);
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

    QVariant SettingsClientModel::dataForManage(const DBusClientPtr& client, int role) const
    {
        switch (role) {
        case Qt::EditRole: {
            return QVariant::fromValue(client);
        }
        default:
            return {};
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

        endResetModel();
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
