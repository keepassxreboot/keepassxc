/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "WhisperSettingsPage.h"

#include "WhisperSettingsWidget.h"
#include "gui/Icons.h"

QString WhisperSettingsPage::name()
{
    return QObject::tr("Whisper");
}

QIcon WhisperSettingsPage::icon()
{
    return icons()->icon("share-link");
}

QWidget* WhisperSettingsPage::createWidget()
{
    return new WhisperSettingsWidget();
}

void WhisperSettingsPage::loadSettings(QWidget* widget)
{
    qobject_cast<WhisperSettingsWidget*>(widget)->loadSettings();
}

void WhisperSettingsPage::saveSettings(QWidget* widget)
{
    qobject_cast<WhisperSettingsWidget*>(widget)->saveSettings();
}
