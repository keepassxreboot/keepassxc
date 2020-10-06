/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseSettingsPageKeeShare.h"

#include "core/Database.h"
#include "core/Group.h"
#include "gui/Icons.h"
#include "keeshare/DatabaseSettingsWidgetKeeShare.h"
#include "keeshare/KeeShare.h"

#include <QApplication>

QString DatabaseSettingsPageKeeShare::name()
{
    return QApplication::tr("KeeShare");
}

QIcon DatabaseSettingsPageKeeShare::icon()
{
    return icons()->icon("preferences-system-network-sharing");
}

QWidget* DatabaseSettingsPageKeeShare::createWidget()
{
    return new DatabaseSettingsWidgetKeeShare();
}

void DatabaseSettingsPageKeeShare::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    DatabaseSettingsWidgetKeeShare* settingsWidget = reinterpret_cast<DatabaseSettingsWidgetKeeShare*>(widget);
    settingsWidget->loadSettings(db);
}

void DatabaseSettingsPageKeeShare::saveSettings(QWidget* widget)
{
    DatabaseSettingsWidgetKeeShare* settingsWidget = reinterpret_cast<DatabaseSettingsWidgetKeeShare*>(widget);
    settingsWidget->saveSettings();
}
