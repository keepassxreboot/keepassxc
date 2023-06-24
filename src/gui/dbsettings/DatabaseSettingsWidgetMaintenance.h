/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETMAINTENANCE_H
#define KEEPASSXC_DATABASESETTINGSWIDGETMAINTENANCE_H

#include "DatabaseSettingsWidget.h"

class QItemSelection;
class CustomIconModel;
class Database;
namespace Ui
{
    class DatabaseSettingsWidgetMaintenance;
}

class DatabaseSettingsWidgetMaintenance : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidgetMaintenance(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingsWidgetMaintenance);
    ~DatabaseSettingsWidgetMaintenance() override;

public slots:
    void initialize() override;
    void uninitialize() override{};
    inline bool save() override
    {
        return true;
    };

private slots:
    void selectionChanged();
    void removeCustomIcon();
    void purgeUnusedCustomIcons();

private:
    void populateIcons(QSharedPointer<Database> db);
    void removeSingleCustomIcon(QSharedPointer<Database> database, QModelIndex index);

protected:
    const QScopedPointer<Ui::DatabaseSettingsWidgetMaintenance> m_ui;

private:
    CustomIconModel* const m_customIconModel;
    uint64_t m_deletionDecision;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETMAINTENANCE_H
