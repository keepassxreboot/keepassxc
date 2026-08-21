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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H
#define KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H

#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QWidget>

#include "fdosecrets/ClientAuth.h"

namespace Ui
{
    class DatabaseSettingsWidgetFdoSecrets;
}

namespace FdoSecrets
{
    class ClientRecordsModel;
}

class Database;
class Entry;
class DatabaseSettingsWidgetFdoSecrets : public QWidget
{
    Q_OBJECT
public:
    explicit DatabaseSettingsWidgetFdoSecrets(QWidget* parent = nullptr);
    ~DatabaseSettingsWidgetFdoSecrets() override;

    void loadSettings(QSharedPointer<Database> db);
    void saveSettings();

private slots:
    void addClientRecord();
    void editClientRecord();
    void removeClientRecord();
    void updateRecordButtons();

private:
    void settingsWarning();
    void updateOverlapWarning();
    /// Fit the group tree to its content, capped by the maximum height from the .ui
    void updateGroupViewHeight();
    /// Hand the staged records to the model, with decision counts staged removals already applied
    void refreshRecords();
    /// Give the rules column whatever the other two do not need
    void updateRecordColumns();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:

private:
    QScopedPointer<Ui::DatabaseSettingsWidgetFdoSecrets> m_ui;

    QSharedPointer<Database> m_db;

    class GroupModelNoRecycle;
    QScopedPointer<GroupModelNoRecycle> m_model;

    FdoSecrets::ClientRecordsModel* m_recordsModel = nullptr;

    // staged until the settings are saved, like every other page of the dialog
    QList<FdoSecrets::ClientRecord> m_records;
    QList<FdoSecrets::DBusClientId> m_removedRecords;
    QList<QPair<QPointer<Entry>, FdoSecrets::DBusClientId>> m_removedDecisions;

    bool m_updatingColumns = false;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H
