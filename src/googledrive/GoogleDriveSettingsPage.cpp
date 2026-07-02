/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GoogleDriveSettingsPage.h"
#include "GoogleDriveSettingsWidget.h"
#include "gui/Icons.h"

QString GoogleDriveSettingsPage::name()
{
    return QObject::tr("Google Drive");
}

QIcon GoogleDriveSettingsPage::icon()
{
    return icons()->icon("google-drive");
}

QWidget* GoogleDriveSettingsPage::createWidget()
{
    return new GoogleDriveSettingsWidget();
}

void GoogleDriveSettingsPage::loadSettings(QWidget* widget)
{
    qobject_cast<GoogleDriveSettingsWidget*>(widget)->loadSettings();
}

void GoogleDriveSettingsPage::saveSettings(QWidget* widget)
{
    qobject_cast<GoogleDriveSettingsWidget*>(widget)->saveSettings();
}
