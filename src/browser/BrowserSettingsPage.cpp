/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "BrowserSettingsPage.h"

#include "BrowserService.h"
#include "BrowserSettings.h"
#include "BrowserSettingsWidget.h"
#include "gui/Icons.h"

QString BrowserSettingsPage::name()
{
    return QObject::tr("Browser Integration");
}

QIcon BrowserSettingsPage::icon()
{
    return icons()->icon("internet-web-browser");
}

QWidget* BrowserSettingsPage::createWidget()
{
    return new BrowserSettingsWidget();
}

void BrowserSettingsPage::loadSettings(QWidget* widget)
{
    qobject_cast<BrowserSettingsWidget*>(widget)->loadSettings();
}

void BrowserSettingsPage::saveSettings(QWidget* widget)
{
    qobject_cast<BrowserSettingsWidget*>(widget)->saveSettings();
    browserService()->setEnabled(browserSettings()->isEnabled());
}
