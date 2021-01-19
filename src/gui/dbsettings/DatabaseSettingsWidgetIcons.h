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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETICONS_H
#define KEEPASSXC_DATABASESETTINGSWIDGETICONS_H

#include "DatabaseSettingsWidget.h"

#include <QScopedPointer>

class QItemSelection;
class CustomIconModel;
class Database;
namespace Ui
{
    class DatabaseSettingsWidgetIcons;
}

class DatabaseSettingsWidgetIcons : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidgetIcons(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingsWidgetIcons);
    ~DatabaseSettingsWidgetIcons() override;

    inline bool hasAdvancedMode() const override { return false; }

public slots:
    void initialize() override;
    void uninitialize() override { };
    inline bool save() override { return true; };

private slots:
    void selectionChanged();
    void removeCustomIcon();
    void purgeUnusedCustomIcons();

private:
    void populateIcons(QSharedPointer<Database> db);
    void removeSingleCustomIcon(QSharedPointer<Database> database, QModelIndex index);

protected:
    const QScopedPointer<Ui::DatabaseSettingsWidgetIcons> m_ui;

private:
    CustomIconModel* const m_customIconModel;
    uint64_t m_deletionDecision;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETICONS_H
