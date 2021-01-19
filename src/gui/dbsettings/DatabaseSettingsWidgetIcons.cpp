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

#include "DatabaseSettingsWidgetIcons.h"
#include "ui_DatabaseSettingsWidgetIcons.h"

#include <QProgressDialog>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/MessageBox.h"
#include "gui/IconModels.h"

DatabaseSettingsWidgetIcons::DatabaseSettingsWidgetIcons(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetIcons())
    , m_customIconModel(new CustomIconModel(this))
{
    m_ui->setupUi(this);

    m_ui->customIconsView->setModel(m_customIconModel);

    connect(m_ui->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));
    connect(m_ui->purgeButton, SIGNAL(clicked()), SLOT(purgeUnusedCustomIcons()));
}

DatabaseSettingsWidgetIcons::~DatabaseSettingsWidgetIcons()
{
}

void DatabaseSettingsWidgetIcons::initialize()
{
    auto database = DatabaseSettingsWidget::getDatabase();
    if(!database) { return; }
    m_customIconModel->setIcons(database->metadata()->customIconsPixmaps(IconSize::Default),
                                database->metadata()->customIconsOrder());
}

void DatabaseSettingsWidgetIcons::removeCustomIcon()
{
}

void DatabaseSettingsWidgetIcons::purgeUnusedCustomIcons()
{
    auto database = DatabaseSettingsWidget::getDatabase();
    if(!database) { return; }

    QList<Entry*> historyEntries;
    QSet<QUuid> historicIcons;
    QSet<QUuid> iconsInUse;

    const QList<Entry*> allEntries = database->rootGroup()->entriesRecursive(true);
    for (Entry* entry : allEntries) {
        if (!entry->group()) {
            // Icons exclusively in use by historic entries (no
            // group assigned) are also purged from the database
            historyEntries << entry;
            historicIcons << entry->iconUuid();
        } else {
            iconsInUse << entry->iconUuid();
        }
    }

    const QList<Group*> allGroups = database->rootGroup()->groupsRecursive(true);
    for (Group* group : allGroups) {
        iconsInUse.insert(group->iconUuid());
    }

    int purgeCounter = 0;
    QList<QUuid> customIcons = database->metadata()->customIconsOrder();
    for (QUuid iconUuid : customIcons) {
        if (iconsInUse.contains(iconUuid)) { continue; }

        if (historicIcons.contains(iconUuid)) {
            // Remove the icon from history entries using this icon
            for (Entry* historicEntry : asConst(historyEntries)) {
                if (historicEntry->iconUuid() != iconUuid) { continue; }
                historicEntry->setUpdateTimeinfo(false);
                historicEntry->setIcon(0);
                historicEntry->setUpdateTimeinfo(true);
            }
        }

        ++purgeCounter;
        database->metadata()->removeCustomIcon(iconUuid);
    }

    if (0 == purgeCounter) {
        MessageBox::information(this,
                tr("Custom Icons Are In Use"),
                tr("All custom icons are in use by at least one entry or group."),
                MessageBox::Ok);
        return;
    }

    // update the list of icons
    initialize();

    MessageBox::information(this,
            tr("Purged Unused Icons"),
            tr("Purged %n icon(s) from the database.", "", purgeCounter),
            MessageBox::Ok);
}
