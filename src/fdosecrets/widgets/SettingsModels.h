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

#ifndef KEEPASSXC_FDOSECRETS_SETTINGSMODELS_H
#define KEEPASSXC_FDOSECRETS_SETTINGSMODELS_H

#include "fdosecrets/dbus/DBusClient.h"

#include <QAbstractTableModel>

class DatabaseTabWidget;
class DatabaseWidget;

namespace FdoSecrets
{
    class SettingsDatabaseModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit SettingsDatabaseModel(DatabaseTabWidget* dbTabs, QObject* parent = nullptr);

        void setTabWidget(DatabaseTabWidget* dbTabs);

        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex& parent) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        enum Column
        {
            ColumnFileName,
            ColumnGroup,
            ColumnManage,
        };
        static constexpr const char* ColumnNames[] = {
            QT_TRANSLATE_NOOP("SettingsDatabaseModel", "File Name"),
            QT_TRANSLATE_NOOP("SettingsDatabaseModel", "Group"),
            QT_TRANSLATE_NOOP("SettingsDatabaseModel", "Manage"),
        };

    private:
        QVariant dataForName(DatabaseWidget* db, int role) const;
        static QVariant dataForExposedGroup(DatabaseWidget* db, int role);
        QVariant dataForManage(DatabaseWidget* db, int role) const;

    private slots:
        void populateModel();
        void databaseAdded(DatabaseWidget* db, bool emitSignals);
        void databaseRemoved(const QString& filePath);

    private:
        // source
        QPointer<DatabaseTabWidget> m_dbTabs;

        // internal store
        QList<QPointer<DatabaseWidget>> m_dbs;
    };

    class DBusMgr;

    class SettingsClientModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit SettingsClientModel(DBusMgr& dbus, QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex& parent) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        enum Column
        {
            ColumnApplication,
            ColumnPID,
            ColumnDBus,
            ColumnManage,
        };
        static constexpr const char* ColumnNames[] = {
            QT_TRANSLATE_NOOP("SettingsClientModel", "Application"),
            QT_TRANSLATE_NOOP("SettingsClientModel", "PID"),
            QT_TRANSLATE_NOOP("SettingsClientModel", "DBus Address"),
            QT_TRANSLATE_NOOP("SettingsClientModel", "Manage"),
        };

    private:
        QVariant dataForApplication(const DBusClientPtr& client, int role) const;
        QVariant dataForPID(const DBusClientPtr& client, int role) const;
        QVariant dataForDBus(const DBusClientPtr& client, int role) const;
        QVariant dataForManage(const DBusClientPtr& client, int role) const;

    private slots:
        void populateModel();
        void clientConnected(const DBusClientPtr& client, bool emitSignals);
        void clientDisconnected(const DBusClientPtr& client);

    private:
        // source
        DBusMgr& m_dbus;

        // internal copy, so we can emit with changed index
        QList<DBusClientPtr> m_clients;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SETTINGSMODELS_H
