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

#include "DatabaseSettingsPageFdoSecrets.h"

#include "fdosecrets/widgets/DatabaseSettingsWidgetFdoSecrets.h"

#include "gui/Icons.h"

QString DatabaseSettingsPageFdoSecrets::name()
{
    return QObject::tr("Secret Service Integration");
}

QIcon DatabaseSettingsPageFdoSecrets::icon()
{
    return icons()->icon(QStringLiteral("freedesktop"));
}

QWidget* DatabaseSettingsPageFdoSecrets::createWidget()
{
    return new DatabaseSettingsWidgetFdoSecrets;
}

void DatabaseSettingsPageFdoSecrets::loadSettings(QWidget* widget, QSharedPointer<Database> db)
{
    auto settingsWidget = qobject_cast<DatabaseSettingsWidgetFdoSecrets*>(widget);
    settingsWidget->loadSettings(db);
}

void DatabaseSettingsPageFdoSecrets::saveSettings(QWidget* widget)
{
    auto settingsWidget = qobject_cast<DatabaseSettingsWidgetFdoSecrets*>(widget);
    settingsWidget->saveSettings();
}
