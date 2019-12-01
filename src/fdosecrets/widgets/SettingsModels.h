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

#include <QAbstractTableModel>
#include <QPointer>

class DatabaseTabWidget;
class DatabaseWidget;
class FdoSecretsPlugin;

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

    class Service;
    class Session;

    class SettingsSessionModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit SettingsSessionModel(FdoSecretsPlugin* plugin, QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex& parent) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    private:
        void setService(Service* service);

        QVariant dataForApplication(Session* sess, int role) const;
        QVariant dataForManage(Session* sess, int role) const;

    private slots:
        void populateModel();
        void sessionAdded(Session* sess, bool emitSignals);
        void sessionRemoved(Session* sess);

    private:
        // source
        QPointer<Service> m_service;

        // internal copy, so we can emit with changed index
        QList<Session*> m_sessions;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SETTINGSMODELS_H
